// EventLoop.cc


#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "Logger.h"

#include <sys/eventfd.h>     // eventfd
#include <unistd.h>           // read / write / close
#include <fcntl.h>
#include <errno.h>

namespace lim
{

namespace
{
// 防止一个线程创建多个 EventLoop —— 用 thread_local 全局变量记录,__thread关键字实现
// 每个线程独立一份。若不为空则当前线程已有 EventLoop
__thread EventLoop* t_loopInThisThread=nullptr;

// poll 超时时间(毫秒),10 秒醒一次,避免长时间无事件时彻底僵死
const int kPollTimeMs = 10000;

// 创建 wakeup 用的 eventfd
int createEventfd(){
    int evtfd=::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC); //系统调用，创建一个用于实践通知的文件描述符。阻塞+执行时关闭
    if(evtfd<0){
        LOG_FATAL<<"Failed in eventfd,errno:"<<errno; //记录日志,错误终止行为已经在日志逻辑里了
    }
    return evtfd;
}
} //anonymous namespace

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))
    ,wakeupFd_(createEventfd())
    ,wakeupChannel_(new Channel(this,wakeupFd_))
    {
        LOG_DEBUG<<"EventLoop created"<<this<<"in thread"<<threadId_;
        if(t_loopInThisThread){
            LOG_FATAL<<"Another EventLoop"<<t_loopInThisThread<<"exitts in this thread"<<threadId_;
        }else{
            t_loopInThisThread=this;
        }
        wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
        wakeupChannel_->enableReading();
    }

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
}

void EventLoop::loop(){
    assertInLoopThread(); 
    looping_=true;
    quit_=false;

    LOG_INFO<<"EventLoop"<<this<<"start looping";

    while(!quit_){
        activeChannels_.clear();
        //调用epoll_wait,使用activeChannels_承接有事件的channel
        //epoll_wait返回的是epoll_event类型的数组，通过poller里的fillActiveChannels将其整理成channel数组返回
        //fillActiveChannels函数通过调用channel里的set_revents函数，将实际发生的事件存进channel里
        pollReturnTime_=poller_->poll(kPollTimeMs,&activeChannels_);

        //处理活跃channel，根据事件的发生分发不同的回调函数
        for(Channel* channel:activeChannels_){
            channel->handleEvent(pollReturnTime_);
        }

        //处理其他线程派来的任务
        doPendingFunctors();
    }

    //t跳出循环说明loop停止了，记录日志
    LOG_INFO<<"EventLoop"<<this<<"stop looping";
    looping_=false;

}

void EventLoop::quit(){
    quit_=true;

    // 如果是别的线程调 quit,需要唤醒本 loop
    //通过wakeup事件
    // (本 loop 此时可能阻塞在 epoll_wait,光设 quit_ 还不够)
    if(!isInLoopThread()){
        wakeup();
    }
}

//对象所属线程和被哪个线程调用函数是不一样的。
//---------------------------------------
//跨线程任务处理方式
//---------------------------------------
void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){
        cb();
    }else{
        //别人线程，则放在别人线程的跨任务队列中
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb){
    {
        //线程处理队列任务和线程往队列里放任务，产生竞速关系
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    //非本线程或者该线程正在处理跨线程任务都要进行wakeup
    if(!isInLoopThread() || callingPendingFunctors_){
        wakeup();
    }
}

//唤醒方式：往eventloop的wakeupfd_中写入字节
void EventLoop::wakeup(){
    uint64_t one=1;
    ssize_t n=::write(wakeupFd_,&one,sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

//处理wakeup事件
void EventLoop::handleRead(){
    uint64_t one=1;
    ssize_t n=::read(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one){
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_=true;   //记录标志，正在执行跨线程任务列表

    //这里要拿锁对列表进行处理
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& functor:functors){
        functor();
    }
    callingPendingFunctors_=false;
}


//--------------------------------------------
//channel和poller之间的转发接口
//-------------------------------------------

//保证有些操作只能是对象归属的线程可以执行
void EventLoop::abortNotInLoopThread(){
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
                << " was created in threadId_ = " << threadId_
                << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::updateChannel(Channel* channel){
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    assertInLoopThread();
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel){
    assertInLoopThread();
    return poller_->hasChannel(channel);
}





} // namespace lim