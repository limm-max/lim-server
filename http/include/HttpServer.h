// HttpServer.h
//
// HttpServer = HTTP 框架对外门面。
//   • 持有 lim::TcpServer (TCP 层)
//   • 给每个连接挂一个 HttpContext (协议层状态)
//   • onMessage 时驱动 HttpContext 解析,完整请求出来后调业务回调
//   • 业务填好 HttpResponse 后,序列化回连接

#pragma once

#include "Noncopyable.h"
#include "TcpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Callbacks.h"

#include <functional>
#include <string>

namespace lim
{
class HttpServer:Noncopyable
{
public:
    using HttpCallback=std::function<void(const HttpRequest&,HttpResponse*)>;

    HttpServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const std::string& name,
                TcpServer::Option option=TcpServer::kNoReusePort);
    
    EventLoop* getLoop() const{return server_.getLoop();}

    void setHttpCallback(HttpCallback cb)   {httpCallback_=cb;}
    void setThreadNum(int n)    {server_.setThreadNum(n);}
    void start();

private:
    //将一个connection的callback 翻译成 HTTP 层语义
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp receiveTime);
    // 完整请求收齐后,组装响应、发回去
    void onRequest(const TcpConnectionPtr& conn, const HttpRequest& req);

    TcpServer server_;
    HttpCallback    httpCallback_;
};

}//namespace lim