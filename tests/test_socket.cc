// test_socket.cc
//
// 测试 Socket + InetAddress:
//   1. InetAddress 三种构造,toIpPort 输出验证
//   2. Socket 接管 fd,bind / listen / accept 系统调用打通
//   3. 用 nc 127.0.0.1 8080 验证能连上并打印对端地址
//
// 测试方式:
//   终端 1: ./test_socket
//   终端 2: nc 127.0.0.1 8080
//   终端 3: nc 127.0.0.1 8080
//   (按 Ctrl+C 退出 test_socket)

#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

using namespace lim;

// 测试 1: InetAddress 各种构造和读取
void test_inet_address()
{
    printf("\n=== Test 1: InetAddress ===\n");

    // 默认端口构造,IP 为 0.0.0.0
    InetAddress addr1(8080);
    printf("addr1: ip=%s, port=%u, ipPort=%s\n",
           addr1.toIp().c_str(), addr1.toPort(), addr1.toIpPort().c_str());

    // 显式 IP + 端口
    InetAddress addr2(8888, "192.168.1.100");
    printf("addr2: ip=%s, port=%u, ipPort=%s\n",
           addr2.toIp().c_str(), addr2.toPort(), addr2.toIpPort().c_str());

    // 验证: 拷贝行为(InetAddress 是值类型,可以拷贝)
    InetAddress addr3 = addr2;
    printf("addr3 (copy of addr2): ipPort=%s\n", addr3.toIpPort().c_str());
}

// 测试 2: Socket 完整的服务端流程
void test_socket_server()
{
    printf("\n=== Test 2: Socket server ===\n");
    printf("用 nc 127.0.0.1 8080 来测试\n");
    printf("Ctrl+C 退出\n");

    // 用最朴素的 ::socket 创建 fd (Day 7 Acceptor 会用更优雅的工厂函数)
    // SOCK_STREAM = TCP, IPPROTO_TCP = TCP 协议
    int listenfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                            IPPROTO_TCP);
    if (listenfd < 0)
    {
        LOG_FATAL << "::socket failed";
    }

    // 接管 fd
    Socket listenSocket(listenfd);

    // 必须开 SO_REUSEADDR,否则上次的 TIME_WAIT 会让 bind 失败
    listenSocket.setReuseAddr(true);
    listenSocket.setKeepAlive(true);

    // bind + listen
    InetAddress listenAddr(8080);
    listenSocket.bindAddress(listenAddr);
    listenSocket.listen();

    LOG_INFO << "Server listening on " << listenAddr.toIpPort();
    printf("已监听 %s\n", listenAddr.toIpPort().c_str());

    // 简陋的"我自己来 epoll"逻辑:循环 accept
    // 因为 listenfd 是非阻塞的,accept 没连接时会返回 -1 / EAGAIN
    // 真正的项目里这是 EventLoop + Channel 在干的事,这里就 sleep 简化
    while (true)
    {
        InetAddress peer;
        int connfd = listenSocket.accept(&peer);
        if (connfd >= 0)
        {
            printf("✅ 新连接 fd=%d, 对端=%s\n",
                   connfd, peer.toIpPort().c_str());
            // 用完 connfd 后关掉(真实项目里会扔进 TcpConnection 接管)
            ::close(connfd);
        }
        ::usleep(100 * 1000);   // 100ms 轮询一次
    }
}

int main()
{
    test_inet_address();
    test_socket_server();   // 永久循环,Ctrl+C 退出
    return 0;
}