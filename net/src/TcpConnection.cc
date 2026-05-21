// TcpConnection.cc
#include "TcpConnection.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"
#include "Logger.h"
 
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>     // memcpy

namespace lim
{

namespace
{
    EventLoop* CheckLoopNotNull(EventLoop* loop)
    {
        if(loop==nullptr)
        {
            LOG_FATAL << "TcpConnection loop is null";
        }
        return loop;
    }
}//anonymous namespace
    TcpConnection::TcpConnection(EventLoop* loop,
                             const std::string& name,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
        :loop_(CheckLoopNotNull(loop))
        ,name_(name)
        , state_(kConnecting)
        , reading_(true)                                 //读端是否开启
        , socket_(new Socket(sockfd))                    // 接管 connfd
        , channel_(new Channel(loop, sockfd))            // 监听 connfd
        , localAddr_(localAddr)
        , peerAddr_(peerAddr)
        , highWaterMark_(64 * 1024 * 1024)  
    {
        //事件回调注册
        channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
        channel_->setWriteCallback(
            std::bind(&TcpConnection::handleWrite, this));
        channel_->setCloseCallback(
            std::bind(&TcpConnection::handleClose, this));
        channel_->setErrorCallback(
            std::bind(&TcpConnection::handleError, this));
        
        LOG_INFO << "TcpConnection::ctor[" << name_ << "] fd=" << sockfd;

        socket_->setKeepAlive(true);    //设置tcp心跳保活

    }

    TcpConnection::~TcpConnection()
    {
        LOG_INFO << "TcpConnection::dtor[" << name_ << "] fd=" << channel_->fd()
                << " state=" << static_cast<int>(state_);
    }


    // 业务接口: send / shutdown / forceClose / setTcpNoDelay
    //string接口，实际的send必须在本loop
    void TcpConnection::send(const std::string& message)
    {
        send(message.data(), message.size());
    }

    //判断是否本线程
    void TcpConnection::send(const void* data, size_t len)
    {
        if (state_ == kConnected)
        {
            if (loop_->isInLoopThread())
            {
                sendInLoop(data, len);
            }
            else
            {
                // 跨线程: 派到 loop_ 线程执行
                // 这里要注意: data 是临时的,跨线程时需要拷贝
                std::string str(static_cast<const char*>(data), len);
                loop_->runInLoop(
                    [self = shared_from_this(), str = std::move(str)]() {
                        self->sendInLoop(str.data(), str.size());
                    });
            }
        }
    }

    //真正的send函数
    void TcpConnection::sendInLoop(const void* data,size_t len)
    {
        ssize_t nwrote=0;   //已写入（发送）长度
        size_t remaining=len;   //未发送长度
        bool faultError=false;

        if(state_==kDisconnected)
        {
            LOG_WARN << "disconnected, give up writing";
            return;
        }

        //————outputbuffer_空，即前面数据已发完+channel没在writeing——————————
        //直接write，跳过buffer
        if(!channel_->isWriting() && outputBuffer_.readableBytes()==0)
        {
            nwrote=::write(channel_->fd(),data,len);
            if(nwrote>=0)
            {
                remaining-=nwrote;
                if(remaining==0 && writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
            }
            else    //出错了，恢复nwrote
            {
                nwrote=0;
                if(errno!=EWOULDBLOCK)  //如果不是非阻塞，而是真的出错
                {
                    LOG_ERROR << "TcpConnection::sendInLoop";
                    if (errno == EPIPE || errno == ECONNRESET)  //如果是出现连接关闭/被重置的错误
                    {
                        faultError = true;   // 严重错误
                    }
                }
            }
        }

        //如果是outpubuffer_非空、前面的write没写完、前面的write非阻塞，将数据放在buffer里等待write事件发生处理
        if(!faultError && remaining>0)
        {
            size_t oldlen=outputBuffer_.readableBytes();

            //如果buffer挤压的未写数据超过阈值，需要触发callback
            if(oldlen+remaining>=highWaterMark_ && oldlen<highWaterMark_ && highWaterMarkCallback_)
            {
                loop_->queueInLoop(
                        std::bind(highWaterMarkCallback_,shared_from_this(),oldlen+remaining));
            }

            outputBuffer_.append(static_cast<const char*>(data)+nwrote,remaining);

            //确保channel注册了写事件
            if(!channel_->isWriting())
            {
                channel_->enableWriting();
            }
        }
    }


    //半关闭，关闭写端。
    void TcpConnection::shutdown()
    {
        if(state_==kConnected)
        {
            setState(kDisconnecting);
            loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,shared_from_this()));
        }
    }

