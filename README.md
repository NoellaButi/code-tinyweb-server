# Tiny Web Server — wserver 🌐⚡  
Multithreaded Static File Server with Thread Pool, Bounded Queue, and Safe Paths  

![Language](https://img.shields.io/badge/language-C-blue.svg) 
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg) 
![Build](https://img.shields.io/badge/build-Makefile-orange.svg)  

---

✨ **Overview**  
This project implements a multithreaded HTTP/1.0 static file server in C.  
Originally an Operating Systems assignment, it was extended into a practical, portfolio-ready project.  

It demonstrates mastery of:  
- **Concurrency** with thread pools  
- **Synchronization** (mutex + condition variables)  
- **System-level I/O** with `sendfile()`  
- **Security** via safe path resolution  

🛠️ **Workflow**  
- Main (Producer): accepts client connections, enqueues requests  
- Worker Threads (Consumers): dequeue requests, serve files  
- Safe Paths: prevents directory traversal (`..`) via `realpath()`  
- MIME Detection: detects `.html`, `.css`, `.js`, images, etc.  
- Zero-Copy Send: efficient file transfer with `sendfile()`  
- Flags:  
  - `-d` → docroot  
  - `-p` → port (default 10000)  
  - `-t` → number of threads  
  - `-b` → queue size  

📁 **Repository Layout**  
```bash
src/        # server source (wserver.c)
public/     # static HTML/CSS/JS files
docs/       # screenshots, benchmarks (ab-*.txt)
Makefile    # build rules
README.md   # this overview
```

🚦 **Demo**

Build and run:

```bash
make
./wserver -d ./public -p 10000 -t 8 -b 128
```

Visit in browser:

```bash
http://localhost:10000
```

Fetch with curl:

```bash

curl -i http://localhost:10000/
curl -i http://localhost:10000/products.html
```

🔍 **Features**

- Thread Pool + Bounded Queue (pthread mutex + cond vars)
- Safe Path Resolution (no .. traversal)
- MIME Types (HTML, CSS, JS, images)
- HTTP/1.0 Responses (200, 404, 405 with headers)
- Zero-Copy sendfile for performance

🚦 **Results (Benchmarks)**

```bash
ab -n 500 -c 50 → ~1137 req/s, P50 43 ms, 0 failed
ab -n 1000 -c 50 → ~1200 req/s, P50 43 ms, 0 failed
```

📜 **License**

MIT (see [LICENSE](LICENSE))

---
