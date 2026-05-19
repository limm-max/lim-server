// HttpContext.h
//
// HttpContext = HTTP 请求的解析状态机。
// 每个 TcpConnection 绑定一个 HttpContext (通过 conn->setContext 挂上去)。
//这样context可以持续保持这个connection的解析上下文
//
#pragma once

#include "HttpRequest.h"
namespace lim
{
class Buffer;       //TcpConnection中存放的数据
class HttpContext
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    HttpContext():state_(kExpectRequestLine){}

    //解析，返回是否成功
    bool parseRequest(Buffer* buf,Timestamp receiveTime);

    //查询request解析是否完成
    bool gotAll() const {return state_==kGotAll;}

    //解析完一个请求，复位
    void reset()
    {
        state_=kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    //取走完整的请求：判断gotAll——取走完整request——复位reset
    const HttpRequest& request() const {return request_;}
    HttpRequest& request()  {return request_;}
private:
    //解析请求行
    // 输入:[begin, end) 是一行(不含 \r\n)
    bool processRequestLine(const char* begin,const char* end);

    HttpRequestParseState state_;
    HttpRequest request_;
};
}