// CorsMiddleware.h
//Middleware的一个具体中间组件，作为示例

// CORS 跨域中间件
//
// 工作:
//   • before:遇到 OPTIONS 预检请求,直接拼好响应抛出,提前终止
//   • after :给所有正常响应加 CORS headers
//
// 配置项 (CorsConfig):
//   • allowedOrigins:允许的源(域名),"*" 表示所有
//   • allowedMethods:允许的方法(GET, POST...)
//   • allowedHeaders:允许的请求头
//   • allowCredentials:是否允许带 cookie
//   • maxAge:预检结果的缓存时间(秒)

#pragma once

#include "Middleware.h"

#include <string>
#include <vector>

namespace lim
{

struct CorsConfig
{
    std::vector<std::string> allowedOrigins;    //允许的域名
    std::vector<std::string> allowedMethods;    //允许的方法
    std::vector<std::string> allowedHeaders;    //允许的报头
    bool allowCredentials = false;      //跨域请求时,允许带上 Cookie / Authorization 头 / TLS 客户端证书
    int  maxAge = 3600;

    // 工厂方法:生产默认coresconfig，默认放开所有
    static CorsConfig defaultConfig()
    {
        CorsConfig config;
        config.allowedOrigins = {"*"};
        config.allowedMethods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"};
        config.allowedHeaders = {"Content-Type", "Authorization"};
        return config;
    }
};

class CorsMiddleware : public Middleware
{
public:
    explicit CorsMiddleware(const CorsConfig& config = CorsConfig::defaultConfig());

    void before(HttpRequest& request) override;
    void after(HttpResponse& response) override;

private:
    bool isOriginAllowed(const std::string& origin) const;                      //判断跨域是否被允许
    void handlePreflightRequest(const HttpRequest& req, HttpResponse& resp);    //请求前直接处理，不需要走下一步
    void addCorsHeaders(HttpResponse& resp, const std::string& origin);         //after函数给response增加cores头
    std::string join(const std::vector<std::string>& v, const std::string& sep);

    CorsConfig config_;
};

} // namespace lim