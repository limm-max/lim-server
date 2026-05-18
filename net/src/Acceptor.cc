#// Acceptor.cc
#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "Logger.h"
 
#include <fcntl.h>      // open
#include <unistd.h>     // close
#include <errno.h>

namespace lim
{

    Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport)
        :loop_(loop)
        ,acceptSocket_(sockets::createNonblockingOrDie(AF_INET))
        ,acceptChannel_(loop,acceptSocket_.fd())
        ,listening_(false)
        ,idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
    {
        // /dev/null 几乎不可能打开失败,但还是检查一下
        if (idleFd_ < 0)
        {
            LOG_FATAL << "Acceptor::Acceptor open /dev/null fail";
        }

        //对listenfd设置选项,并绑定端口ip
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.setReusePort(reuseport);
        acceptSocket_.bindAddress(listenAddr);

        //注册acceptchannel的回调函数
        acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
    }

    //关闭channel和idleFd_，listenfd由acceptsocket_负责close
    Acceptor::~Acceptor()
    {
        acceptChannel_.disableAll();   // 不再关心任何事件
        acceptChannel_.remove();       // 从 Poller 里摘掉
        ::close(idleFd_);
    }

    // 必须在 loop_ 所属线程调用 (assertInLoopThread)
    //    因为 enableReading → update → poller->updateChannel 是非线程安全的
    void Acceptor::listen()
    {
        loop_->assertInLoopThread();
        listening_=true;
        acceptSocket_.listen();
        //channel的enableReading ➡loop的updateChannel➡poller的updateChannel➡系统调用注册到epoll中
        acceptChannel_.enableReading(); 
    }

    // 由 acceptChannel_ 的可读事件触发。
    //可读事件就是listenfd的accept队列有socket TCP连接对象了
    //这时候需要accept拿走connfd
    //这里的回调函数就是决定这个connfd被调度给哪个线程进行处理
    //如果没有线程处理，那就直接close
    void Acceptor::handleRead()
    {
        loop_->assertInLoopThread();
        InetAddress peerAddr;
        int connfd=acceptSocket_.accept(&peerAddr);

        //accept成功
        if(connfd>=0)
        {
            //  通过回调把 connfd 抛给上层
            if (newConnectionCallback_)
            {
                newConnectionCallback_(connfd, peerAddr);
            }
            else
            {
                // 上层没注册回调,直接 close 这个新连接,避免泄漏
                sockets::close(connfd);
            }
        }
        else
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
                LOG_ERROR << "Acceptor::handleRead accept failed, errno=" << errno;
            }
            //如果是因为fd配额满了,accept 持续失败,但 listenfd 仍可读
            //使用一个"/dev/null"中间fd作缓冲，持续关闭并打开再关闭
            //即优雅关闭该客户连接，直到有新配额。这样服务器不会 busy loop
            if (errno == EMFILE)
            {
                ::close(idleFd_);
                idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
                ::close(idleFd_);
                idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            }
        }
    }
}//namespace lim