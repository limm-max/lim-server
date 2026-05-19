#include "HttpRequest.h"
#include <ctype.h>
namespace lim
{

    bool HttpRequest::setMethod(const char* start, const char* end)
    {
        assert(method_ == kInvalid);
        std::string m(start, end);
        if      (m == "GET")    method_ = kGet;
        else if (m == "POST")   method_ = kPost;
        else if (m == "HEAD")   method_ = kHead;
        else if (m == "PUT")    method_ = kPut;
        else if (m == "DELETE") method_ = kDelete;
        else                    method_ = kInvalid;
        return method_ != kInvalid;

    }

    const char* HttpRequest::methodString() const
    {
        switch (method_)
        {
            case kGet:    return "GET";
            case kPost:   return "POST";
            case kHead:   return "HEAD";
            case kPut:    return "PUT";
            case kDelete: return "DELETE";
            default:      return "UNKNOWN";
        }
    }

    void HttpRequest::setQueryParameters(const char* start,const char* end)
    {
        std::string s(start,end);
        size_t prev=0,pos=0;        //prev记录整个query查找的其实位置，pos记录每个&位置

        auto addPair=[&](const std::string& pair){
            size_t eq=pair.find('=');
            if(eq!=std::string::npos){      //npos标识没找到
                queryParameters_[pair.substr(0,eq)]=pair.substr(eq+1);
            }           

        };

        while((pos=s.find('&',prev))!=std::string::npos)
        {
            addPair(s.substr(prev,pos-prev));
            prev=pos+1;
        }
        addPair(s.substr(prev));
    }

    void HttpRequest::addHeader(const char* start,const char* colon,const char* end)
    {
        // 输入:["Host", ':', "127.0.0.1:8080"]
        //         start  colon            end
        // 注意:colon 后通常有空格,要 trim 掉
        std::string field(start,colon);
        ++colon;

        while(colon<end && isspace(*colon)) ++colon;    //isspace判断是否为空格
        std::string value(colon,end);

        while(!value.empty() && isspace(value[value.size()-1])) 
            value.resize(value.size()-1);
        headers_[field]=value;
    }

    std::string HttpRequest::getHeader(const std::string& field) const
    {
        std::string result;
        auto it=headers_.find(field);
        if(it!=headers_.end())   result=it->second;
        return result;
    }


    void HttpRequest::swap(HttpRequest& that)
    {
        std::swap(method_, that.method_);
        std::swap(version_, that.version_);
        path_.swap(that.path_);
        pathParameters_.swap(that.pathParameters_);
        queryParameters_.swap(that.queryParameters_);
        std::swap(receiveTime_, that.receiveTime_);
        headers_.swap(that.headers_);
        content_.swap(that.content_);
        std::swap(contentLength_, that.contentLength_);
    }
}//namespace lim