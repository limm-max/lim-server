// TcpConnection.h
//   1. 从 connfd 读数据,触发业务的 onMessage
//   2. 业务调 send 时,写到 connfd 或暂存到 outputBuffer
//   3. 用状态机管理连接生命周期 (Connecting / Connected / Disconnecting / Disconnected)
//   4. 优雅关闭 / 强制关闭 / 半关闭
//   5. 错误处理

#pragma once

#include "Noncopyable.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"
#include "Timestamp.h"
 
#include <memory>
#include <atomic>
#include <string>

namespace lim
{
class EventLoop;
class Socket;
class Channel;

class TcpConnection:Noncopyable,
                    public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,      //负责监听这个连接connfd事件的loop，属于子线程
                  const std::string& name,
                  int sockfd,       //连接对应的connfd
                  const InetAddress& localAddr,      //本端地址
                  const InetAddress& peerAddr);       //对端地址，由listenfd的accept获得
    ~TcpConnection();

    //——————————基本信息查询——————————————————————————
    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const {return name_;}
    const InetAddress& localAddress() const {return localAddr_;}
    const InetAddress& peerAddress() const {return peerAddr_;}
    bool connected()    const {return state_==kConnected;}

    // ─── 业务调用的接口 ─────────────────
    //发送数据
    void send(const std::string& message);
    void send(const void* data,size_t len);

    //关闭
    void shutdown();    //关闭写端
    void forceClose();  //强制关闭

    //TCP选项设置
    void setTcpNoDelay(bool on);

    // ───—————— 业务注册回调 ─────────────
    //回调函数类型定义在callback中
    void setConnectionCallback(ConnectionCallback cb)
    {connectionCallback_=std::move(cb);}
    //收到信息时要read的时候调用
    void setMessageCallback(MessageCallback cb)
    { messageCallback_ = std::move(cb); }

    void setWriteCompleteCallback(WriteCompleteCallback cb)
    { writeCompleteCallback_ = std::move(cb); }

    void setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t highWaterMark)
    { highWaterMarkCallback_ = std::move(cb); highWaterMark_ = highWaterMark; }

    //连接真的关闭时调用。TcpServer 内部用,业务不调
    void setCloseCallback(CloseCallback cb)
    { closeCallback_ = std::move(cb); }


    //——————————TcpServer调用的连接建立与销毁——————————————————————
    // 必须在 loop_ 线程调用
    // 连接建立完成: 注册 channel 到 epoll,触发 ConnectionCallback
    void connectEstablished();
    // destroy是让connection在本线程内的channel、epoll中清除
    //closecallback调回主线程，主线程处理完之后又调回子线程进行connectdestroy
    void connectDestroyed();

private:
        // ————— 状态机 ──────
        enum StateE
        {
            kDisconnected,  
            kConnecting,    //TCP 已连接,但框架内部初始化还没完成
            kConnected,     //一切就绪
            kDisconnecting,    
        };

        void setState(StateE s) {state_=s;}

        // ─── Channel 事件回调 (4 个) ───────────────────
        //事件回调
        void handleRead(Timestamp receiveTime);
        void handleWrite();
        void handleClose();
        void handleError();

        // ─── send 的内部实现 (一定在 loop 线程) ──────────────
        void sendInLoop(const void* data, size_t len);
        void shutdownInLoop();
        void forceCloseInLoop();


        // ─── 成员变量 ────────────────────────────────────────────
        EventLoop* loop_;                       // 所属 subLoop
        const std::string name_;                // 连接名
        std::atomic<int> state_;                // 状态机 (atomic 支持多线程读)
        bool reading_;                          // channel 是否在 reading

        std::unique_ptr<Socket>  socket_;       // 包 connfd
        std::unique_ptr<Channel> channel_;      // 监听 connfd 事件
        const InetAddress        localAddr_;
        const InetAddress        peerAddr_;

        //业务回调
        ConnectionCallback    connectionCallback_;
        MessageCallback       messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
        CloseCallback         closeCallback_;
        size_t                highWaterMark_;   // 触发 HWM 的阈值

        Buffer inputBuffer_;       // 收: ::read 后数据进这里
        Buffer outputBuffer_;      // 发: 没发完的数据暂存这里

};
}//namespace lim