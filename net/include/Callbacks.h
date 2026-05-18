//Callbacks.h
//所有跨类共享的回调类型 + 几个 forward declaration(前置声明),集中到一个轻量头文件

#pragma once

#include "Timestamp.h"
#include <functional>
#include<memory>

namespace lim
{

    class Buffer;
    class TcpConnection;

    //————————五个核心回调函数类型申明————————————————————————
    //1.连接建立/断开回调
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;


    //2.收到消息时回调：业务解析协议、处理请求、生成响应
    using MessageCallback=std::function<void(const TcpConnectionPtr&,
                                             Buffer*,
                                             Timestamp)>;
    
    //3.数据全部写完时调用 (可选)
    using WriteCompleteCallback=std::function<void(const TcpConnectionPtr&)>;

    //4.outputBuffer_ 积压超过阈值时调用 (可选)
    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;

    // 5. 连接真要关闭时调用 (内部用,业务一般不设)
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
}//namespace lim