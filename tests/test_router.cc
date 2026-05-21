// test_router.cc
//
// 演示 Router 4 种用法:
//   1. 静态 + 回调:  GET /         → "Hello, World!"
//   2. 静态 + 回调:  GET /about    → "lim HTTP server"
//   3. 静态 + Handler:GET /login   → 用对象封装的处理器
//   4. 动态 + 回调:  GET /user/:id → 提取 id 返回
//
// 测法:
//   curl http://127.0.0.1:8888/
//   curl http://127.0.0.1:8888/about
//   curl http://127.0.0.1:8888/login
//   curl http://127.0.0.1:8888/user/42
//   curl http://127.0.0.1:8888/user/lim
//   curl http://127.0.0.1:8888/notfound       (应该 404)
#include "HttpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "RouterHandler.h"

#include <iostream>

using namespace lim;

// ─── Handler 对象示例:LoginHandler ──────────────
// 实际业务里会有 queryUser / verifyPassword 等私有方法
class LoginHandler : public RouterHandler
{
public:
    void handle(const HttpRequest& req, HttpResponse* resp) override
    {
        std::cout << "[LoginHandler] hit\n";
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        std::string body = "Login page (handler object)\n";
        resp->setContentLength(body.size());
        resp->setBody(body);
    }

private:
    // bool queryUser(...) { ... }
    // std::string generateCookie(...) { ... }
};

int main()
{
    EventLoop loop;
    InetAddress addr(8888);
    HttpServer server(&loop, addr, "limHttpServer");

    // ─── 路由 1:静态 + 回调 ──────────
    server.Get("/", [](const HttpRequest&, HttpResponse* resp){
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        std::string body = "Hello, World!\n";
        resp->setContentLength(body.size());
        resp->setBody(body);
    });

    // ─── 路由 2:静态 + 回调 ──────────
    server.Get("/about", [](const HttpRequest&, HttpResponse* resp){
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        std::string body = "lim HTTP server, Day 14\n";
        resp->setContentLength(body.size());
        resp->setBody(body);
    });

    // ─── 路由 3:静态 + Handler 对象 ──────────
    server.Get("/login", std::make_shared<LoginHandler>());

    // ─── 路由 4:动态路由 ──────────
    // GET /user/:id  匹配 /user/42、/user/lim 等
    server.addRoute(HttpRequest::kGet, "/user/:id",
        [](const HttpRequest& req, HttpResponse* resp){
            std::string id = req.getPathParameter("param1");
            std::cout << "[/user/:id] id = " << id << "\n";

            resp->setStatusCode(HttpResponse::k200Ok);
            resp->setStatusMessage("OK");
            resp->setContentType("text/plain");
            std::string body = "Hello, user " + id + "!\n";
            resp->setContentLength(body.size());
            resp->setBody(body);
        });

    // ─── 路由都没命中 → 默认 httpCallback (404) ──────────
    server.setHttpCallback([](const HttpRequest& req, HttpResponse* resp){
        std::cout << "[404] " << req.path() << "\n";
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setContentType("text/plain");
        resp->setCloseConnection(true);
        std::string body = "404: " + req.path() + " not found\n";
        resp->setContentLength(body.size());
        resp->setBody(body);
    });

    server.setThreadNum(2);

    std::cout << "HttpServer running at http://127.0.0.1:8888/\n";
    std::cout << "Test:\n";
    std::cout << "  curl http://127.0.0.1:8888/\n";
    std::cout << "  curl http://127.0.0.1:8888/about\n";
    std::cout << "  curl http://127.0.0.1:8888/login\n";
    std::cout << "  curl http://127.0.0.1:8888/user/42\n";
    std::cout << "  curl http://127.0.0.1:8888/notfound\n";

    server.start();
    loop.loop();
    return 0;
}