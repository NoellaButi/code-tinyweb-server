# Tiny Shell — mysh 🐚⚡  
Custom Unix-like Shell with Built-ins, Aliases, Pipes, and Redirection  

[![Language](https://img.shields.io/badge/language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))  
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)  
[![Build](https://img.shields.io/badge/build-Makefile-orange.svg)](Makefile)  

---

✨ **Overview**  
This project implements a lightweight shell (`mysh`) in C.  
It started as an Operating Systems assignment and was extended into a practical developer tool.  

It demonstrates mastery of:  
- **Process control** (fork, execvp, wait)  
- **System calls** (I/O, file descriptors)  
- **User experience** (history, aliases, configs)  

🛠️ **Workflow**  
- Prompt: `user@host:cwd$mysh>` dynamic display  
- Loop: read → parse → execute → re-prompt  
- Built-ins: `cd`, `pwd`, `exit`, `export`, `alias`, `unalias`, `which`  
- Config: loads `~/.myshrc` (aliases, exports, etc.)  
- Environment: support for `export VAR=value`  
- History & editing: via GNU Readline (`↑ ↓`, `Ctrl+R`)  
- External commands: launched via `fork + execvp + wait`  
- Pipes & redirection: `|`, `<`, `>`, `>>` across multiple stages  
- Errors: descriptive messages (e.g., *No such directory*)  

📁 **Repository Layout**  
```bash
src/ # server source (wserver.c)
public/ # static HTML/CSS/JS files
docs/ # screenshots, benchmarks (ab-*.txt)
Makefile # build rules
README.md # this overview
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
