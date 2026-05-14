// SocketsOps.h
//工厂函数，负责创建listenfd，使用sockets命名空间
//并封装两个操作fd的函数,为了能在fd没有让socket管理的时候也能对其操作

#pragma once

#include<arpa/inet.h>
#include<sys/socket.h>

namespace lim
{
namespace sockets
{
   // 创建一个非阻塞的 TCP socket fd
    // SOCK_NONBLOCK + SOCK_CLOEXEC 一次设置,避免 fcntl 多次系统调用
    // 失败直接 LOG_FATAL 终止 
    int createNonblockingOrDie(sa_family_t family);

    void close(int sockfd);

    void shutDownWrite(int sockfd);
}// namespace sockets
}// namespace lim