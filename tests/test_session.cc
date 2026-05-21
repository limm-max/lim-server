// test_session.cc
//
// 模拟登录态:
//   POST /login        → 创建 session,种 cookie
//   GET  /profile      → 检查 session,返回用户信息
//   POST /logout       → 销毁 session
//
// 测试:
//   1. curl --cookie-jar cookies.txt -X POST http://127.0.0.1:8888/login 
//        -d 'username=lim&password=secret'
//      (--cookie-jar 让 curl 把响应的 Set-Cookie 存到文件)
//
//   2. curl --cookie cookies.txt http://127.0.0.1:8888/profile
//      (--cookie 把 cookies.txt 里的 cookie 发回去)
//      应该看到:Hello lim, userId=42
//
//   3. curl http://127.0.0.1:8888/profile
//      (不带 cookie)→ 应该 401

#include "HttpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SessionManager.h"
#include "SessionStorage.h"

#include <iostream>

using namespace lim;

HttpServer* g_server = nullptr;

void handleLogin(const HttpRequest& req, HttpResponse* resp)
{
    // 简化:从 body 解析 username=xxx&password=xxx
    // 真实应该 JSON 解析或者 form 解析,Day 17 偷懒用字符串包含
    std::string body = req.getBody();

    std::cout << "[DEBUG] body length = " << body.size() << "\n";
    std::cout << "[DEBUG] body content = [" << body << "]\n";  

    bool ok = body.find("username=lim") != std::string::npos
           && body.find("password=secret") != std::string::npos;

    if (!ok) {
        resp->setStatusCode(HttpResponse::k401Unauthorized);
        resp->setStatusMessage("Unauthorized");
        resp->setContentType("text/plain");
        std::string b = "Invalid credentials\n";
        resp->setContentLength(b.size());
        resp->setBody(b);
        return;
    }

    // 登录成功:拿 session,塞用户信息
    auto session = g_server->getSessionManager()->getSession(req, resp);
    session->setValue("userId", "42");
    session->setValue("username", "lim");
    session->setValue("isLoggedIn", "true");
    g_server->getSessionManager()->updateSession(session);

    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    std::string b = "Login OK, sessionId=" + session->getId() + "\n";
    resp->setContentLength(b.size());
    resp->setBody(b);
}

void handleProfile(const HttpRequest& req, HttpResponse* resp)
{
    auto session = g_server->getSessionManager()->getSession(req, resp);
    if (session->getValue("isLoggedIn") != "true") {
        resp->setStatusCode(HttpResponse::k401Unauthorized);
        resp->setStatusMessage("Unauthorized");
        std::string b = "Please login first\n";
        resp->setContentType("text/plain");
        resp->setContentLength(b.size());
        resp->setBody(b);
        return;
    }

    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    std::string b = "Hello " + session->getValue("username") +
                    ", userId=" + session->getValue("userId") + "\n";
    resp->setContentLength(b.size());
    resp->setBody(b);
}

void handleLogout(const HttpRequest& req, HttpResponse* resp)
{
    auto session = g_server->getSessionManager()->getSession(req, resp);
    g_server->getSessionManager()->destroySession(session->getId());

    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    std::string b = "Logout OK\n";
    resp->setContentLength(b.size());
    resp->setBody(b);
}

int main()
{
    EventLoop loop;
    InetAddress addr(8888);
    HttpServer server(&loop, addr, "limSessionServer");
    g_server = &server;

    // ⭐ 给 server 装上 SessionManager
    auto storage = std::make_unique<MemorySessionStorage>();
    auto manager = std::make_unique<SessionManager>(std::move(storage));
    server.setSessionManager(std::move(manager));

    // 路由
    server.Post("/login",  handleLogin);
    server.Get ("/profile", handleProfile);
    server.Post("/logout", handleLogout);

    server.setThreadNum(2);
    std::cout << "Session server running at http://127.0.0.1:8888/\n";
    std::cout << "Try:\n";
    std::cout << "  curl --cookie-jar c.txt -X POST -d 'username=lim&password=secret' http://127.0.0.1:8888/login\n";
    std::cout << "  curl --cookie c.txt http://127.0.0.1:8888/profile\n";
    std::cout << "  curl http://127.0.0.1:8888/profile  (无 cookie,应 401)\n";

    server.start();
    loop.loop();
}