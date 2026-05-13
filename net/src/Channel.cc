// Channel.cc
#include "Channel.h"
#include "EventLoop.h"   // ← 现在要真正用到 EventLoop 的成员了
#include "Logger.h"      // base/ 提供的 LOG_* 宏

#include <sys/epoll.h>   // EPOLLIN/EPOLLOUT/EPOLLPRI/EPOLLHUP/EPOLLERR/EPOLLRDHUP

namespace lim
{

// 三个事件位掩码常量定义（声明在 .h 里）
const int Channel::kNoneEvent  = 0;
const int Channel::kReadEvent  = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// ----------------- 构造 / 析构 -----------------

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)   // -1 即 kNew，"从未加入 Poller"
    , tied_(false)
{
}

Channel::~Channel()
{
    // Channel 析构时,fd 应当已经从 Poller 注销
    // （由更上层的 TcpConnection 保证）
}

// ----------------- tie 防止悬空 -----------------

void Channel::tie(const std::shared_ptr<void>& obj)
{
    // 用 weak_ptr 弱引用 obj（一般是 TcpConnection）
    // handleEvent 调用时会先 lock() 升级成 shared_ptr 延长生命
    tie_  = obj;
    tied_ = true;
}

// ----------------- 让 EventLoop 把本 Channel 同步到 Poller -----------------

void Channel::update()
{
    // 通过所属 EventLoop 调到 Poller 的 updateChannel
    // 真正调 epoll_ctl 是在 Poller 那边
    loop_->updateChannel(this);
}

// ----------------- 从 EventLoop 注销本 Channel -----------------

void Channel::remove()
{
    loop_->removeChannel(this);
}

// ----------------- handleEvent：事件分发的入口 -----------------

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        // 升级 weak_ptr → shared_ptr，期间被监听对象不会销毁
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
        // 如果 guard 是空,说明对象已被销毁,什么都不做(防止 use-after-free)
    }
    else
    {
        // 没 tie 过的 Channel（比如 wakeupChannel、Acceptor 用的）直接调
        handleEventWithGuard(receiveTime);
    }
}

// ----------------- 根据 revents_ 分发到具体回调 -----------------

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO << "channel handleEvent revents:" << revents_;

    // 1) 对端关闭事件（EPOLLHUP 表示连接挂起，且对端无新数据要发）
    //    注意 EPOLLIN 同时被设置时不算关闭——可能还有最后一批数据要读
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_) closeCallback_();
    }

    // 2) 错误事件
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_) errorCallback_();
    }

    // 3) 可读事件（普通数据 EPOLLIN、带外 EPOLLPRI、对端半关闭 EPOLLRDHUP）
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if (readCallback_) readCallback_(receiveTime);
    }

    // 4) 可写事件
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }
}

} // namespace lim