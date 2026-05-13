// Poller.cc
#include "Poller.h"
#include "Channel.h"

namespace lim
{

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop)
{
}

bool Poller::hasChannel(Channel* channel) const
{
    // 在 channels_ 里查找
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

} // namespace lim