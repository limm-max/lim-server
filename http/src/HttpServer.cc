// HttpServer.cc
#include "HttpServer.h"
#include "HttpContext.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "Logger.h"

#include <any>
#include <functional>

namespace lim
{
    //设置默认callback函数
    //如果上层没有设置httpcallback，则同一返回404，避免崩溃
    static void defaultHttpCalback_(const HttpRequest&,HttpResponse* resp)
    {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found.");
        resp->setCloseConnection(true);
    }

    HttpServer::HttpServer(EventLoop* loop,
                      const InetAddress& listenAddr,
                      const std::string& name,
                      TcpServer::Option option)
        :server_(loop,listenAddr,name,option)
        ,httpCallback_(defaultHttpCalback_)
    {
        server_.setConnectionCallback(std::bind(&HttpServer::onConnection,this,std::placeholders::_1));
        server_.setMessageCallback(std::bind(&HttpServer::onMessage,this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3));
    }

    void HttpServer::start()
    {
        LOG_INFO << "HttpServer[" << server_.getLoop()
             << "] starts listening";
        server_.start();
    }


    //建立连接/断开连接的处理函数
    //   建立时:给连接挂一个 HttpContext (用 TcpConnection 的 context 槽)
    //   断开时:任何资源由 TcpConnection 析构自动清理 (HttpContext 是 std::any 持有的)
    void HttpServer::onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            // 给这个连接挂一个全新的 HttpContext
            // std::any 会按值拷贝,内部用 type erasure 装着
            conn->setContext(HttpContext());
            LOG_INFO << "HttpServer - new connection: " << conn->name();
        }
        else
        {
            LOG_INFO << "HttpServer - connection closed: " << conn->name();
            // 不用手动清 context,TcpConnection 析构时 std::any 自然销毁
        }
    }

    // 来字节了 → 驱动 HttpContext 解析 → 解析完整就调业务回调
    void HttpServer::onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp receiveTime)
    {
        // 从连接的 context 槽里把 HttpContext 取出来
        // 用 any_cast<HttpContext>(...) 拿到指针(修改用 mutable context)
        HttpContext* context=std::any_cast<HttpContext>(conn->getMutableContext());

        //解析，解析出错直接这里就回400+关闭连接，否则就调业务回调（onRequest）
        //这里是解析请求头的时候有错才会返回false
        //如果解析没有完毕，这次调用，context会保持上下文信息，知道gotAll状态
        if(!context->parseRequest(buf,receiveTime))
        {
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n"); //不用reponse数据结构
            conn->shutdown();   //关闭写端，保持读端，避免内核缓冲区还有数据没读完而报错。
            return;
        }

        //每次调用onMessage解析一次，如果解析完毕，则进行request业务处理，并reset解析下一条请求
        if(context->gotAll())
        {
            onRequest(conn,context->request());
            context->reset();
        }
    }

    // 业务回调跑完后,序列化 response 发回去
    void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
    {
        // 看请求是否要 Keep-Alive:
        //   HTTP/1.0 默认短连接,Connection: Keep-Alive 才长连
        //   HTTP/1.1 默认长连接,Connection: close 才短连

        //请求报文的connection头，如果没有，即保持默认，和版本一致
        const std::string& connection=req.getHeader("Connection"); 
        //首选与请求报文的connection一致
        bool    close=(connection=="close") || (req.getVersion()==HttpRequest::kHttp10 && connection!="Keep-Alive");

        //构造响应报文基本信息
        HttpResponse response(close);
        response.setVersion(req.getVersion()==HttpRequest::kHttp11 ? "HTTP/1.1" : "HTTP/1.0");

        //根据业务逻辑去构造响应报文其他信息
        //加入中间组件的before和after
        //先走路由，如果路由没命中则总全局，默认是404
        try
        {
            // ─── 1. before 阶段 ───
            HttpRequest mutableReq = req;             // 中间件可能要改 req
            middlewareChain_.processBefore(mutableReq);

            // ─── 2. 路由 / 兜底 ───
            if (!router_.route(mutableReq, &response))
            {
                httpCallback_(mutableReq, &response);
            }

            // ─── 3. after 阶段 ───
            middlewareChain_.processAfter(response);
        }
        catch (const HttpResponse& earlyResp)   //例如cores预检请求的before抛出了resp
        {
            // ⭐ 中间件 before 抛响应 (CORS 预检) → 直接用它
            response = earlyResp;
        }
        catch (const std::exception& e)
        {
            // 业务抛异常 → 500 兜底
            LOG_ERROR << "Handler exception: " << e.what();
            response.setStatusCode(HttpResponse::k500InternalServerError);
            response.setStatusMessage("Internal Server Error");
            response.setBody(e.what());
        }

        //序列化并发送
        Buffer buf;
        response.appendToBuffer(&buf);
        conn->send(buf.retrieveAllAsString());  //buffer自己会处理，如果没send完会设置写事件回调处理剩下部分

        //短连接，发完就关
        if(response.closeConnection())
        {
            conn->shutdown();
        }
    }
}//namespace lim