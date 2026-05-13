// EPollPoller.h
// 基于 Linux epoll 的 Poller 实现
// 使用 LT (Level Triggered) 模式
#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

namespace lim
{

class Channel;

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    // 重写基类纯虚函数 ----------
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    // 初始 events_ 数组大小（动态扩容）
    static const int kInitEventListSize = 16;

    // 把就绪事件填到 activeChannels 中
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    // 真正调用 epoll_ctl（被 updateChannel / removeChannel 调用）
    void update(int operation, Channel* channel);

    using EventList = std::vector<struct epoll_event>;

    int       epollfd_;  // epoll_create1 返回的 fd
    EventList events_;   // epoll_wait 的输出缓冲（就绪事件数组）
};

} // namespace lim