// test_acceptor.cc
//
// 测试 Acceptor:
//   1. 用一个真正的 EventLoop 驱动 Acceptor
//   2. 注册 newConnectionCallback: 收到新连接打印一行,然后 close 掉
//   3. 用 nc 客户端连进来验证
//   4. 重点观察: CPU 应该接近 0% (epoll_wait 在睡觉)
//                 EAGAIN 错误日志彻底消失
//
// 测试方法:
//   终端 1:  ./test_acceptor
//   终端 2:  nc 127.0.0.1 8888
//   终端 3:  nc 127.0.0.1 8888
//   终端 4:  htop / top -p $(pgrep test_acceptor)   ← 看 CPU 占用
//
//   Ctrl+C 退出 test_acceptor

#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"
#include "SocketsOps.h"

#include <stdio.h>
#include <unistd.h>

using namespace lim;

//这里没有线程池调度，只是存打印信息
// 新连接回调:打印连接信息,然后关掉 fd (Day 9 才会真正交给 TcpConnection)
void onNewConnection(int connfd, const InetAddress& peer)
{
    printf("✅ 新连接 connfd=%d, peer=%s\n",
           connfd, peer.toIpPort().c_str());
    fflush(stdout);

    // Day 6 还没有 TcpConnection,这里直接 close 掉
    sockets::close(connfd);
}

int main()
{
    printf("=== Acceptor 测试 ===\n");
    printf("用 nc 127.0.0.1 8888 测试\n");
    printf("Ctrl+C 退出\n\n");

    EventLoop loop;     // 主 loop (baseLoop)

    InetAddress listenAddr(8888);
    Acceptor acceptor(&loop, listenAddr, false);

    // ⭐ 注册回调: 这是 Acceptor 跟外界的唯一接口
    acceptor.setNewConnectionCallback(onNewConnection);

    acceptor.listen();
    printf("已监听 %s\n\n", listenAddr.toIpPort().c_str());

    // 进入 Reactor 主循环 —— epoll_wait 会阻塞,CPU 趋近 0
    loop.loop();

    return 0;
}