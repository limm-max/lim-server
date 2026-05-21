# lim-server

> 一个基于 C++17 从零实现的高性能 Web 服务器框架，包含自研网络库与 HTTP 应用层。

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-green.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)]()
[![License](https://img.shields.io/badge/License-MIT-orange.svg)]()

## 📖 项目简介

`lim-server` 是一个**从零开始**实现的现代 C++ Web 服务器框架，采用经典的**主从 Reactor 多线程模型**，分为底层网络库和上层 HTTP 框架两个层次：

- **底层网络库**：参考 muduo 库设计，实现了基于 epoll 的高性能 Reactor 网络模型
- **上层 HTTP 框架**：在网络库之上构建完整的 HTTP/1.0，HTTP/1.1 应用层框架，支持路由、中间件、cookie等

本项目旨在**深度实践** C++ 服务器编程技术，涵盖 Linux 系统编程、网络编程、并发编程、HTTP 协议等核心知识。

'''
┌─────────────────────────────────────────┐
│  业务层 (Handler / Service / DAO)  
|  未详细设计，后续考虑设计详细业务          |
├─────────────────────────────────────────┤
│  HTTP 框架                               │
│  ├─ Router          路由分发              │
│  ├─ MiddlewareChain 中间件管道           │
│  ├─ SessionManager  会话管理              │
│  ├─ HttpContext     协议解析状态机        │
│  └─ HttpServer      HTTP 服务总装         │
├─────────────────────────────────────────┤
│  TCP 网络层 (lim-server core)            │
│  ├─ TcpServer + Acceptor                │
│  ├─ EventLoop + EventLoopThreadPool     │
│  ├─ Channel + Poller (epoll)            │
│  ├─ TcpConnection + Buffer              │
│  └─ AsyncLogging (异步日志)              │
├─────────────────────────────────────────┤
│  Linux Kernel (epoll / socket)          │
└─────────────────────────────────────────┘
'''