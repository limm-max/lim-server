# lim-server

> 一个基于 C++17 从零实现的高性能 Web 服务器框架，包含自研网络库与 HTTP 应用层。

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-green.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)]()
[![License](https://img.shields.io/badge/License-MIT-orange.svg)]()

## 📖 项目简介

`lim-server` 是一个**从零开始**实现的现代 C++ Web 服务器框架，采用经典的**主从 Reactor 多线程模型**，分为底层网络库和上层 HTTP 框架两个层次：

- **底层网络库**：参考 muduo 库设计，实现了基于 epoll 的高性能 Reactor 网络模型
- **上层 HTTP 框架**：在网络库之上构建完整的 HTTP/1.1 应用层框架，支持路由、中间件、会话管理等

本项目旨在**深度实践** C++ 服务器编程技术，涵盖 Linux 系统编程、网络编程、并发编程、HTTP 协议等核心知识。

## 🏗 整体架构
'''
┌─────────────────────────────────────────────────┐
│  应用层 (Application Layer)                      │
│  - 业务示例：Echo Server / 简易论坛 / 文件服务   │
├─────────────────────────────────────────────────┤
│  HTTP 框架层 (HTTP Framework)                    │
│  - 路由系统（静态 + 动态正则路由）                │
│  - 中间件链（CORS、日志、鉴权）                   │
│  - 会话管理（Session + Cookie）                  │
│  - 协议解析（主从状态机）                         │
├─────────────────────────────────────────────────┤
│  网络库层 (Network Library) ⭐ 自研               │
│  - TcpServer / TcpConnection / Acceptor          │
│  - EventLoop / Channel / EpollPoller             │
│  - Buffer / TimerQueue (timerfd)                 │
│  - EventLoopThreadPool                           │
├─────────────────────────────────────────────────┤
│  基础设施层 (Base Infrastructure)                 │
│  - 异步日志（双缓冲）                             │
│  - 线程封装 / 时间戳                              │
└─────────────────────────────────────────────────┘
'''
## ✨ 核心特性

### 网络库层
- **主从 Reactor 模型**：One Loop Per Thread，主 Reactor 负责接受连接，从 Reactor 处理 IO
- **epoll LT 模式 + 非阻塞 IO**：支撑高并发连接
- **应用层 Buffer**：参考 muduo 三段式设计（prependable / readable / writable），结合 readv 栈空间扩容机制
- **基于 timerfd 的定时器**：用红黑树管理，将定时事件统一为 IO 事件
- **eventfd 唤醒机制**：跨 EventLoop 的高效线程通信
- **双缓冲异步日志**：前后端解耦，避免日志写入阻塞主流程

### HTTP 框架层
- **HTTP/1.1 协议解析**：基于主从状态机，支持 Keep-Alive 长连接与管线化
- **路由系统**：支持静态路由（精确匹配）和动态路由（`/users/:id` 正则匹配）
- **中间件链**：洋葱模型设计，支持 CORS、鉴权等横切关注点
- **会话管理**：基于 Cookie 的 Session 机制
- **数据库连接池**：（计划中）MySQL 连接池
- **HTTPS 支持**：（计划中）OpenSSL 集成

## 🛠 技术栈

| 类别 | 选型 |
|------|------|
| 语言标准 | C++17 |
| 构建系统 | CMake 3.10+ |
| 运行平台 | Linux（Ubuntu 24.04 测试通过） |
| 编译器 | g++ 13+ |
| 网络模型 | epoll + 主从 Reactor |
| 并发模型 | One Loop Per Thread |
| 日志系统 | 自研双缓冲异步日志 |

## 🚀 快速开始

### 环境要求

- Linux（推荐 Ubuntu 22.04+）
- g++ 9+ （支持 C++17）
- CMake 3.10+

### 构建项目

```bash
# 克隆仓库
git clone git@github.com:limm-max/lim-server.git
cd lim-server

# 创建构建目录并编译
mkdir build && cd build
cmake ..
make

# 运行示例（待开发完成后补充）
./bin/echo_server 8080
```

## 📁 项目结构
'''
lim-server/
├── base/             # 基础设施层（日志、线程、时间戳）
│   ├── include/
│   └── src/
├── net/              # 网络库层（EventLoop、TcpServer 等）
│   ├── include/
│   └── src/
├── http/             # HTTP 框架层（Router、Middleware 等）
│   ├── include/
│   └── src/
├── examples/         # 示例程序
├── tests/            # 单元测试
├── docs/             # 设计文档
├── scripts/          # 辅助脚本
└── CMakeLists.txt    # 顶层构建文件
'''
## 📋 开发进度

### 阶段一：基础设施层
- [x] 项目脚手架与构建系统
- [ ] Noncopyable / Timestamp / 线程封装
- [ ] 双缓冲异步日志系统

### 阶段二：网络库层
- [ ] Buffer 应用层缓冲区
- [ ] Channel + EpollPoller
- [ ] EventLoop 事件循环
- [ ] TimerQueue 定时器（基于 timerfd）
- [ ] EventLoopThreadPool 线程池
- [ ] TcpServer / TcpConnection

### 阶段三：HTTP 框架层
- [ ] HttpRequest / HttpResponse / HttpContext
- [ ] HttpServer
- [ ] 路由系统（静态 + 动态）
- [ ] 中间件链
- [ ] 会话管理

### 阶段四：进阶特性
- [ ] HTTPS（OpenSSL 集成）
- [ ] MySQL 连接池
- [ ] 业务示例 Demo
- [ ] 性能压测与调优

## 📊 性能指标

> 待项目完成后补充压测数据（计划使用 wrk / webbench）

## 📚 参考资料

- 陈硕《Linux 多线程服务端编程：使用 muduo C++ 网络库》
- 游双《Linux 高性能服务器编程》
- [muduo 网络库源码](https://github.com/chenshuo/muduo)
- 《C++ Primer》第 5 版

## 📄 开源协议

本项目采用 [MIT License](LICENSE) 开源。

## 👤 作者

**limm-max** - [GitHub](https://github.com/limm-max)

---

> 本项目是个人学习成长项目，欢迎 Issue 和 PR！