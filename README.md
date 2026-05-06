# lim-server

> A high-performance C++ web server framework with self-implemented network library and HTTP application layer.

## ✨ Project Overview

`lim-server` is a from-scratch implementation of a modern C++17 web server framework, designed as a layered architecture:

- **Layer 1 — Base infrastructure**: async logging, thread utilities, timestamp
- **Layer 2 — Network library**: a Reactor-based TCP networking library (inspired by muduo), featuring `EventLoop`, `Channel`, `EpollPoller`, `Buffer`, and `EventLoopThreadPool`
- **Layer 3 — HTTP framework**: HTTP/1.1 parsing, routing system, middleware chain, session management

## 🏗 Architecture
┌─────────────────────────────────┐
│  HTTP Framework (Layer 3)       │
│  Router / Middleware / Session  │
├─────────────────────────────────┤
│  Network Library (Layer 2)      │
│  TcpServer / EventLoop / Buffer │
├─────────────────────────────────┤
│  Base Infrastructure (Layer 1)  │
│  Logger / Thread / Timestamp    │
└─────────────────────────────────┘
## 🚀 Build

```bash
mkdir build && cd build
cmake ..
make
```

## 📋 Roadmap

- [x] Project skeleton & build system
- [ ] Base infrastructure (logging, thread)
- [ ] Network library (Reactor, Buffer, EventLoop)
- [ ] HTTP framework (parser, router, middleware)
- [ ] HTTPS support (OpenSSL)
- [ ] Demo application & benchmark

## 📚 Tech Stack

- **Language**: C++17
- **Build**: CMake 3.10+
- **Platform**: Linux (Ubuntu 24.04 tested)
- **Compiler**: g++ 13+

## 📝 License

MIT (TBD)