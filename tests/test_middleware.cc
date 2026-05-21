// test_middleware.cc
//
// 演示中间件用法:
//   1. 自己写一个简单的 LoggingMiddleware
//   2. 加上 CorsMiddleware
//   3. 注册 / 路由
//
// 测法:
//   curl http://127.0.0.1:8888/         (看日志 + CORS header)
//   curl -X OPTIONS http://127.0.0.1:8888/ -H "Origin: http://example.com"
//   curl -H "Origin: http://example.com" http://127.0.0.1:8888/

#include "HttpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Middleware.h"
#include "CorsMiddleware.h"

#include <iostream>
#include <chrono>

using namespace lim;

// 自定义中间件:打印请求和响应耗时
class LoggingMiddleware : public Middleware
{
public:
    void before(HttpRequest& req) override
    {
        startTime_ = std::chrono::steady_clock::now();
        std::cout << "[LOG] → " << req.methodString() << " " << req.path() << "\n";
    }

    void after(HttpResponse& resp) override
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - startTime_).count();
        std::cout << "[LOG] ← " << resp.statusCode() << " (" << elapsed << "us)\n";
    }

private:
    std::chrono::steady_clock::time_point startTime_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8888);
    HttpServer server(&loop, addr, "limHttpServer");

    // ─── 加中间件(顺序很重要)──
    server.addMiddleware(std::make_shared<LoggingMiddleware>());  // 外层
    server.addMiddleware(std::make_shared<CorsMiddleware>());     // 内层

    // ─── 注册路由 ──
    server.Get("/", [](const HttpRequest&, HttpResponse* resp){
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        std::string body = "Hello with middleware!\n";
        resp->setContentLength(body.size());
        resp->setBody(body);
    });

    server.setThreadNum(2);
    std::cout << "Server running at http://127.0.0.1:8888/\n";
    std::cout << "Try: curl -H 'Origin: http://x.com' http://127.0.0.1:8888/\n";
    server.start();
    loop.loop();
}