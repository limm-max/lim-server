// Middleware.h
//
// 中间件基类。每个具体中间件继承它,实现 before / after。
//在每个请求来之后，根据具体需求，先执行不同的中间件，然后再进行handler
// before:在调用 handler 之前执行 (可以改 req,或抛异常打断)
// after :在 handler 之后执行 (修改 resp,加 header 等)
//
//   1. 中间件按"洋葱"顺序执行 —— before 正向,after 反向
//   2. before 抛出 HttpResponse 可以提前终止 (用于 CORS 预检请求等)
//   3. 中间件之间不直接关联,统一由 MiddlewareChain 管
#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"

namespace lim
{

class Middleware
{
public:
    virtual ~Middleware() = default;

    // 请求前处理:可改 req,也可以抛 HttpResponse 中断流程
    virtual void before(HttpRequest& request) = 0;

    // 响应后处理:可改 resp
    virtual void after(HttpResponse& response) = 0;
};

} // namespace lim