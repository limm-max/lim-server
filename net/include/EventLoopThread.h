// EventLoopThread.h
// ==========================================
// 把一个【新线程】和【一个 EventLoop】打包在一起:
//
//   外部调 thread.startLoop()
//     → 内部创建新线程
//     → 新线程里创建一个 EventLoop(局部变量)
//     → 把 EventLoop 指针传回主线程
//     → 新线程进入 loop.loop() 永不退出
//================================================

#pragma once

#include "Noncopyable.h"
#include "Thread.h"
#include "MutexLock.h"
#include "Condition.h"

#include<functional>
#include<string>

namespace lim{

class EventLoop;
class EventLoopThread:public Noncopyable{
public:
    //因为callback函数要传入loop指针参数进行设置
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const std::string& name = std::string());
    ~EventLoopThread();
    EventLoop* startLoop();


private:
    //子线程入口函数
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;   //保护loop_
    Condition cond_;
    ThreadInitCallback callback_;   // 子线程初始化回调
};
}