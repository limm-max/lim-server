//EventLoopThreadPool.cc

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"

namespace lim
{
    EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,const std::string& name)
        :baseloop_(baseLoop)
        ,name_(name)
        ,started_(false)
        ,numThreads_(0)
        ,next_(0)
    {

    }
    //EventLoop由子线程上的对象析构而析构，EventLoopThreadPool没有其他堆内存，不需要析构
    EventLoopThreadPool::~EventLoopThreadPool(){}

    void EventLoopThreadPool::start(const ThreadInitCallback& cb)
    {
        started_=true;
        for(int i=0;i<numThreads_;++i)
        {
            char buf[64]={0};
            snprintf(buf,sizeof buf,"%s-%d",name_.c_str(),i);
            EventLoopThread* t=new EventLoopThread(cb,buf);
            threads_.push_back(std::unique_ptr<EventLoopThread>(t));    //使用unique_ptr接管所有权
            loops_.push_back(t->startLoop());
        }

        if(numThreads_==0 && cb)
        {
            cb(baseloop_);
        }
    }

        EventLoop* EventLoopThreadPool::getNextLoop()
        {
            EventLoop* loop=baseloop_;  //如果退化，则直接返回baseloop
            if(!loops_.empty())
            {
                loop=loops_[next_];
                ++next_;
                if(next_>=static_cast<int>(loops_.size()))
                {
                    next_=0;
                }
                    
            }
            return loop;
        }

        std::vector<EventLoop*> EventLoopThreadPool::getAllLoop()
        {
            if(loops_.empty())
            {
                return std::vector<EventLoop*>{baseloop_};
            }
            return loops_;
        }

}//namespace lim;