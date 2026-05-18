// TcpServer.cc
#include "TcpServer.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "Logger.h"
 
#include <stdio.h>          // snprintf
#include <sys/socket.h>     // getsockname

namespace lim
{

namespace
{
    EventLoop* CheckLoopNotNull(EventLoop* loop)
    {
        if(loop==nullptr)
        {
            LOG_FATAL<< "TcpServer: baseLoop is null";
        }
        return loop;
    }
}

    TcpServer::TcpServer(EventLoop* loop,
                         const InetAddress& listenAddr,
                         const std::string& nameArg,
                         Option option)
        :baseLoop_(CheckLoopNotNull(loop))
        ,ipPort_(listenAddr.toIpPort())
        ,name_(nameArg)
        ,acceptor_(new Acceptor(loop,listenAddr,option==kReusePort))
        ,threadPool_(new EventLoopThreadPool(loop,name_))   //只是创建pool对象，但还没有创建多线程
        ,started_(0)
        ,nextConnId_(1)
    {
        //设置acceptor的新连接回调
        acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,
                                            std::placeholders::_1,std::placeholders::_2));
    }

    TcpServer::~TcpServer()
    {
        //关闭每个connection，
        for(auto& item:connections_)
        {
            TcpConnectionPtr conn(item.second);
            conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));        //获得负责conn的loop对象指针，跑其关闭函数
        }
    }

    void TcpServer::setThreadNum(int numThreads)
    {
        threadPool_->setThreadNum(numThreads);
    }

    void TcpServer::start()
    {
        // 原子计数器防止重复 start
        if(started_++==0)
        {
            threadPool_->start(threadInitCallback_);
            baseLoop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
        }
    }

    //Acceptor 拿到 connfd 后调
    void TcpServer::newConnection(int sockfd,const InetAddress& peerAddr)
    {
        //主线程执行
        baseLoop_->assertInLoopThread();

        //选择一个loop处理（即选择一个子线程）
        EventLoop* ioLoop=threadPool_->getNextLoop();

        //给连接命名
        char buf[64];
        snprintf(buf,sizeof buf, "-%s#%d",ipPort_.c_str(),nextConnId_);
        ++nextConnId_;
        std::string connName=name_+buf; //服务器名+ip端口+连接编号

        LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName
             << "] from " << peerAddr.toIpPort();

        //获取本端地址,用于创建tcpconnection
        sockaddr_in local;
        ::bzero(&local,sizeof local);
        socklen_t addrlen = sizeof local;
        if (::getsockname(sockfd, reinterpret_cast<sockaddr*>(&local), &addrlen) < 0)
        {
            LOG_ERROR << "TcpServer::newConnection getsockname failed";
        }
        InetAddress localAddr(local); 

        //创建TcpConnection
        TcpConnectionPtr conn(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));
        connections_[connName]=conn;

        //设置业务回调
        conn->setConnectionCallback(connectionCallback_);
        conn->setMessageCallback(messageCallback_);
        conn->setWriteCompleteCallback(writeCompleteCallback_);

        //设置connection本身的删除连接
        conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));  // std::placeholders::_1为connection指针

        //通知子线程完成TcpConnection的连接工作
        ioLoop->runInLoop(
            std::bind(&TcpConnection::connectEstablished,conn));
        
    }

    //removeConnection - TcpConnection 关闭时通过 closeCallback 回调
    void TcpServer::removeConnection(const TcpConnectionPtr& conn)
    {
        baseLoop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
    }

    void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
    {
        LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();
        connections_.erase(conn->name());

        // 2. 派回 ioLoop 调 connectDestroyed 完成最后清理
        //    queueInLoop 强制入队,不立即执行
        //    因为我们当前可能在 handleEvent 的调用栈里
        EventLoop* ioLoop = conn->getLoop();
        ioLoop->queueInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}//namepsace lim