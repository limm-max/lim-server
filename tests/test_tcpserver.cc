// test_tcpserver.cc
//
// ⭐ Day 10 高光时刻 ⭐
// 一个完整的多线程 echo server
//
// 测试方法:
//   终端 1: ./test_tcpserver
//   终端 2: nc 127.0.0.1 9999  (输入文字应该回显)
//   终端 3: nc 127.0.0.1 9999  (同时连接,不互相干扰)
//   终端 4: top -H -p $(pgrep test_tcpserver)
//          (能看到 1 主线程 + 4 个 IO 线程)
//
//   压力测试 (可选):
//   终端 2: for i in {1..1000}; do (echo "msg-$i" | nc -q 1 127.0.0.1 9999) & done

#include "TcpServer.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "Logger.h"

#include <stdio.h>
#include <functional>

using namespace lim;
using namespace std::placeholders;

// ─── echo server 业务类 ──────────────────────────────────────────────────────
class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name)
        : server_(loop, addr, name)
    {
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, _1));
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, _1, _2, _3));
        server_.setThreadNum(4);    // ⭐ 4 个 IO 线程
    }

    void start() { server_.start(); }

    ~EchoServer() {};

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            printf("✅ [%s] UP   from %s\n",
                   conn->name().c_str(),
                   conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            printf("❌ [%s] DOWN from %s\n",
                   conn->name().c_str(),
                   conn->peerAddress().toIpPort().c_str());
        }
        fflush(stdout);
    }

    void onMessage(const TcpConnectionPtr& conn,
                   Buffer* buf,
                   Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        printf("📨 [%s] got %zu bytes: %s",
               conn->name().c_str(), msg.size(), msg.c_str());
        if (!msg.empty() && msg.back() != '\n') printf("\n");
        fflush(stdout);

        conn->send(msg);   // echo
    }

    TcpServer server_;
};

// ─── main ────────────────────────────────────────────────────────────────────
int main()
{
    printf("============================================\n");
    printf("  lim-server · EchoServer · 4 IO threads\n");
    printf("  Listen on 0.0.0.0:9999\n");
    printf("  Test: nc 127.0.0.1 9999\n");
    printf("============================================\n\n");
    Logger::setLogLevel(Logger::WARN); 
    EventLoop loop;
    InetAddress addr(9999);

    EchoServer server(&loop, addr, "EchoServer");
    server.start();
    loop.loop();

    return 0;
}