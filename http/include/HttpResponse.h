// HttpResponse.h
//   1. 数据容器 + 序列化方法 (appendToBuffer)
//   2. 不持有连接,跟 HttpRequest 一样是纯数据对象
//   3. Keep-Alive 由 closeConnection_ 字段控制
//      → true:  服务端发完就关,客户端 next 请求要重连
//      → false: 服务端发完保持连接,HttpServer 看 closeConnection() 决定是否 shutdown

#pragma once

#include<string>
#include<map>
#include<stdint.h>


namespace lim
{

class  Buffer;
class HttpResponse
{
public:
    enum HttpStatusCode // 常用状态码(只列业务最常碰到的)
    {
        kUnknown,
        k200Ok                  = 200,
        k204NoContent           = 204,
        k301MovedPermanently    = 301,
        k400BadRequest          = 400,
        k401Unauthorized        = 401,
        k403Forbidden           = 403,
        k404NotFound            = 404,
        k409Conflict            = 409,
        k500InternalServerError = 500,
    };

    // closeConnection 默认 true,因为最简单的 HTTP/1.0 短连接就是用完即关
    // 业务想 Keep-Alive 的话,自己 setCloseConnection(false)
    explicit HttpResponse(bool closeConnection=true)
        :statusCode_(kUnknown)
        ,closeConnection_(closeConnection)
        {}

    //————状态行————————————————————————
    void setVersion(const std::string& V){httpVersion_=V;}
    void setStatusCode(HttpStatusCode code) {statusCode_=code;}
    void setStatusMessage(const std::string& msg)   {statusMessage_=msg;}
    void setStatusLine(const std::string& version,HttpStatusCode code,const std::string msg)
        {
            httpVersion_=version;
            statusCode_=code;
            statusMessage_=msg;
        }
    
    HttpStatusCode statusCode() const{return statusCode_;}      //测试使用

    // ─── Keep-Alive 控制 ──────────────────
    void setCloseConnection(bool on)    {closeConnection_=on;}
    bool closeConnection() const{return closeConnection_;}

    //─── Header ──────────────────
    void addHeader(const std::string& key,const std::string& value)
    {
        headers_[key]=value;
    }

    // 常用 header 的便利方法
    void setContentType(const std::string& type)
    {
        addHeader("Content-Type", type);
    }

    void setContentLength(uint64_t length)
    {
        addHeader("Content-Length", std::to_string(length));
    }

    //——————Body——————————————————————
    void setBody(const std::string& body)  {body_=body;}
    const std::string& body() const { return body_; }

    // ─── 序列化 ────────────────────────────
    // 把整个 HttpResponse 按 HTTP 报文格式写到 buffer 里
    // 这是 HttpResponse 唯一不那么"纯数据"的方法
    //使用方法：response存成一串string到buffer中，然后使用buffer的转string方法，
    //然后系统调用send出去，没send完的string存在tcpconnection对象的outputbuffer中
    void appendToBuffer(Buffer* output) const;
private:
    std::string     httpVersion_;
    HttpStatusCode  statusCode_;  //状态行状态码
    std::string     statusMessage_; //状态码具体信息（短语）
    bool            closeConnection_;   // 控制 Connection 头,决定短链接还是长连接
    std::map<std::string,std::string>   headers_;
    std::string     body_;   
};
}//lim