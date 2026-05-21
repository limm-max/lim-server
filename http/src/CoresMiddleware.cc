// CorsMiddleware.cc
//跨域访问：
//浏览器检查发现跨域——发送options方法预检请求——before方法检查跨域合理性，并发包含Access-Control-Allow-Origin的响应头
//——浏览器检查收到的响应通过——发正式请求——服务器响应——after方法增加Access-Control-Allow-Origin和其他相关的报头
#include "CorsMiddleware.h"
#include "Logger.h"

#include <algorithm>
#include <sstream>

namespace lim
{

CorsMiddleware::CorsMiddleware(const CorsConfig& config)
    : config_(config)
{}

// before:拦截 OPTIONS 预检请求
//
// 浏览器在发"复杂"跨域请求前 (比如带 Authorization 的 POST),
// 会先发一个 OPTIONS 请求"探路",问服务器:"这个跨域允许吗?"
// 服务器要立即响应预检,不该走真正的业务 handler。
// 实现方式:抛出已经填好的 HttpResponse,HttpServer 接住直接发回去。
void CorsMiddleware::before(HttpRequest& request)
{
    if (request.method() == HttpRequest::kOptions)
    {
        LOG_INFO << "CORS preflight request received";
        HttpResponse resp;
        handlePreflightRequest(request, resp);
        throw resp;   // ⭐ 抛 HttpResponse 中断流程
    }
}

// after:给所有正常响应加 CORS headers
void CorsMiddleware::after(HttpResponse& response)
{
    // 简化逻辑:如果配置了 "*" 就用 *,否则用第一个配置的源
    if (config_.allowedOrigins.empty()) return;

    auto& origins = config_.allowedOrigins;
    if (std::find(origins.begin(), origins.end(), "*") != origins.end())
    {
        addCorsHeaders(response, "*");
    }
    else
    {
        addCorsHeaders(response, origins[0]);
    }
}

// ─── 私有辅助方法 ──────────────────────────

bool CorsMiddleware::isOriginAllowed(const std::string& origin) const
{
    auto& origins = config_.allowedOrigins;
    if (origins.empty()) return false;
    if (std::find(origins.begin(), origins.end(), "*") != origins.end()) return true;
    return std::find(origins.begin(), origins.end(), origin) != origins.end();
}

void CorsMiddleware::handlePreflightRequest(const HttpRequest& req,
                                            HttpResponse& resp)
{
    std::string origin = req.getHeader("Origin");
    if (!isOriginAllowed(origin))
    {
        LOG_WARN << "Origin not allowed: " << origin;
        resp.setStatusCode(HttpResponse::k403Forbidden);
        resp.setStatusMessage("Forbidden");
        return;
    }

    addCorsHeaders(resp, origin);
    resp.setStatusCode(HttpResponse::k204NoContent);   // 预检通过用 204
    resp.setStatusMessage("No Content");
}

void CorsMiddleware::addCorsHeaders(HttpResponse& resp, const std::string& origin)
{
    resp.addHeader("Access-Control-Allow-Origin", origin);

    if (config_.allowCredentials)
        resp.addHeader("Access-Control-Allow-Credentials", "true");

    if (!config_.allowedMethods.empty())
        resp.addHeader("Access-Control-Allow-Methods",
                       join(config_.allowedMethods, ", "));

    if (!config_.allowedHeaders.empty())
        resp.addHeader("Access-Control-Allow-Headers",
                       join(config_.allowedHeaders, ", "));

    resp.addHeader("Access-Control-Max-Age", std::to_string(config_.maxAge));
}

std::string CorsMiddleware::join(const std::vector<std::string>& v,
                                 const std::string& sep)
{
    std::ostringstream oss;
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (i > 0) oss << sep;
        oss << v[i];
    }
    return oss.str();
}

} // namespace lim