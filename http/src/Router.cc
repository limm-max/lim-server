//Router.cc

#include "Router.h"
#include "Logger.h"

namespace lim
{
    //——————————————路由表注册接口——————————————————————————
    //静态路由HandlerCallback注册
    void Router::registerCallback(HttpRequest::Method method,
                              const std::string& path,
                              HandlerCallback callback)
    {
        RouteKey key{method,path};
        staticCallbacks_[key]=callback;
    }

    //静态路由Handler注册
    void Router::registerHandler(HttpRequest::Method method,
                             const std::string& path,
                             HandlerPtr handler)
    {
        RouteKey key{method, path};
        staticHandlers_[key] = std::move(handler);
    }

    //动态路由(regex类型）HandlerCallback注册
   void Router::addRegexCallback(HttpRequest::Method method,
                              const std::string& pattern,
                              HandlerCallback callback)
    {
        regexCallbacks_.push_back({method, convertToRegex(pattern), std::move(callback)});
    }

    //动态路由(regex类型）Handler注册
    void Router::addRegexHandler(HttpRequest::Method method,
                             const std::string& pattern,
                             HandlerPtr handler)
    {
        regexHandlers_.push_back({method, convertToRegex(pattern), std::move(handler)});
    }


    // ─── 路由分发 ────────────
    // 查找顺序:静态 callback → 静态 handler → 动态 callback → 动态 handler
    //找到就执行handler，找不到就返回false，由默认httpcallback_接手
    bool Router::route(const HttpRequest& req,HttpResponse* resp)
    {
        RouteKey key{req.method(), req.path()};

        // 1. 静态 callback
        if (auto it = staticCallbacks_.find(key); it != staticCallbacks_.end())
        {
            it->second(req, resp);
            return true;
        }

        // 2. 静态 handler
        if (auto it = staticHandlers_.find(key); it != staticHandlers_.end())
        {
            it->second->handle(req, resp);
            return true;
        }

        //3.动态callback 正则匹配
        for(const auto& rc:regexCallbacks_)
        {
            if(rc.method!=req.method()) continue;
            std::smatch match;
            std::string pathStr=req.path();
            if(std::regex_match(pathStr,match,rc.pattern))
            {
                //拷贝一份新的req因为后续好更改rea中的PathParameters
                HttpRequest newReq(req);    //默认拷贝
                extractPathParameters(match,newReq);
                rc.callback(newReq,resp);   //将匹配到的内容作为参数传递给callback
                return true;
            }

        }

        // 4. 动态 handler
        for (const auto& rh : regexHandlers_)
        {
            if (rh.method != req.method()) continue;
            std::smatch match;
            std::string pathStr = req.path();
            if (std::regex_match(pathStr, match, rh.pattern))
            {
                HttpRequest newReq(req);
                extractPathParameters(match, newReq);
                rh.handler->handle(newReq, resp);
                return true;
            }
        }

        return false;

    }

    // ─── 工具 ────────────────
    // "/user/:id/profile" → "^/user/([^/]+)/profile$"
    //   :id 替换成捕获组 ([^/]+) (匹配除 / 外任意非空字符)
    //   首尾加 ^ $ 确保完整匹配
    std::regex Router::convertToRegex(const std::string& pattern)
{
    std::string regexStr = "^" +
        std::regex_replace(pattern, std::regex(R"(/:([^/]+))"), R"(/([^/]+))") +
        "$";
    return std::regex(regexStr);
}

// 把捕获组按顺序塞进 pathParameters
//   "/user/42/order/99" 匹配 "/user/:userId/order/:orderId"
//   → param1=42, param2=99
//
// 注意:这里只用 param1/param2/... 编号,没拿到原始的 :userId 名字
// 工程化做法是同时记下捕获组的名字 (需要 std::regex_iterator 多解析一遍 pattern)
// Day 14 先用编号方式,简单清楚。业务里用 req.getPathParameter("param1") 取
void Router::extractPathParameters(const std::smatch& match, HttpRequest& req)
{
    // match[0] 是整个匹配的路径,从 match[1] 开始才是捕获组
    for (size_t i = 1; i < match.size(); ++i)
    {
        req.setPathParameter("param" + std::to_string(i), match[i].str());
    }
}
}//namespace lim