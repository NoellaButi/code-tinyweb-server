#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* ------------------ Config via CLI flags ------------------ */
static char  g_basedir[PATH_MAX] = "./public";
static int   g_port   = 10000;
static int   g_nthreads = 4;
static int   g_qsize  = 64;
static int   g_listen_fd = -1;
static volatile sig_atomic_t g_shutdown = 0;

/* ------------------ Bounded queue (ring buffer) ----------- */
typedef struct {
    int *buf;
    int cap;
    int head; // next pop
    int tail; // next push
    int size;
    pthread_mutex_t m;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} fdqueue_t;

static void q_init(fdqueue_t *q, int cap) {
    q->buf = (int*)malloc(sizeof(int)*cap);
    q->cap = cap; q->head = q->tail = q->size = 0;
    pthread_mutex_init(&q->m, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}
static void q_destroy(fdqueue_t *q) {
    free(q->buf);
    pthread_mutex_destroy(&q->m);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}
static void q_push(fdqueue_t *q, int fd) {
    pthread_mutex_lock(&q->m);
    while (q->size == q->cap) pthread_cond_wait(&q->not_full, &q->m);
    q->buf[q->tail] = fd;
    q->tail = (q->tail + 1) % q->cap;
    q->size++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->m);
}
static int q_pop(fdqueue_t *q) {
    pthread_mutex_lock(&q->m);
    while (q->size == 0) pthread_cond_wait(&q->not_empty, &q->m);
    int fd = q->buf[q->head];
    q->head = (q->head + 1) % q->cap;
    q->size--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->m);
    return fd;
}

static fdqueue_t g_queue;

/* ------------------ Utilities ------------------ */
static void die(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}
static void perr(const char *msg) { perror(msg); }

static ssize_t write_all(int fd, const void *buf, size_t len) {
    const char *p = (const char*)buf;
    size_t nleft = len;
    while (nleft > 0) {
        ssize_t n = write(fd, p, nleft);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break;
        p += n; nleft -= (size_t)n;
    }
    return (ssize_t)(len - nleft);
}

static void http_sendf(int fd, const char *fmt, ...) {
    char buf[4096];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) write_all(fd, buf, (size_t)n);
}

