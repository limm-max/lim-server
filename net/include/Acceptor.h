// Acceptor.h
//重点就是对listenfd的管理
//   监听某个端口,等新连接到来,accept 拿到 connfd,
//   然后通过回调把 connfd 抛给上层 (TcpServer)。

#pragma once

#include "Noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include<functional>

namespace lim
{
class EventLoop;
class InetAddress;

//监听listenfd也会使用channel来管理它的事件，并注册事件相对应的回调函数
//一旦又accept成功，通过回调函数选择线程来处理新连接
class Acceptor : Noncopyable
{
public:
    using NewConnectionCallback=
        std::function<void(int sockfd,const InetAddress&)>;
    
    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
    ~Acceptor();

    //上层的TcpServer可以注册新的连接回调函数
    void setNewConnectionCallback(NewConnectionCallback cb){newConnectionCallback_=cb;};

    bool listening() const{return listening_;} 

    // 真正开始监听, (不仅是使用socket的方法调用 ::listen +，
    //并将把 acceptChannel_ 挂到 epoll对listen上是否由新连接事件（可读事件）进行监听)
    // 必须在 loop_ 所属线程调用
    void listen();  
private:
    // acceptChannel_ 的读回调 —— 这是 Acceptor 的核心入口
    // 由 EventLoop 在 listenfd 可读时调用
    //handleread用于注册channel的可读事件回调函数
    //NewConnectionCallback
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;

    bool listening_;
    int idleFd_;    // /dev/null 的 fd,用于 EMFILE 应急
};
}//namespace lim