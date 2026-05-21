// MiddlewareChain.h
//
// 管理一组 Middleware,按洋葱模型执行。
// 由 HttpServer 持有,每个请求来时跑一遍。

#pragma once

#include "Middleware.h"

#include <memory>
#include <vector>

namespace lim
{

class MiddlewareChain
{
public:
    void addMiddleware(std::shared_ptr<Middleware> mw)
    { middlewares_.push_back(std::move(mw)); }

    // 请求前:正向遍历所有中间件的 before
    void processBefore(HttpRequest& request);

    // 响应后:反向遍历所有中间件的 after
    void processAfter(HttpResponse& response);

private:
    std::vector<std::shared_ptr<Middleware>> middlewares_;
};

} // namespace lim