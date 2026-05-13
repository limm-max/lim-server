// EPollPoller.cc
#include "EPollPoller.h"
#include "Channel.h"
#include "Logger.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

namespace lim
{

// Channel 在 EPollPoller 内的三种状态
//   kNew    : Channel 从未加入过 epoll,channels_ 也没记录
//   kAdded  : Channel 已在 epoll 中,channels_ 有记录
//   kDeleted: 从 epoll 暂时摘掉,但 channels_ 仍有记录(以便快速复用)
const int kNew     = -1;  // 与 Channel 默认 index_ 一致
const int kAdded   = 1;
const int kDeleted = 2;

// ------------------- 构造 / 析构 -------------------

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL << "epoll_create error:" << errno;
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// ------------------- poll：调用 epoll_wait -------------------

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    // 真正调用 epoll_wait
    //   events_.data() 是 vector 底层数组指针
    //   numEvents 是就绪的 fd 数量
    LOG_INFO << "fd total count:" << channels_.size();
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);

    int savedErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);

        // 如果就绪事件填满了整个 events_,下次扩容一倍
        // 自适应大小,避免初期浪费、高峰期不够用
        if (numEvents == static_cast<int>(events_.size()))
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG << "timeout!";  // 超时,无事件
    }
    else
    {
        // 错误处理:EINTR 是被信号打断,不算真正错误
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR << "EPollPoller::poll() err!";
        }
    }
    return now;
}

// ------------------- fillActiveChannels：填充输出列表 -------------------

void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList* activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        // ⭐ 通过 data.ptr 直接拿到 Channel 指针(注册时存进去的)
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);  // Poller 设置 revents_
        activeChannels->push_back(channel);
    }
}

// ------------------- updateChannel -------------------

void EPollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index();
    LOG_INFO << "fd=" << channel->fd()
             << " events=" << channel->events()
             << " index=" << index;

    if (index == kNew || index == kDeleted)
    {
        // 第一次注册,或者从 kDeleted 恢复
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;  // 加入 channels_ 映射
        }
        // 如果是 kDeleted,channels_ 里仍有记录,不需要重新加

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else  // kAdded:已注册,只是修改关心事件
    {
        int fd = channel->fd();
        (void)fd;  // 避免 release 模式 unused 警告
        if (channel->isNoneEvent())
        {
            // 如果不关心任何事件了,从 epoll 摘掉(但 channels_ 保留)
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// ------------------- removeChannel -------------------

void EPollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    channels_.erase(fd);  // 从 map 删

    LOG_INFO << "fd=" << fd;

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);  // 状态回到初始
}

// ------------------- update：真正调用 epoll_ctl -------------------

void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    ::bzero(&event, sizeof event);

    int fd = channel->fd();
    event.events   = channel->events();
    event.data.fd  = fd;
    event.data.ptr = channel;  // ⭐ 把 Channel 指针存进去,epoll_wait 时直接拿到

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR << "epoll_ctl del error:" << errno;
        }
        else
        {
            LOG_FATAL << "epoll_ctl add/mod error:" << errno;
        }
    }
}

} // namespace lim