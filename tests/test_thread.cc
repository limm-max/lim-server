#include "Thread.h"
#include "CurrentThread.h"

#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <memory>

void threadFunc() {
    printf("子线程: tid=%d\n", lim::CurrentThread::tid());
    sleep(1);
    printf("子线程: 工作完成\n");
}

void threadFunc2(int x) {
    printf("子线程带参数: tid=%d, x=%d\n", lim::CurrentThread::tid(), x);
}

int main() {
    printf("主线程: tid=%d\n", lim::CurrentThread::tid());

    // 测试 1: 基本启动 + join
    {
        lim::Thread t1(threadFunc, "TestThread-1");
        printf("[创建后] started=%d, tid=%d, name=%s\n",
               t1.started(), t1.tid(), t1.name().c_str());

        t1.start();
        printf("[start后] started=%d, tid=%d\n", t1.started(), t1.tid());

        t1.join();
        printf("[join后] 线程 %s 结束\n", t1.name().c_str());
    }

    // 测试 2: Lambda + 捕获
    {
        int x = 42;
        lim::Thread t2([x]() {
            printf("Lambda 子线程: tid=%d, 捕获 x=%d\n",
                   lim::CurrentThread::tid(), x);
        }, "LambdaThread");
        t2.start();
        t2.join();
    }

    // 测试 3: std::bind 传参.(因为thread签名统一传入无参函数入口，所以如果入口函数有参数，需要用bind进行传参)
    {
        lim::Thread t3(std::bind(threadFunc2, 100), "BindThread");
        t3.start();
        t3.join();
    }

    // 测试 4: numCreated 计数
    printf("总共创建过 %d 个 Thread\n", lim::Thread::numCreated());

    // 测试 5: 不 join 也不崩(析构会 detach)
    {
        lim::Thread t5([]() {
            printf("分离线程: tid=%d\n", lim::CurrentThread::tid());
        }, "DetachedThread");
        t5.start();
        // 故意不 join,看析构会不会出问题
    }

    sleep(2);   // 等分离线程跑完
    printf("主线程退出\n");
    return 0;
}