static void http_respond_simple(int fd, int code, const char *reason, const char *ctype, const char *body) {
    size_t blen = body ? strlen(body) : 0;
    http_sendf(fd,
        "HTTP/1.0 %d %s\r\n"
        "Server: mytiny/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        code, reason, ctype ? ctype : "text/plain; charset=utf-8", blen);
    if (blen) write_all(fd, body, blen);
}

static bool starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static const char* mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (!strcasecmp(ext, ".html") || !strcasecmp(ext, ".htm")) return "text/html; charset=utf-8";
    if (!strcasecmp(ext, ".css"))  return "text/css";
    if (!strcasecmp(ext, ".js"))   return "application/javascript";
    if (!strcasecmp(ext, ".png"))  return "image/png";
    if (!strcasecmp(ext, ".jpg") || !strcasecmp(ext, ".jpeg")) return "image/jpeg";
    if (!strcasecmp(ext, ".gif"))  return "image/gif";
    if (!strcasecmp(ext, ".svg"))  return "image/svg+xml";
    if (!strcasecmp(ext, ".txt"))  return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

/* Read a line (CRLF or LF) */
static ssize_t read_line(int fd, char *buf, size_t cap) {
    size_t i = 0; char c;
    while (i + 1 < cap) {
        ssize_t r = read(fd, &c, 1);
        if (r == 0) break;
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (c == '\r') continue;
        buf[i++] = c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

/* Drain rest of headers */
static void drain_headers(int fd) {
    char line[4096];
    while (1) {
        ssize_t n = read_line(fd, line, sizeof(line));
        if (n <= 0) break;
        if (strcmp(line, "\n") == 0) break;
    }
}

/* Build safe path under g_basedir; return 0 on success */
static int build_safe_path(const char *url_path, char *out, size_t outcap) {
    // default file
    const char *p = url_path;
    if (!*p || !strcmp(p, "/")) p = "/index.html";

    // avoid query string
    char path_only[PATH_MAX];
    strncpy(path_only, p, sizeof(path_only)); path_only[sizeof(path_only)-1] = '\0';
    char *q = strchr(path_only, '?'); if (q) *q = '\0';

    // join basedir + path_only
    char joined[PATH_MAX];
    if (snprintf(joined, sizeof(joined), "%s/%s", g_basedir, path_only[0]=='/'? path_only+1 : path_only) >= (int)sizeof(joined))
        return -1;

    // realpath checks
    char realdoc[PATH_MAX], realbase[PATH_MAX];
    if (!realpath(joined, realdoc)) return -1;
    if (!realpath(g_basedir, realbase)) return -1;

    // ensure doc is within basedir
    if (!starts_with(realdoc, realbase)) return -1;

    strncpy(out, realdoc, outcap); out[outcap-1] = '\0';
    return 0;
}

/* Handle one HTTP request */
static void handle_conn(int fd) {
    char reqline[4096];
    ssize_t n = read_line(fd, reqline, sizeof(reqline));
    if (n <= 0) { close(fd); return; }

    // Expect: "GET /path HTTP/1.0" or HTTP/1.1
    char method[16], url[PATH_MAX], version[16];
    if (sscanf(reqline, "%15s %1023s %15s", method, url, version) != 3) {
        http_respond_simple(fd, 400, "Bad Request", "text/plain; charset=utf-8", "Bad Request\n");
        close(fd); return;
    }

    // Read & discard headers (no keep-alive for simplicity)
    drain_headers(fd);

    if (strcasecmp(method, "GET") != 0) {
        http_respond_simple(fd, 405, "Method Not Allowed", "text/plain; charset=utf-8", "Only GET supported\n");
        close(fd); return;
    }

    char path[PATH_MAX];
    if (build_safe_path(url, path, sizeof(path)) != 0) {
        http_respond_simple(fd, 404, "Not Found", "text/plain; charset=utf-8", "Not Found\n");
        close(fd); return;
    }

    int f = open(path, O_RDONLY);
    if (f < 0) {
        http_respond_simple(fd, 404, "Not Found", "text/plain; charset=utf-8", "Not Found\n");
        close(fd); return;
    }

    struct stat st;
    if (fstat(f, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(f);
        http_respond_simple(fd, 404, "Not Found", "text/plain; charset=utf-8", "Not Found\n");
        close(fd); return;
    }

    const char *ctype = mime_type(path);
    http_sendf(fd,
        "HTTP/1.0 200 OK\r\n"
        "Server: mytiny/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n", ctype, (long)st.st_size);

    off_t offset = 0;
    ssize_t sent;
    while (offset < st.st_size) {
        sent = sendfile(fd, f, &offset, (size_t)(st.st_size - offset));
        if (sent <= 0) { if (errno == EINTR) continue; break; }
    }
    close(f);
    close(fd);
}

/* Worker thread */
static void* worker_main(void *arg) {
    (void)arg;
    while (!g_shutdown) {
        int cfd = q_pop(&g_queue);
        if (cfd >= 0) handle_conn(cfd);
    }
    return NULL;
}

/* Setup listener */
static int listen_or_die(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) die("socket failed");

    int yes=1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) die("bind failed");
    if (listen(fd, 128) < 0) die("listen failed");
    return fd;
}

/* Ctrl-C handler */
static void on_sigint(int signo) {
    (void)signo;
    g_shutdown = 1;
    if (g_listen_fd >= 0) close(g_listen_fd);
}

/* Parse CLI */
static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-d basedir] [-p port] [-t threads] [-b qsize]\n", prog);
}
static void parse_flags(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "d:p:t:b:")) != -1) {
        switch (opt) {
            case 'd':
                if (!realpath(optarg, g_basedir)) die("Invalid basedir");
                break;
            case 'p': g_port = atoi(optarg); break;
            case 't': g_nthreads = atoi(optarg); break;
            case 'b': g_qsize = atoi(optarg); break;
            default: usage(argv[0]); exit(1);
        }
    }
}

int main(int argc, char **argv) {
    /* defaults: resolve default basedir safely (no aliasing) */
    {
        char tmpreal[PATH_MAX];
        if (!realpath(g_basedir, tmpreal)) die("realpath on basedir failed (create ./public?)");
        strncpy(g_basedir, tmpreal, sizeof(g_basedir));
        g_basedir[sizeof(g_basedir)-1] = '\0';
    }

    parse_flags(argc, argv);
    if (g_port <= 0 || g_port > 65535) die("invalid port");
    if (g_nthreads <= 0) g_nthreads = 4;
    if (g_qsize <= 0) g_qsize = 64;

    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    q_init(&g_queue, g_qsize);

    pthread_t *workers = (pthread_t*)malloc(sizeof(pthread_t)*g_nthreads);
    for (int i=0;i<g_nthreads;i++) pthread_create(&workers[i], NULL, worker_main, NULL);

    g_listen_fd = listen_or_die(g_port);
    fprintf(stderr, "listening on http://localhost:%d  (root: %s, threads: %d, q:%d)\n",
            g_port, g_basedir, g_nthreads, g_qsize);

    while (!g_shutdown) {
        struct sockaddr_in cli; socklen_t len = sizeof(cli);
        int cfd = accept(g_listen_fd, (struct sockaddr*)&cli, &len);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            perr("accept"); break;
        }
        q_push(&g_queue, cfd);
    }

    /* shutdown */
    for (int i=0;i<g_nthreads;i++) q_push(&g_queue, -1);
    for (int i=0;i<g_nthreads;i++) pthread_join(workers[i], NULL);
    free(workers);
    q_destroy(&g_queue);
    return 0;
}
