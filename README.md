# Tiny Web Server — wserver 🌐⚡
Multithreaded Static File Server with Thread Pool, Bounded Queue, and Safe Paths

![GitHub repo size](https://img.shields.io/github/repo-size/NoellaButi/tinyweb-server)
![GitHub issues](https://img.shields.io/github/issues/NoellaButi/tinyweb-server)
![GitHub last commit](https://img.shields.io/github/last-commit/NoellaButi/tinyweb-server)
![GitHub license](https://img.shields.io/github/license/NoellaButi/tinyweb-server)

**Author:** Noëlla Buti

---

## ✨ Overview
This project implements a multithreaded HTTP/1.0 static file server in C.  
Originally an academic OS assignment, it has been extended into a practical portfolio-ready project.

It supports thread pooling, safe path resolution, MIME detection, and efficient file transfers with `sendfile()`.  
The goal: demonstrate mastery of concurrency, synchronization, and system-level I/O.

---

## 🛠️ Workflow
- 🧠 **Main (Producer):** accepts client connections and enqueues requests  
- 👷 **Worker Threads (Consumers):** dequeue requests and serve files  
- 🔐 **Safe Paths:** prevents directory traversal attacks via `realpath()`  
- 🗂️ **MIME Detection:** `.html`, `.css`, `.js`, images, etc.  
- ⚡ **Zero-Copy Send:** `sendfile()` for efficient payload delivery  
- ⚙️ **Flags:**  
  - `-d` → docroot  
  - `-p` → port (default 10000)  
  - `-t` → number of threads  
  - `-b` → queue size  

---

## 🚦 Demo
Run the server:
```bash
make
./wserver -d ./public -p 10000 -t 8 -b 128
# listening on http://localhost:10000 (root: ./public, threads: 8, q:128)
```

Fetch content:
```bash
curl -i http://localhost:10000/
curl -i http://localhost:10000/products.html
```
---

## 📁 Repository Layout
```bash
src/      → main server source (wserver.c)
public/   → static HTML files to serve
docs/     → screenshots, benchmark results (ab-*.txt)
Makefile  → build rules
README.md → this overview
```
---



---

## 🔍 Features
- Thread Pool + Bounded Queue (pthread mutex + cond)
- Safe Path Resolution (no .. traversal)
- MIME Types (HTML, CSS, JS, images, text)
- HTTP/1.0 Responses (200, 404, 405 with headers)
- Zero-Copy Sendfile for performance

---
##🚦 Results (Latest)
- Config: ./wserver -d ./public -p 10000 -t 2 -b 32 (localhost)
- ab -n 500 -c 50 → 1137 req/s, P50 43 ms, 0 failed
- ab -n 1000 -c 50 → ~1200 req/s, P50 43 ms, 0 failed
Raw outputs:
`docs/ab-500x50.txt`

`docs/ab-1000x50.txt`

---

## 📜 License
MIT (see [LICENSE](LICENSE))

---

