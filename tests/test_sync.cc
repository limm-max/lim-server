#include "MutexLock.h"
#include "Condition.h"
#include "CountDownLatch.h"
#include "Thread.h"
#include "CurrentThread.h"

#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <memory>

// ===== 测试 1:MutexLock + MutexLockGuard 保护共享计数 =====
//4个线程，循环1000次使用互斥锁对counter进行竞争修改，最终结果应该是40000
void test_mutex(){
    printf("\n=== Test 1: MutexLock ===\n");

    lim::MutexLock mutex;
    int counter=0;
    const int kThreadNum=4;
    const int kPerThread = 10000;

    std::vector<std::unique_ptr<lim::Thread>> threads;
    for(int i=0;i<kThreadNum;++i){
        threads.emplace_back(new lim::Thread([&](){
            for(int j=0;j<kPerThread;++j){
                lim::MutexLockGuard lock(mutex);
                ++counter;  //抢到锁就可以修改

            }
        },"MutexThread"));
    }

    for(auto &t:threads) t->start();
    for(auto & t:threads) t->join();
    int expected=kThreadNum*kPerThread;
    printf("counter = %d (expected %d) %s\n",
           counter, expected,
           counter == expected ? "✅ PASS" : "❌ FAIL");
}

// ===== 测试 2:Condition 实现"等待数据就绪" =====
//锁+条件变量
void test_condition(){
    printf("\n=== Test 2: Condition ===\n");
    lim::MutexLock mutex;
    lim::Condition cond(mutex);
    bool ready=false;
    int data=0;

    lim::Thread producer([&](){
        sleep(1);
        printf("[生产者] 准备数据中...\n");
        {
            lim::MutexLockGuard lock(mutex);
            data = 42;
            ready = true;
            cond.notify();
            printf("[生产者] 数据就绪,发送通知\n");
        }
    },"producer");

    lim::Thread consumer([&](){
        printf("[消费者] 等待数据...\n");
        lim::MutexLockGuard lock(mutex);
        while (!ready) {
            cond.wait();
        }
        printf("[消费者] 拿到数据 = %d %s\n",
               data, data == 42 ? "✅" : "❌");
    }, "Consumer");

    consumer.start();
    producer.start();
    producer.join();
    consumer.join();

}

// ===== 测试 3:CountDownLatch 实现"等所有工作线程就绪" =====
void test_countdown_latch(){
    printf("\n=== Test 3: CountDownLatch ===\n");
    const int kWorkerNum = 3;
    lim::CountDownLatch latch(kWorkerNum);

    std::vector<std::unique_ptr<lim::Thread>> workers;
    for (int i = 0; i < kWorkerNum; ++i) {
        workers.emplace_back(new lim::Thread([&latch, i]() {
            sleep(1 + i);   // 每个线程"准备时间"不同
            printf("[Worker %d] 准备完成 (tid=%d)\n",
                   i, lim::CurrentThread::tid());
            latch.countDown();
        }, "Worker"));
    }

    for (auto& t : workers) t->start();
    printf("[主线程] 等待所有工作线程就绪...\n");
    latch.wait();   // 这里阻塞,主线程获得的锁会释放掉，直到所有 worker 都 countDown
    printf("[主线程] 所有线程就绪,继续执行 ✅\n");

    for (auto& t : workers) t->join();
}

int main(){
    printf("主线程 tid = %d\n", lim::CurrentThread::tid());

    test_mutex();
    test_condition();
    test_countdown_latch();

    printf("\n所有测试完成 🎉\n");
    return 0;
}