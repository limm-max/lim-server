// DefaultPoller.cc
// 实现 Poller::newDefaultPoller 工厂方法
// 单独一个文件,避免基类 Poller.cc 直接依赖派生类
#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>  // ::getenv

namespace lim
{

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if (::getenv("LIM_USE_POLL"))
    {
        // 预留:如果设置了 LIM_USE_POLL 环境变量,用 poll 实现
        // 但 lim-server 没实现 PollPoller,所以直接返回 nullptr
        // 真要支持的话,这里 new PollPoller(loop)
        return nullptr;
    }
    else
    {
        return new EPollPoller(loop);
    }
}

} // namespace lim