    // 只有 outputBuffer_ 空了才能真关
    // 如果 channel 还在 writing,说明 outputBuffer 还有数据,等 handleWrite 发完
    //shutdownInLoop只会在本线程内被调用
    void TcpConnection::shutdownInLoop()
    {
        
        if(!channel_->isWriting())  //没有isWriting，说明不再等写事件——outputbuffer没有数据了
        {
            socket_->shutdownWrite();
        }
    }

    //强制关闭，不管outputbuffer是否还有未发数据
    void TcpConnection::forceClose()
    {
        if(state_==kConnected || state_==kDisconnecting)
        {
            setState(kDisconnecting);
            loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop,shared_from_this()));
        }
    }

    void TcpConnection::forceCloseInLoop()  //执行的时候需要再一次确定状态
    {
        if(state_ == kConnected || state_ == kDisconnecting)
        {
            handleClose();
        }
    }

    void TcpConnection::setTcpNoDelay(bool on)
    {
        socket_->setTcpNoDelay(on);
    }

    //上层TcpServer调用，用于完成Tcpconnection的连接/断开
    //连接：切换转台，这侧channel，通知业务
    void TcpConnection::connectEstablished()
    {
        setState(kConnected);
        channel_->tie(shared_from_this());  //为了事件回调函数防野指针
        channel_->enableReading();

        //通知业务
        connectionCallback_(shared_from_this());
    }

    void TcpConnection::connectDestroyed()
    {
        if(state_==kConnected)
        {
            setState(kDisconnected);
            channel_->disableAll();

            connectionCallback_(shared_from_this());
        }
        channel_->remove(); //别忘了remove
    }


    //————————事件回调函数————————————————————
    void TcpConnection::handleRead(Timestamp receiverTime)
    {
        int savedErrno=0;
        ssize_t n=inputBuffer_.readFd(channel_->fd(),&savedErrno);

        //读到数据了，触发业务callback
        if(n>0)
        {
            messageCallback_(shared_from_this(),&inputBuffer_,receiverTime);
        }
        else if(n==0)   //read 返回 0 是 TCP FIN 的标志，对端关闭，则这端的连接也要关闭
        {
            handleClose();
        }
        else    //出错了
        {
            errno = savedErrno;
            LOG_ERROR << "TcpConnection::handleRead";
            handleError();
        }
    }

    void TcpConnection::handleWrite()
    {
        //判断写端是否还开着
        if(channel_->isWriting())
        {
            int savedErrno=0;
            //能写就一次性写出去（也有可能还是写不完）
            ssize_t n=::write(channel_->fd(),outputBuffer_.peek(),outputBuffer_.readableBytes());
            if(n>0)
            {
                outputBuffer_.retrieve(n);  //手动消费的，所以需要手动①
                if(outputBuffer_.readableBytes()==0)
                {
                    channel_->disableWriting(); //缓冲区没有需要写出去得了，所以不需要等写信号了
                    if(writeCompleteCallback_)
                    {
                        loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));   //放入下次处理，避免嵌套回调
                    }
                    // 如果状态是 kDisconnecting,说明业务调过 shutdown,数据发完了真正关闭
                    if (state_ == kDisconnecting)
                    {
                        shutdownInLoop();
                    }
                }
            }
            else    //n<=0写出错
            {
                LOG_ERROR << "TcpConnection::handleWrite";
            }
        }
        else    //写端关闭，不能再写
        {
            LOG_ERROR << "TcpConnection fd=" << channel_->fd()
                  << " is down, no more writing";
        }

    }


    void TcpConnection::handleClose()
    {
        LOG_INFO << "TcpConnection::handleClose fd=" << channel_->fd()
             << " state=" << static_cast<int>(state_);
        setState(kDisconnected);
        channel_->disableAll();
    
        TcpConnectionPtr guardThis(shared_from_this());
        connectionCallback_(guardThis);   // 通知业务: 我断了
        closeCallback_(guardThis);         // 通知 TcpServer:已经关闭了， 把我从 map 删掉
    }

    // handleError: 出错事件 (SO_ERROR 拿出来打日志)
    void TcpConnection::handleError()
    {
        int optval;
        socklen_t optlen = sizeof optval;
        int err = 0;
        if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)   //获取socket错误信息
        {
            err = errno;    //getsockopt本身调用错误
        }
        else
        {
            err = optval;
        }
        // LOG_ERROR << "TcpConnection::handleError [" << name_
        //         << "] - SO_ERROR=" << err;
        if (err == ECONNRESET || err == ETIMEDOUT) {
            LOG_INFO << "TcpConnection::handleError [...] - peer reset, ignored";
        } else {
            LOG_ERROR << "TcpConnection::handleError [...] - SO_ERROR=" << err;
        }
    }

}//namespace lim