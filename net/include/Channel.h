// Channel.h
// 事件分发器：包装一个 fd + 它关心的事件 + 事件到来时的回调
// 跟 muduo/kama 对齐
#pragma once

#include "Noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

namespace lim
{

// 前置声明：避免循环依赖
// Channel 只持有 EventLoop 指针，不需要看到 EventLoop 的完整定义
class EventLoop;

// Channel 翻译成"通道"，封装了 sockfd 和它感兴趣的事件，
// 比如 EPOLLIN、EPOLLOUT，还绑定了 Poller 返回的具体事件
class Channel : public Noncopyable
{
public:
    // 普通事件的回调类型：void(void)
    using EventCallback = std::function<void()>;
    // 读事件的回调类型：带一个时间戳（事件发生时间）
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // ⭐ 核心方法：Poller 检测到事件后调用，内部根据 revents_ 分发到对应回调
    void handleEvent(Timestamp receiveTime);

    // 设置回调（move 进来，避免拷贝）
    void setReadCallback(ReadEventCallback cb)  { readCallback_  = std::move(cb); }
    void setWriteCallback(EventCallback cb)     { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb)     { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb)     { errorCallback_ = std::move(cb); }

    // 防止 Channel 被手动 remove 后还在执行回调（用 shared_ptr 弱引用做保护）
    void tie(const std::shared_ptr<void>& obj);

    int  fd() const         { return fd_; }
    int  events() const     { return events_; }
    void set_revents(int r) { revents_ = r; }   // Poller 使用

    // 修改 events_ 并通知 Poller 同步到 epoll
    void enableReading()    { events_ |= kReadEvent;  update(); }
    void disableReading()   { events_ &= ~kReadEvent; update(); }
    void enableWriting()    { events_ |= kWriteEvent; update(); }
    void disableWriting()   { events_ &= ~kWriteEvent; update(); }
    void disableAll()       { events_ = kNoneEvent;   update(); }

    // 状态查询
    bool isNoneEvent() const  { return events_ == kNoneEvent; }
    bool isReading() const    { return events_ & kReadEvent;  }
    bool isWriting() const    { return events_ & kWriteEvent; }

    // Poller 用：标记 Channel 在 Poller 内部的状态（kNew/kAdded/kDeleted）
    int  index() const       { return index_; }
    void set_index(int idx)  { index_ = idx;  }

    // 返回所属 EventLoop
    EventLoop* ownerLoop() { return loop_; }

    // 把 Channel 从所属 EventLoop 中删除
    void remove();

private:
    // 内部：通知 EventLoop 把本 Channel 的状态同步到 Poller（实际调 epoll_ctl）
    void update();
    // handleEvent 的真正实现，被 tie 保护后才调
    void handleEventWithGuard(Timestamp receiveTime);

    // 三个事件位掩码常量（Channel.cc 里定义）
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;    // 所属 EventLoop
    const int  fd_;      // 包装的 fd（Channel 不拥有它）
    int        events_;  // 关心的事件
    int        revents_; // Poller 返回的实际事件
    int        index_;   // 在 Poller 中的状态（默认 kNew = -1）

    // 用于延长被监听对象的生命周期（用 weak_ptr 弱引用 TcpConnection）
    // 防止 handleEvent 执行时 TcpConnection 已被销毁
    std::weak_ptr<void> tie_;
    bool                tied_;

    // 四个回调
    ReadEventCallback readCallback_;
    EventCallback     writeCallback_;
    EventCallback     closeCallback_;
    EventCallback     errorCallback_;
};

} // namespace lim