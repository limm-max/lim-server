// test_tcpconnection.cc
//
// 测试 TcpConnection: 实现一个简易 echo server
//   主线程: Acceptor 接受连接
//   接到连接后,创建 TcpConnection,在同一个 loop 跑(简化,没派子线程)
//   业务回调: 收到啥发回啥
//
// 测试方法:
//   终端 1: ./test_tcpconnection
//   终端 2: nc 127.0.0.1 9999
//           输入任意文字回车,应该回显
//           Ctrl+C 断开,服务器应打印 "断开" 日志
//   Ctrl+C 退出

#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "InetAddress.h"
#include "Logger.h"
#include "Buffer.h"

#include <unordered_map>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace lim;

// 全局连接表 (简化版 TcpServer 的 connections_)
std::unordered_map<int, TcpConnectionPtr> g_connections;
int g_nextConnId = 0;
EventLoop* g_loop = nullptr;

// 业务: 连接建立 / 断开
void onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        printf("✅ 新连接 %s 已建立\n", conn->name().c_str());
    }
    else
    {
        printf("❌ 连接 %s 已断开\n", conn->name().c_str());
        // 从 map 删除 (在 loop 里 queue 一下,避免迭代中删)
        g_loop->queueInLoop([connFd = conn->getLoop() ? 0 : 0,
                             name = conn->name()](){
            // 简化:遍历 map 找名字匹配的删
            for (auto it = g_connections.begin(); it != g_connections.end(); )
            {
                if (it->second->name() == name)
                {
                    it->second->connectDestroyed();
                    it = g_connections.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        });
    }
}

// 业务: 收到数据,echo 回去
void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    std::string msg = buf->retrieveAllAsString();
    printf("📨 [%s] 收到 \"%s\" (at %s)\n",
           conn->name().c_str(),
           msg.c_str(),
           time.toFormattedString().c_str());
    conn->send("ECHO: " + msg);
}

// 模拟 TcpServer 的 newConnection: 创建 TcpConnection
void onNewConnection(int connfd, const InetAddress& peer)
{
    ++g_nextConnId;

    // 获取本端地址
    sockaddr_in localSa;
    socklen_t localLen = sizeof localSa;
    if (::getsockname(connfd, reinterpret_cast<sockaddr*>(&localSa), &localLen) < 0)
    {
        LOG_ERROR << "getsockname fail";
    }
    InetAddress localAddr(localSa);

    char buf[64];
    snprintf(buf, sizeof buf, "test-conn#%d", g_nextConnId);

    // ⭐ 关键: TcpConnection 必须用 shared_ptr 包
    TcpConnectionPtr conn(new TcpConnection(g_loop, buf, connfd, localAddr, peer));

    // 注册业务回调
    conn->setConnectionCallback(onConnection);
    conn->setMessageCallback(onMessage);

    // 保存到 map (否则 shared_ptr 出作用域就析构,连接立刻关掉)
    g_connections[g_nextConnId] = conn;

    // 触发连接建立流程 (注册到 epoll + 调 connectionCallback)
    conn->connectEstablished();
}

int main()
{
    printf("=== TcpConnection 测试 (简易 echo server) ===\n");
    printf("用 nc 127.0.0.1 9999 测试\n");
    printf("发什么文字应该回显 \"ECHO: <文字>\"\n");
    printf("Ctrl+C 退出服务器\n\n");

    EventLoop loop;
    g_loop = &loop;

    InetAddress listenAddr(9999);
    Acceptor acceptor(&loop, listenAddr, false);
    acceptor.setNewConnectionCallback(onNewConnection);

    acceptor.listen();
    LOG_INFO << "Server listening on " << listenAddr.toIpPort();

    loop.loop();
    return 0;
}