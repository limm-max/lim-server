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
#include"Router.h"
#include "Callbacks.h"
#include "MiddlewareChain.h"
#include "SessionManager.h"

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

    //——————————注册router的回调表——————————————————
    // 静态路由 - 回调
    void Get(const std::string& path, Router::HandlerCallback cb)
    { router_.registerCallback(HttpRequest::kGet, path, std::move(cb)); }

    void Post(const std::string& path, Router::HandlerCallback cb)
    { router_.registerCallback(HttpRequest::kPost, path, std::move(cb)); }

    // 静态路由 - Handler 对象
    void Get(const std::string& path, Router::HandlerPtr handler)
    { router_.registerHandler(HttpRequest::kGet, path, std::move(handler)); }

    void Post(const std::string& path, Router::HandlerPtr handler)
    { router_.registerHandler(HttpRequest::kPost, path, std::move(handler)); }

    // 动态路由 (method 显式传)
    void addRoute(HttpRequest::Method method, const std::string& pattern,
                Router::HandlerCallback cb)
    { router_.addRegexCallback(method, pattern, std::move(cb)); }

    void addRoute(HttpRequest::Method method, const std::string& pattern,
                Router::HandlerPtr handler)
    { router_.addRegexHandler(method, pattern, std::move(handler)); }

    void addMiddleware(std::shared_ptr<Middleware> mw)
    { middlewareChain_.addMiddleware(std::move(mw)); }


    //session and cookie
    void setSessionManager(std::unique_ptr<SessionManager> mgr)
    { sessionManager_ = std::move(mgr); }

    SessionManager* getSessionManager() const
    { return sessionManager_.get(); }



private:
    //将一个connection的callback 翻译成 HTTP 层语义
    //对应tcpserver里的connectionCallback
    void onConnection(const TcpConnectionPtr& conn);
    //对应TcpServer里的MessageCallback
    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp receiveTime);
    // 业务回调（完整请求收齐后,响应，组装响应，发回去）
    //httpCallback_是真正的业务逻辑，这个是对request进行初步check，调用httpCallback_业务逻辑，发送最终回复
    void onRequest(const TcpConnectionPtr& conn, const HttpRequest& req);

    Router router_;
    TcpServer server_;
    HttpCallback    httpCallback_;  //输入请求，处理请求，输出请求。业务逻辑，由上层注册
    MiddlewareChain middlewareChain_;   //中间插件
    std::unique_ptr<SessionManager> sessionManager_;
};

}//namespace lim