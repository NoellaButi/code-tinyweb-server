# Tiny Web Server — wserver 🌐⚡  
Multithreaded Static File Server with Thread Pool, Bounded Queue, and Safe Paths

![Language](https://img.shields.io/badge/language-C-blue.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)
![Build](https://img.shields.io/badge/build-Makefile-orange.svg)
![CI](https://github.com/NoellaButi/code-tinyweb-server/actions/workflows/ci.yml/badge.svg?branch=main)

---

## ✨ Overview
A compact HTTP/1.0 static file server in C with:
- **Thread pool** + **bounded request queue**
- **Safe path resolution** (no `..` traversal)
- **MIME detection** for common types
- **Zero-copy** fast path using `sendfile()` when available

---

## 🔍 Features
- HTTP/1.0 responses: `200`, `404`, `405` with headers  
- Flags:
  - `-d` → docroot (default: `./public`)
  - `-p` → port (default: `10000`)
  - `-t` → number of worker threads (default: `8`)
  - `-b` → queue size (default: `128`)
- Thread pool (pthreads) with producer/consumer queue  
- `realpath()` safe path normalization  
- Basic MIME map: `.html`, `.css`, `.js`, `.png`, `.jpg`, `.gif`, `.svg`, `.txt`, …

---

## 🚦 Quickstart

Build:
```bash
make
```

Run:
```bash
./wserver -d ./public -p 10000 -t 8 -b 128
```

Open in a browser:
```arduino
http://localhost:10000/
```

Or curl:
```bash
curl -i http://localhost:10000/
curl -i http://localhost:10000/does-not-exist.txt   # expect 404
```

## 📁 Repository Layout
```bash
code-tinyweb-server/
├─ src/         # server source (wserver.c, helpers)
├─ public/      # static files to serve (index.html, assets/)
├─ docs/        # screenshots, benchmarks
├─ tests/       # optional scripts
├─ Makefile     # gcc + pthread + sendfile
└─ README.md
```

## 📊 Results (Benchmarks)
```text
ab -n 500 -c 50  → ~1137 req/s,  P50 43 ms, 0 failed
ab -n 1000 -c 50 → ~1200 req/s,  P50 43 ms, 0 failed
```

## 🔮 Roadmap
- keep-alive support (HTTP/1.1)
- Directory listings toggle
- Access logs + latency histograms
- Basic cache headers

## 📜 License
MIT (see LICENSE)

---
