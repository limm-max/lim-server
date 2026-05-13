// Poller.h
// IO 多路复用的抽象基类。
// 派生类实现 epoll/poll/select 等具体机制（lim-server 只实现 EPollPoller）
#pragma once

#include "Noncopyable.h"
#include "Timestamp.h"

#include <unordered_map>
#include <vector>

namespace lim
{

class Channel;
class EventLoop;

// muduo demo 的接口设计：Poller 不持有 Channel 的所有权，只是观察它们
class Poller : public Noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    // ⭐ 核心接口：调用一次 epoll_wait（或 select/poll）
    //   timeoutMs: 阻塞毫秒数（-1 永久阻塞、0 立即返回）
    //   activeChannels: 输出参数，有事件的 Channel 列表
    //   返回值：本次返回的时刻（Timestamp）
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    // 增加或修改一个 Channel 在 Poller 中的状态
    virtual void updateChannel(Channel* channel) = 0;

    // 从 Poller 中删除一个 Channel
    virtual void removeChannel(Channel* channel) = 0;

    // 判断某 Channel 是否在本 Poller 中（基类提供默认实现）
    virtual bool hasChannel(Channel* channel) const;

    // 工厂方法：根据环境变量或默认值创建具体 Poller
    // 实现在 DefaultPoller.cc（避免基类依赖派生类头文件）
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    // 子类共用的"fd → Channel*"映射
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

private:
    EventLoop* ownerLoop_;  // 本 Poller 属于哪个 EventLoop
};

} // namespace lim