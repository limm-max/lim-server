// TcpServer.h
// TcpServer 是 lim-server 对外的最高层接口。它把:
//   - Acceptor (Day 6)          : 接受新连接
//   - EventLoopThreadPool (Day 4) : 多线程 subLoop 池
//   - TcpConnection (Day 9)     : 单个连接管理
// 三个组件粘合成一个完整的 TCP 服务器。

// 用户用 TcpServer 写服务器只需要:
//   ① 创建 TcpServer 实例
//   ② 设置业务回调 (ConnectionCallback / MessageCallback)
//   ③ 设置线程数
//   ④ start()
//   ⑤ baseLoop.loop()

//主从Reactor模型的完整体现
//   主线程                      子线程池
//   ────────                    ────────
//   baseLoop                    subLoop1, subLoop2, ...
//      ↓                            ↓
//   Acceptor                    各管几个 TcpConnection
//      ↓ newConnection            ↑
//   TcpServer (主):               │
//      • connections_ map          │
//      • 从 pool 取 subLoop  ───── 派 TcpConnection 给某 subLoop
//      • runInLoop connectEstablished

#pragma once
 
#include "Noncopyable.h"
#include "Callbacks.h"
#include "EventLoopThreadPool.h"
#include "Acceptor.h"
#include "InetAddress.h"
 
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace lim
{

class EventLoop;

class TcpServer:Noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;   //线程初始化设置函数

    //选项设置，端口是否复用
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
              const InetAddress& listenAddr,
              const std::string& nameArg,
              Option option=kNoReusePort);
    ~TcpServer();

    //——————信息查询——————————————————
    const std::string& ipPort() const {return ipPort_;}     //监听的本地ipPort
    const std::string& name() const {return name_;}
    EventLoop* getLoop() const {return baseLoop_;}

    // 设置 IO 线程数 (subLoop 数量)
    //   0 = 退化模式,所有连接都在 baseLoop (主线程)
    //   N = N 个子线程
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb)
    {threadInitCallback_=cb;}


    // ─── 业务注册回调 ─────────────────────────
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }
 
    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
 
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }


    //启动服务器
    void start();
private:
    // Acceptor 接到新连接的回调 (在主线程调),相当于Acceptor里的newConnectionCallback_;
    void newConnection(int sockfd, const InetAddress& peerAddr);

    // TcpConnection 关闭时调 (从 subLoop 调过来)
    // 派回主线程执行 removeConnectionInLoop
    //相当于Tcpconnection里的closeCallback_
    void removeConnection(const TcpConnectionPtr& conn);
    // 真正从 map 删除连接 (主线程)
    void removeConnectionInLoop(const TcpConnectionPtr& conn);


    EventLoop* baseLoop_;
    const std::string ipPort_;   // "0.0.0.0:8888" 这样的字符串
    const std::string name_;    // 服务器名,如 "EchoServer"

    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;   //从线程让从线程对象来管理

    std::atomic<int> started_;    // 防止重复 start
    int              nextConnId_; // 自增 ID,只在主线程访问,不用锁
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    ConnectionMap    connections_;  //已经建立的TcpConnection字典

    //——————业务回调，上接应用层————————————
    ConnectionCallback    connectionCallback_;
    MessageCallback       messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback    threadInitCallback_;

};
}//namespace lim