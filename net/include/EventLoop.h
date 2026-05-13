// EventLoop.h
// =============================================================================
// 类名: EventLoop
// 中文: 事件循环
// 别名: "Reactor"(反应堆),muduo 网络库的灵魂
//
// ─── 功能 ─────────────────────────────────────────────────────────────────────
// EventLoop 是 muduo 主从 Reactor 模型的核心。它做三件事:
//
//   1. 跑一个永不退出的主循环 loop():
//      while (!quit_) {
//          activeChannels = poller_->poll();      // 等事件
//          for (ch : activeChannels) ch->handleEvent();  // 分发事件
//          doPendingFunctors();                   // 执行跨线程任务
//      }
//
//   2. 充当 Channel 和 Poller 之间的"中介"(Mediator 模式):
//      Channel::update() → EventLoop::updateChannel() → Poller::updateChannel()
//
//   3. 提供线程安全的"跨线程派活"机制:
//      别的线程可以调 runInLoop / queueInLoop,让本 loop 在自己线程内执行任务
//
// =============================================================================
// 【成员变量】
// =============================================================================
//
//  std::atomic_bool       looping_       ← 主循环是否在跑(loop() 进入时设 true)
//  std::atomic_bool       quit_          ← 退出标志(quit() 设 true)
//  std::atomic_bool       callingPendingFunctors_ ← 是否正在执行任务队列
//                                          (影响 wakeup 策略)
//
//  const pid_t            threadId_      ← 本 loop 绑定的线程 ID(构造时记下)
//  Timestamp              pollReturnTime_← 最近一次 poll 返回的时刻
//
//  std::unique_ptr<Poller> poller_       ← IO 多路复用器(实际是 EPollPoller)
//
//  int                    wakeupFd_      ← eventfd 创建的 fd,用于跨线程唤醒
//  std::unique_ptr<Channel> wakeupChannel_← 包装 wakeupFd_ 的 Channel
//
//  ChannelList            activeChannels_← 每轮循环 poll() 返回的活跃 Channel
//
//  MutexLock              mutex_         ← 保护 pendingFunctors_
//  std::vector<Functor>   pendingFunctors_ ← 跨线程派来的任务队列
//
// =============================================================================
// 【公开方法】
// =============================================================================
//
//  ─ 生命周期 ─
//  EventLoop()                       构造,创建 Poller + wakeupFd + wakeupChannel
//  ~EventLoop()                      析构,关闭 wakeupFd
//
//  ─ 主循环控制 ─
//  void loop()                       启动主循环(只能在本线程调)
//  void quit()                       请求退出(可跨线程调)
//
//  ─ 跨线程派活 ─
//  void runInLoop(Functor cb)        派一个任务,自己线程则立即执行,否则入队
//  void queueInLoop(Functor cb)      派一个任务到队列(必入队 + wakeup)
//
//  ─ 提供给 Channel 的接口 ─
//  void updateChannel(Channel* ch)   转发到 poller_->updateChannel
//  void removeChannel(Channel* ch)   转发到 poller_->removeChannel
//  bool hasChannel(Channel* ch)      转发到 poller_->hasChannel
//
//  ─ 线程归属 ─
//  bool isInLoopThread() const       判断当前线程是否本 loop 的线程
//  void assertInLoopThread()         不是则 LOG_FATAL
//
//  ─ 唤醒(本类内部用,但暴露出来给 wakeup_) ─
//  void wakeup()                     往 wakeupFd_ 写 8 字节,触发可读事件
//
// =============================================================================
// 【私有方法】
// =============================================================================
//
//  void handleRead()           wakeupChannel_ 的读回调,读出 wakeupFd_ 8 字节
//  void doPendingFunctors()    主循环末尾执行任务队列(用 swap 减少持锁时间)
//  void abortNotInLoopThread() 断言失败时调,LOG_FATAL 并终止
//
// =============================================================================
#pragma once 
#include "Noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include "MutexLock.h"
 
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace lim{


class Channel;
class Poller;

class EventLoop:public Noncopyable{

public:
    using Functor=std::function<void()>;
    EventLoop();
    ~EventLoop();

    //主循环函数
    void loop();

    void quit();

    //最近一次poll返回时刻
    Timestamp pollReturnTime() const {return pollReturnTime_;}

    //cb:派的具体任务.runInLoop是统一对外接口
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    void wakeup();

    //channel接口
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    //线程归属判断
    bool isInLoopThread() const{ return threadId_==CurrentThread::tid();}
    void assertInLoopThread(){
        if(!isInLoopThread())
            abortNotInLoopThread();
    }

private:
    void handleRead();  // eventfd的回调函数
    void doPendingFunctors();  // 执行任务队列
    void abortNotInLoopThread();

    using ChannelList=std::vector<Channel*>;

    //原子类型，这仨变量可能被多线程读写。
    std::atomic_bool looping_;
    std::atomic_bool quit_;
    std::atomic_bool callingPendingFunctors_;  //是否在执行跨线程任务

    const pid_t threadId_;
    Timestamp pollReturnTime_;

    std::unique_ptr<Poller> poller_;

    // wakeup 机制
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;

    //跨线程任务队列
    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;  //存储其他线程交给本线程执行的各种回调函数
};
}//namespace lim