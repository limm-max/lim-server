// test_httpserver.cc
//
// 真打一个 HTTP 服务器,跑起来用 curl/浏览器验证。
// 路由(简陋的 if-else 版,Router 是 Day 14 的事):
//   /             → 200, "Hello, World!"
//   /about        → 200, "lim HTTP server"
//   /json         → 200, application/json
//   其他          → 404
//
// 测法:
//   ./test_httpserver
//   另一个终端:
//     curl -v http://127.0.0.1:8888/
//     curl -v http://127.0.0.1:8888/about
//     curl -v http://127.0.0.1:8888/json
//     curl -v http://127.0.0.1:8888/foo

#include "HttpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include <iostream>
#include <string>

using namespace lim;

void onRequest(const HttpRequest& req, HttpResponse* resp)
{
    std::cout << "[请求] " << req.methodString() << " " << req.path() << "\n";

    if (req.path() == "/")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        std::string body = "Hello, World!\n";
        resp->setContentLength(body.size());
        resp->setBody(body);
    }
    else if (req.path() == "/about")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        std::string body = "lim HTTP server, Day 13\n";
        resp->setContentLength(body.size());
        resp->setBody(body);
    }
    else if (req.path() == "/json")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("application/json");
        std::string body = R"({"name":"lim","day":13,"status":"ok"})";
        resp->setContentLength(body.size());
        resp->setBody(body);
    }
    else
    {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setCloseConnection(true);
        std::string body = "404 Not Found\n";
        resp->setContentLength(body.size());
        resp->setBody(body);
    }
}

int main()
{
    EventLoop loop;
    InetAddress addr(8888);
    HttpServer server(&loop, addr, "limHttpServer");

    server.setHttpCallback(onRequest);
    server.setThreadNum(2);   // baseLoop + 2 个 subLoop

    std::cout << "HttpServer running at http://127.0.0.1:8888/\n";
    std::cout << "Try:\n";
    std::cout << "  curl -v http://127.0.0.1:8888/\n";
    std::cout << "  curl -v http://127.0.0.1:8888/about\n";
    std::cout << "  curl -v http://127.0.0.1:8888/json\n";
    std::cout << "  curl -v http://127.0.0.1:8888/foo\n";

    server.start();
    loop.loop();
    return 0;
}