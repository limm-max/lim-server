// Socket.h
// 包装一个 socket fd,提供:
//
//   1. RAII 生命周期管理: 析构时自动 close,杜绝 fd 泄漏
//
//   2. 系统调用的 OOP 门面:
//        sock.bindAddress(addr);   // 内部 ::bind
//        sock.listen();             // 内部 ::listen
//        sock.accept(&peerAddr);    // 内部 ::accept4
//
//   3. 常用 socket 选项的便捷开关:
//        sock.setReuseAddr(true);    // SO_REUSEADDR  端口快速复用
//        sock.setReusePort(true);    // SO_REUSEPORT  内核负载均衡
//        sock.setKeepAlive(true);    // SO_KEEPALIVE  TCP 保活
//        sock.setTcpNoDelay(true);   // TCP_NODELAY   禁用 Nagle
//
//   4. 半关闭 (优雅断开):
//        sock.shutdownWrite();       // 只关写端,读端还能收
//

#pragma once

#include "Noncopyable.h"

namespace lim
{

class InetAddress;
class Socket:Noncopyable
{
public:
    explicit Socket(int sockfd):sockfd_(sockfd){}
    ~Socket();

    int fd() const {return sockfd_;}

    //listenfd系统调用封装
    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);  //peeraddr输出型参数，返回的是连接的fd

    //关闭处理：半关闭，只关闭读端
    void shutdownWrite();

    //常用socket选项封装
    void setTcpNoDelay(bool on);    // TCP_NODELAY  禁用 Nagle,降低小包延迟
    void setReuseAddr(bool on);     // SO_REUSEADDR 允许 bind 处于 TIME_WAIT 的端口(listen监听必须)
    void setReusePort(bool on);     // SO_REUSEPORT 多个 fd 绑同一端口,内核负载均衡，即允许1端口多listenfd
    void setKeepAlive(bool on);     // SO_KEEPALIVE TCP 心跳保活。发送给对端保活探测分节，响应ACK连接才继续
private:
    const int sockfd_;  // 一旦接管就不能换,const 强调这点

};
}//namespace lim