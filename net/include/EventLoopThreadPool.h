//EventLoopThreadPool.h
// 管理一组 EventLoopThread,提供"按需获取一个 subLoop"的能力。
// 是【主从 Reactor 模型】的最后一块拼图。
//
// 主要使用者: 未来的 TcpServer
//   TcpServer 持有一个 EventLoopThreadPool
//   accept 到新连接时,调 pool.getNextLoop() 拿一个 subLoop
//   把新连接的 Channel 注册到这个 subLoop

#pragma once

#include "Noncopyable.h"

#include <functional>
#include<memory>
#include<string>
#include<vector>

namespace lim
{
class EventLoop;
class EventLoopThread;
class EventLoopThreadPool:public Noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop,const std::string& name);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads){numThreads_=numThreads;}

    //启动pool：创建N个EventLoopThread。cb是创建Thread的回调函数
    void start(const ThreadInitCallback& cb=ThreadInitCallback());
    bool started() const {return started_;}
    const std::string& name() const {return name_;}

    //从池中获得一个loop，后续分发任务
    EventLoop* getNextLoop();
    std::vector<EventLoop*> getAllLoop();
private:
    EventLoop* baseloop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;  //next的索引

    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};
}//namespace lim