// test_static_server.cc
//
// 静态文件服务器示例:
//   • GET /          → 自动返回 /index.html
//   • GET /xxx.html  → 返回 resource/xxx.html
//   • GET /style.css → 返回 resource/style.css
//   • 找不到 → 404 + NotFound.html

#include "HttpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "StaticFileReader.h"
#include "MimeType.h"

#include <iostream>
#include <string>

using namespace lim;

const std::string RESOURCE_DIR = "./resources";   // 静态资源根目录

// 静态文件 handlercallback
//   把请求路径拼到 resource/ 下,读文件、返回
void serveStatic(const HttpRequest& req, HttpResponse* resp)
{
    std::string path = req.path();

    // 根路径 → index.html
    if (path == "/") path = "/index.html";

    // 拼成实际文件路径
    const std::string filePath = RESOURCE_DIR + path;

    StaticFileReader file{filePath};

    // 文件不存在 → 404
    if (!file.isValid())
    {
        std::cout << "[404] " << filePath << "\n";
        file.resetDefaultFile(RESOURCE_DIR + "/NotFound.html");

        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
    }
    else
    {
        std::cout << "[200] " << filePath << "\n";
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
    }

    // 读文件内容
    std::vector<char> buf(file.size());
    file.readFile(buf);
    std::string body(buf.data(), buf.size());

    // 根据扩展名设 Content-Type
    resp->setContentType(getMimeType(filePath));
    resp->setContentLength(body.size());
    resp->setBody(body);
    resp->setCloseConnection(false);   // 保持长连接,加载完一组资源
}

int main()
{
    EventLoop loop;
    InetAddress addr(8888);
    HttpServer server(&loop, addr, "limStaticServer");

    // ⭐ 把所有 GET 请求都注册到 serveStatic
    // 这里有点 trick:Router 是 (method, exact_path) → handler,
    // 不能直接"所有 GET 路径都用同一个 handler"
    // 简单做法:把 serveStatic 设为 httpCallback (路由兜底)
    // 这样 Router 找不到的请求都会走到这里
    server.setHttpCallback(serveStatic);

    server.setThreadNum(2);
    std::cout << "Static server running at http://127.0.0.1:8888/\n";
    std::cout << "Open browser → http://127.0.0.1:8888/\n";

    server.start();
    loop.loop();
}