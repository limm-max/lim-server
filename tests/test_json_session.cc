// test_json_session.cc
//
// 模拟真实登录流程,前后端用 JSON 通信:
//   POST /login        body: {"username":"lim","password":"secret"}
//                      返回: {"success":true,"userId":42}
//   GET  /profile      返回: {"userId":42,"username":"lim"}
//   POST /logout       返回: {"success":true}
//
// 测试:
//   curl --cookie-jar c.txt -X POST \
//        -H "Content-Type: application/json" \
//        -d '{"username":"lim","password":"secret"}' \
//        http://127.0.0.1:8888/login
//
//   curl --cookie c.txt http://127.0.0.1:8888/profile
//
//   curl --cookie c.txt -X POST http://127.0.0.1:8888/logout

#include "HttpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SessionManager.h"
#include "SessionStorage.h"
#include "JsonUtil.h"

#include <iostream>

using namespace lim;

HttpServer* g_server = nullptr;

// 工具函数:封装一个"返回 JSON 响应"的逻辑
void sendJson(HttpResponse* resp,
              HttpResponse::HttpStatusCode code,
              const std::string& msg,
              const json& body)
{
    std::string bodyStr = body.dump(4);    // 缩进 4 空格,调试友好

    resp->setStatusCode(code);
    resp->setStatusMessage(msg);
    resp->setContentType("application/json");
    resp->setContentLength(bodyStr.size());
    resp->setBody(bodyStr);
    resp->setCloseConnection(false);
}

void handleLogin(const HttpRequest& req, HttpResponse* resp)
{
    try
    {
        // ── 1. 解析请求 JSON ──
        json reqBody = json::parse(req.getBody());

        std::string username = reqBody.value("username", "");
        std::string password = reqBody.value("password", "");

        std::cout << "[Login] username=" << username << "\n";

        // ── 2. 验证(简化,真业务查数据库)──
        if (username != "lim" || password != "secret")
        {
            json resBody = {
                {"success", false},
                {"error", "Invalid credentials"}
            };
            sendJson(resp, HttpResponse::k401Unauthorized, "Unauthorized", resBody);
            return;
        }

        // ── 3. 登录成功,塞 session ──
        auto session = g_server->getSessionManager()->getSession(req, resp);
        session->setValue("userId", "42");
        session->setValue("username", username);
        session->setValue("isLoggedIn", "true");
        g_server->getSessionManager()->updateSession(session);

        // ── 4. 返回 JSON 响应 ──
        json resBody = {
            {"success", true},
            {"userId", 42},
            {"username", username},
            {"sessionId", session->getId()}
        };
        sendJson(resp, HttpResponse::k200Ok, "OK", resBody);
    }
    catch (const json::parse_error& e)
    {
        // 请求 body 不是合法 JSON
        json resBody = {
            {"success", false},
            {"error", std::string("JSON parse error: ") + e.what()}
        };
        sendJson(resp, HttpResponse::k400BadRequest, "Bad Request", resBody);
    }
    catch (const std::exception& e)
    {
        json resBody = {{"success", false}, {"error", e.what()}};
        sendJson(resp, HttpResponse::k500InternalServerError, "Internal Server Error", resBody);
    }
}

void handleProfile(const HttpRequest& req, HttpResponse* resp)
{
    auto session = g_server->getSessionManager()->getSession(req, resp);

    if (session->getValue("isLoggedIn") != "true")
    {
        json resBody = {
            {"success", false},
            {"error", "Please login first"}
        };
        sendJson(resp, HttpResponse::k401Unauthorized, "Unauthorized", resBody);
        return;
    }

    // 返回当前用户的资料
    json resBody = {
        {"success", true},
        {"userId", std::stoi(session->getValue("userId"))},
        {"username", session->getValue("username")},
        {"sessionId", session->getId()}
    };
    sendJson(resp, HttpResponse::k200Ok, "OK", resBody);
}

void handleLogout(const HttpRequest& req, HttpResponse* resp)
{
    auto session = g_server->getSessionManager()->getSession(req, resp);
    std::string sessionId = session->getId();
    g_server->getSessionManager()->destroySession(sessionId);

    json resBody = {
        {"success", true},
        {"message", "Logout OK"},
        {"sessionId", sessionId}
    };
    sendJson(resp, HttpResponse::k200Ok, "OK", resBody);
}

int main()
{
    EventLoop loop;
    InetAddress addr(8888);
    HttpServer server(&loop, addr, "limJsonServer");
    g_server = &server;

    auto storage = std::make_unique<MemorySessionStorage>();
    auto manager = std::make_unique<SessionManager>(std::move(storage));
    server.setSessionManager(std::move(manager));

    server.Post("/login",  handleLogin);
    server.Get ("/profile", handleProfile);
    server.Post("/logout", handleLogout);

    server.setThreadNum(2);
    std::cout << "JSON-aware HTTP server at http://127.0.0.1:8888/\n";
    std::cout << "Test:\n";
    std::cout << "  curl --cookie-jar c.txt -X POST \\\n";
    std::cout << "       -H 'Content-Type: application/json' \\\n";
    std::cout << "       -d '{\"username\":\"lim\",\"password\":\"secret\"}' \\\n";
    std::cout << "       http://127.0.0.1:8888/login\n";
    std::cout << "  curl --cookie c.txt http://127.0.0.1:8888/profile\n";
    std::cout << "  curl --cookie c.txt -X POST http://127.0.0.1:8888/logout\n";

    server.start();
    loop.loop();
}