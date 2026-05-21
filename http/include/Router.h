//https://www.example.com:8080/users/42/profile?tab=info&lang=zh#section2
//└─┬─┘   └───────┬──────┘└─┬┘└──────┬─────────┘└──────┬─────────┘└─┬───┘
//scheme       host       port      path             query        fragment
//(协议)       (域名)      (端口)    (路径)           (查询串)     (锚点)
//    /users/42/profile 多级目录，/为根目录
//静态路由：静态文件、固定特殊业务接口，如 /login、/register、/logout
//→ 用 unordered_map 查表,O(1)
//动态路由：带参数的模糊匹配，对应的是一整类具有相同业务逻辑、但参数不同的资源
//→ 用 regex 一个个试,O(n) 模式匹配
// 每类又分两种注册方式:
//   • 注册 callback (简单业务,直接传 lambda)
//   • 注册 handler 对象 (复杂业务,继承 RouterHandler)
//
#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RouterHandler.h"

#include <functional>
#include <memory>
#include <regex>    //正则表达式，用于对字符串进行模糊匹配、查找等
#include <string>
#include <unordered_map>
#include <vector>

namespace lim
{

//使用默认构造和析构（成员都是STL容器，成员自身可以自己完美构造和析构）
class Router
{
public:
    using HandlerCallback=std::function<void(const HttpRequest&,HttpResponse*)>;
    using HandlerPtr=std::shared_ptr<RouterHandler>;

    //————————————静态路由 key (method + path)————————————————
    //静态路由：path就是静态文件路径，method就是要做的动作
    struct RouteKey
    {
        HttpRequest::Method method;
        std::string path;

        //为后续map查找的时候提供依据
        bool  operator==(const RouteKey& other) const
        {
            return method==other.method && path==other.path;
        }
    };

    //为RouteKey设计hash函数
    struct RouteKeyHash
    {
        size_t operator()(const RouteKey& key) const{
            size_t h1=std::hash<int>{}(static_cast<int>(key.method));   //计算一个int的hash值
            size_t h2=std::hash<std::string>{}(key.path);
            return h1*31+h2;
        }
    };

    // ─── 注册接口 (4 种组合) ─────────────────
    // 静态 + 回调
    void registerCallback(HttpRequest::Method method,
                          const std::string& path,
                          HandlerCallback callback);

    // 静态 + Handler 对象
    void registerHandler(HttpRequest::Method method,
                         const std::string& path,
                         HandlerPtr handler);

    // 动态 + 回调
    void addRegexCallback(HttpRequest::Method method,
                          const std::string& pattern,
                          HandlerCallback callback);

    // 动态 + Handler 对象
    void addRegexHandler(HttpRequest::Method method,
                         const std::string& pattern,
                         HandlerPtr handler);
    

    // ─── 路由分发 ─────────────────
    // 返回 true:找到匹配的 handler 并执行了
    // 返回 false:没找到,HttpServer 应该返回 404
    bool route(const HttpRequest& req, HttpResponse* resp);

private:
    // 把 "/user/:id" 转成正则 "/user/([^/]+)"
    std::regex convertToRegex(const std::string& pattern);

    // 把正则匹配到的捕获组塞进 HttpRequest 的 pathParameters
    void extractPathParameters(const std::smatch& match, HttpRequest& req);

    // 动态路由的存储结构 —— 每条记录都是 (method, regex, callback/handler) 三元组
    struct RegexCallback
    {
        HttpRequest::Method method;
        std::regex          pattern;
        HandlerCallback     callback;
    };

    struct RegexHandlerObj
    {
        HttpRequest::Method method;
        std::regex          pattern;
        HandlerPtr          handler;
    };

    // 4 张表
    std::unordered_map<RouteKey, HandlerCallback, RouteKeyHash> staticCallbacks_;
    std::unordered_map<RouteKey, HandlerPtr,      RouteKeyHash> staticHandlers_;
    std::vector<RegexCallback>   regexCallbacks_;
    std::vector<RegexHandlerObj> regexHandlers_;
    

};
}//namespace lim


/*
正则式：
\d 一个数字
· 匹配任意单个字符
+数量修饰符，表示前面的字符必须出现 1 次或多次
^尖括号开头，规定字符串必须以什么开头
[]表示一个“字符集”（字符范围）
[^...]取反，表示字符集除外
$规定字符串必须以什么结尾
()捕获组，小括号里的内容既要匹配，还能将匹配出的内容提供回。

std::match_results<Iter>    匹配结果数据结构
match_results<Iter> {
    // 一个伪 vector,装多个 sub_match
    sub_match<Iter> match[0];   // 整体匹配
    sub_match<Iter> match[1];   // 捕获组 1
    sub_match<Iter> match[2];   // 捕获组 2
    ...
};
每个 sub_match 装一对迭代器:[start, end),指向原字符串里那一段。
sub_match 重载了 operator std::string(),可以隐式转 string:
match[0].str(),match[0].first,match[0].end,match.length();
*/