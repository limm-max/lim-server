#include "EventLoopThread.h"
#include "EventLoop.h"
#include <stdio.h>

namespace lim{
    EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                        const std::string& name)
        :loop_(nullptr)
        ,exiting_(false)
        ,thread_(std::bind(&EventLoopThread::threadFunc,this),name)
        ,mutex_()
        ,cond_(mutex_)
        ,callback_(cb)
        {}
    
        //析构：确保子线程退出+资源释放
        //设置设置 exiting_、子线程褪去循环，并join
    EventLoopThread::~EventLoopThread(){
        exiting_=true;
        if(loop_!=nullptr){
            loop_->quit();
            thread_.join();
        }
    }

    EventLoop* EventLoopThread::startLoop(){
        thread_.start();

        EventLoop* loop=nullptr;
        {
            MutexLockGuard lock(mutex_);  
            while(loop_==nullptr){
                cond_.wait();
            }
            loop=loop_;
        }

        //子线程负责创建eventloop，并修改当前对象的loop_*指针
        return loop;
    }

    void EventLoopThread::threadFunc(){
        EventLoop loop; //创建在子线程栈上
        //callback函数，用于对该子线程进行初始化（比如，日志记录、设置变量等）
        if(callback_){
            callback_(&loop);
        }
        //锁负责修改主线程的loop_对象
        {
            MutexLockGuard lock(mutex_);
            loop_=&loop;
            cond_.notify();
        }

        loop.loop();

        //如果退出loop走到这一步，说明是析构了
        MutexLockGuard lock(mutex_);
        loop_=nullptr;
    }
}