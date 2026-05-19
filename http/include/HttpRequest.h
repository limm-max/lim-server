// HttpRequest.h
//
// HttpRequest = 一个被解析完的 HTTP 请求对象。
// 它是纯数据容器:HttpContext 解析字节流后,把结果填进来,
// 业务回调拿到这个对象,通过 getter 取信息。

#pragma once

#include "Timestamp.h"

#include<string>
#include<map>
#include<unordered_map>
#include<assert.h>
#include<stdio.h>

namespace lim
{
class HttpRequest
{
public:
    //请求行的方法枚举，仅常见类型
    enum Method
    {
        kInvalid,
        kGet,
        kPost,
        kHead,
        kPut,
        kDelete,
    };

    //请求行协议版本
    enum Version
    {
        kUnknown,
        kHttp10,
        kHttp11,
    };

    HttpRequest()
        :method_(kInvalid)
        ,version_(kUnknown)
        ,contentLength_(0)
        {}

    //————————版本————————————————————
    void setVersion(Version v){version_=v;}
    Version getVersion() const  {return version_;} 

    //——————————————方法————————————————————
    bool setMethod(const char* start, const char* end);
    Method method() const { return method_; }
    // 调试用:enum 转字符串
    const char* methodString() const;


    //——————URL path和query
    // 例如 /search?q=hello&n=10  →  query_ = "q=hello&n=10"
    void setPath(const char* start,const char* end)
    {
        path_.assign(start,end);
    }
    const std::string& path() const {return path_;}

    // ─── 路径参数(给未来动态路由用
    void setPathParameter(const std::string& key, const std::string& value)
    { pathParameters_[key] = value; }
    std::string getPathParameter(const std::string& key) const
    {
        auto it = pathParameters_.find(key);
        return it != pathParameters_.end() ? it->second : std::string();
    }

    // ─── 查询参数
    void setQueryParameters(const char* start, const char* end);   // 在 .cc 实现
    std::string getQueryParameter(const std::string& key) const
    {
        auto it = queryParameters_.find(key);
        return it != queryParameters_.end() ? it->second : std::string();
    }
    const std::unordered_map<std::string, std::string>& queryParameters() const
    { return queryParameters_; }


    // ─── 接收时间(从 onMessage 透传过来,便于打日志/监控)──
    void setReceiveTime(Timestamp t) {receiveTime_=t;}
    Timestamp receive() const {return receiveTime_;}


    //首部请求头，Header
    void addHeader(const char* start,const char* colon,const char* end);
    std::string getHeader(const std::string& field) const;
    const std::map<std::string,std::string>& headers() const {return headers_;}


    // ─── Body(POST/PUT 的请求体)──────
    void setBody(const std::string& body)   {content_=body;}
    void setBody(const char* start,const char* end)
    {
        if(start<=end)
            content_.assign(start,end-start);
    }
    const std::string& getBody() const { return content_; }

    void setContentLength(uint64_t len) { contentLength_ = len; }
    uint64_t contentLength() const { return contentLength_; }


    // ─── 复位(为下一个请求做准备,Keep-Alive 复用)──
    //that输入输出型参数，将已经解析好的requet上传到业务层，然后将原本的request初始化
    void swap(HttpRequest& that);

private:
    Method      method_;
    Version     version_;
    std::string path_;
    std::unordered_map<std::string, std::string> pathParameters_;
    std::unordered_map<std::string, std::string> queryParameters_;
    Timestamp   receiveTime_;
    std::map<std::string, std::string> headers_;
    std::string content_;          
    uint64_t    contentLength_;    
};
}//namespace lim