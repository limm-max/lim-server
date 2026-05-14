// Socket.cc
#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"
 
#include <netinet/tcp.h>    // TCP_NODELAY
#include <sys/socket.h>     // bind, listen, accept4, setsockopt, shutdown
#include <strings.h>        // bzero
#include <unistd.h>         // close
#include <errno.h>

namespace lim
{
    Socket::~Socket()
    {
        ::close(sockfd_);
    }

    void Socket::bindAddress(const InetAddress& localaddr)
    {
        if(::bind(sockfd_,localaddr.getSockAddr(),sizeof(sockaddr_in))!=0)
        {
            LOG_FATAL<<"bind sockfd:"<<sockfd_<<"fail";
        }
    }

    void Socket::listen()
    {
        if(::listen(sockfd_,SOMAXCONN)!=0)//SOMAXCONN 是系统允许的最大 backlog (通常 128 或 4096)
        {
            LOG_FATAL << "listen sockfd:" << sockfd_ << " fail";
        }     
    }

    //使用accept4函数，accept可以对connfd设置非阻塞（Non-blocking）和执行时关闭（Close-on-exec）标志
    int Socket::accept(InetAddress* peeraddr)
    {
        sockaddr_in addr;
        ::bzero(&addr,sizeof addr);

        socklen_t len=sizeof addr;
        int connfd=::accept4(sockfd_,
                            reinterpret_cast<sockaddr*>(&addr),
                            &len,
                            SOCK_NONBLOCK | SOCK_CLOEXEC);   //&len,内核回填实际地址长度
        if(connfd>=0)
        {
            peeraddr->setSockAddr(addr);
        }
        else
        {
                LOG_ERROR << "accept4 failed, errno=" << errno;
        }
        return connfd;

    }

    void Socket::shutdownWrite()
    {
        if (::shutdown(sockfd_, SHUT_WR) < 0)
        {
            LOG_ERROR << "shutdownWrite error, errno=" << errno;
        }
    }

    void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    // IPPROTO_TCP 层 / TCP_NODELAY 选项 / 禁用 Nagle 算法
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                 &optval, sizeof optval);
}
 
    void Socket::setReuseAddr(bool on)
    {
        int optval = on ? 1 : 0;
        // SOL_SOCKET 层 / SO_REUSEADDR 选项 / 允许复用 TIME_WAIT 端口
        ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                    &optval, sizeof optval);
    }
    
    void Socket::setReusePort(bool on)
    {
        int optval = on ? 1 : 0;
        // SO_REUSEPORT: 多 fd 绑同一端口,内核负载均衡
        int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                            &optval, sizeof optval);
        if (ret < 0 && on)
        {
            LOG_ERROR << "SO_REUSEPORT failed, errno=" << errno;        //新锐选项，需要保存可能出错信息
        }
    }
    
    void Socket::setKeepAlive(bool on)
    {
        int optval = on ? 1 : 0;
        // SO_KEEPALIVE: TCP 心跳保活
        ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
                    &optval, sizeof optval);
    }
}//namespace lim