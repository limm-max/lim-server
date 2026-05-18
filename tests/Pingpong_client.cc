// pingpong_client.cc
//
// Day 11 简易压测客户端 (独立程序,不依赖 lim-server)
//
// 用法:
//   ./pingpong_client <ip> <port> <threads> <duration_sec>
//   ./pingpong_client 127.0.0.1 9999 4 10
//
// 流程:
//   - 起 N 个线程
//   - 每个线程:不停 send / recv 16 字节小消息
//   - 持续 duration 秒,统计总 QPS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

std::atomic<int64_t> g_total_messages(0);
std::atomic<int>     g_stop(0);

struct ThreadArg {
    const char* ip;
    int port;
};

// 单个客户端线程: 不停 send/recv
void* clientThread(void* arg)
{
    ThreadArg* a = static_cast<ThreadArg*>(arg);

    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return nullptr; }

    // 禁用 Nagle, 提高小包延迟测试准确性
    int on = 1;
    ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof on);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(a->port);
    ::inet_pton(AF_INET, a->ip, &addr.sin_addr);

    if (::connect(sockfd, (sockaddr*)&addr, sizeof addr) < 0) {
        perror("connect");
        ::close(sockfd);
        return nullptr;
    }

    char buf[16];
    memset(buf, 'X', 16);

    int64_t local_count = 0;
    while (!g_stop.load(std::memory_order_relaxed)) {
        // 发 16 字节
        ssize_t n = ::send(sockfd, buf, 16, 0);
        if (n != 16) break;

        // 收 16 字节 echo
        ssize_t r = ::recv(sockfd, buf, 16, MSG_WAITALL);
        if (r != 16) break;

        ++local_count;
    }

    g_total_messages.fetch_add(local_count, std::memory_order_relaxed);
    ::close(sockfd);
    return nullptr;
}

int main(int argc, char* argv[])
{
    if (argc != 5) {
        printf("Usage: %s <ip> <port> <threads> <duration_sec>\n", argv[0]);
        return 1;
    }

    const char* ip       = argv[1];
    int         port     = atoi(argv[2]);
    int         nthread  = atoi(argv[3]);
    int         duration = atoi(argv[4]);

    printf("Connecting %s:%d  threads=%d  duration=%ds\n",
           ip, port, nthread, duration);

    ThreadArg arg{ ip, port };

    // 启动客户端线程
    std::vector<pthread_t> threads(nthread);
    for (int i = 0; i < nthread; ++i) {
        pthread_create(&threads[i], nullptr, clientThread, &arg);
    }

    // 跑 duration 秒
    auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(duration));
    g_stop.store(1, std::memory_order_relaxed);

    // 等线程退出
    for (int i = 0; i < nthread; ++i) {
        pthread_join(threads[i], nullptr);
    }

    auto end = std::chrono::steady_clock::now();
    double sec = std::chrono::duration<double>(end - start).count();

    int64_t total = g_total_messages.load();
    double qps = total / sec;

    printf("\n========== 压测结果 ==========\n");
    printf("总消息数      : %ld\n", total);
    printf("实际耗时      : %.2f 秒\n", sec);
    printf("QPS (消息/秒) : %.0f\n", qps);
    printf("每线程 QPS    : %.0f\n", qps / nthread);
    printf("===============================\n");

    return 0;
}