// test_eventloopthreadpool.cc
//
// 测试 EventLoopThreadPool:
//   1. 启动一个 Pool, 创建 3 个 subLoop
//   2. 测试 round-robin: 调 getNextLoop 多次,看是不是均匀
//   3. 给每个 subLoop 派任务,验证任务确实在不同子线程跑

#include "EventLoopThreadPool.h"
#include "EventLoop.h" 
#include "CurrentThread.h"
#include "Logger.h"

#include <unistd.h>
#include <stdio.h>

using namespace lim;

void markTid(const char* tag)
{
    printf("[%s] tid=%d\n", tag, CurrentThread::tid());
    fflush(stdout);
}

int main()
{
    markTid("main");

    EventLoop baseLoop;
    markTid("baseLoop created");

    // ───────── 测试 1: 退化模式 ─────────
    {
        printf("\n=== 测试 1: 退化模式 (numThreads=0) ===\n");
        fflush(stdout);

        EventLoopThreadPool pool(&baseLoop, "test-pool-0");
        // 不调 setThreadNum, 默认 0
        pool.start([](EventLoop* loop) {
            printf("  退化模式 cb 调到 loop=%p (baseLoop)\n", loop);
            fflush(stdout);
        });

        for (int i = 0; i < 3; ++i)
        {
            EventLoop* loop = pool.getNextLoop();
            printf("  getNextLoop #%d → %p (期望 baseLoop=%p)\n",
                   i, loop, &baseLoop);
            fflush(stdout);
        }
    }

    // ───────── 测试 2: 多 subLoop + round-robin ─────────
    {
        printf("\n=== 测试 2: 3 个 subLoop + round-robin ===\n");
        fflush(stdout);

        EventLoopThreadPool pool(&baseLoop, "test-pool-3");
        pool.setThreadNum(3);
        pool.start([](EventLoop* loop) {
            printf("  cb in subLoop, tid=%d, loop=%p\n",
                   CurrentThread::tid(), loop);
            fflush(stdout);
        });

        // 给每个 subLoop 派一个任务,看 tid 是否不同
        for (int i = 0; i < 6; ++i)   // 调 6 次,看轮询是否 1→2→3→1→2→3
        {
            EventLoop* loop = pool.getNextLoop();
            int call_no = i;
            loop->runInLoop([call_no, loop]() {
                printf("  任务 %d 在 tid=%d, loop=%p\n",
                       call_no, CurrentThread::tid(), loop);
                fflush(stdout);
            });
        }

        sleep(1);   // 让任务都跑完
    }

    markTid("main exiting");
    return 0;
}