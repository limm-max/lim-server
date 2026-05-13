#include "EventLoop.h"
#include "Thread.h"
#include "CurrentThread.h"
#include "Logger.h"
 
#include <unistd.h>     // sleep

using namespace lim;

int main(){
    LOG_INFO<<"main thread id="<<CurrentThread::tid();
    EventLoop loop;

    // 启动另一个线程,3 秒后调 runInLoop 派任务,5 秒后调 quit
    Thread t([&loop]() {
        LOG_INFO << "worker thread id = " << CurrentThread::tid();
 
        ::sleep(3);
        LOG_INFO << "worker: queueing a task into loop";
        loop.runInLoop([]() {
            LOG_INFO << "task runs in thread " << CurrentThread::tid()
                     << " (should be main thread)";
        });
 
        ::sleep(2);
        LOG_INFO << "worker: calling loop.quit()";
        loop.quit();
    }, "worker");
 
    t.start();
 
    // 主线程进入 loop
    loop.loop();
 
    LOG_INFO << "loop exited, joining worker thread";
    return 0;
}