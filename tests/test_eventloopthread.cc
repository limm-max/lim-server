// test_eventloopthread.cc
//
// 测试 EventLoopThread:
//   1. 创建,启动子线程
//   2. 主线程通过返回的 EventLoop* 给子线程派任务
//   3. 验证任务在子线程跑(打印 tid 确认)

#include "EventLoopThread.h"
#include "EventLoop.h"
#include "CurrentThread.h"
#include "Logger.h"

#include <unistd.h>
#include <stdio.h>

using namespace lim;

void printTid(const char* tag)
{
    printf("[%s] tid = %d\n", tag, CurrentThread::tid());
    fflush(stdout);    // ⭐ 立刻刷新,防止 buffer 缓存看不到
}

int main()
{
    printTid("main");

    // ① 注册子线程初始化回调(可选)
    auto initCb = [](EventLoop* loop) {
        printTid("threadInit");
        
        (void)loop;
    };

    // ② 构造 EventLoopThread,起一个新线程
    printTid("before create EventLoopThread");
    EventLoopThread t(initCb, "myWorker");
    printTid("after create, before startLoop");

    // ③ 启动子线程,拿到它的 EventLoop 指针
    EventLoop* subLoop = t.startLoop();
    printTid("after startLoop, got subLoop ptr");

    // ④ 派任务给子线程(注意:在主线程调,但任务在子线程跑)
    subLoop->runInLoop([]() {
        printTid("task1 in subLoop");
    });

    sleep(1);   // 让子线程有时间处理
    printTid("after first sleep");
    subLoop->runInLoop([]() {
        printTid("task2 in subLoop");
    });
    printTid("after second runInLoop");

    sleep(1);

    printTid("main exiting");

    // ⑤ t 析构时,自动 quit 子线程的 loop + join
    return 0;
}