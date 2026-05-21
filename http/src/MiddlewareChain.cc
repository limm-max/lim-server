// MiddlewareChain.cc
#include "MiddlewareChain.h"
#include "Logger.h"

namespace lim
{

void MiddlewareChain::processBefore(HttpRequest& request)
{
    // 正向遍历:第 0 个先跑,然后 1, 2, ...
    for (auto& mw : middlewares_)
    {
        mw->before(request);
    }
}

void MiddlewareChain::processAfter(HttpResponse& response)
{
    // 反向遍历:最后一个先跑,然后倒数第二,...
    // 用 rbegin/rend 反向迭代器
    try
    {
        for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it)
        {
            if (*it)   // 空指针保护(防止有人 push 了 nullptr)
            {
                (*it)->after(response);
            }
        }
    }
    catch (const std::exception& e)
    {
        // after 阶段就别抛异常了,出错记日志兜底
        LOG_ERROR << "Error in middleware after: " << e.what();
    }
}

} // namespace lim