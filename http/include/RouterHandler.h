//RouterHandler.h
// RouterHandler 是"复杂处理器"的基类。
//handler可携带额外数据，连接数据库等处理复杂逻辑
// 业务想用对象方式封装路由处理,继承它,实现 handle()。
// 比如 LoginHandler 内部可以有 queryUserId() / generateCookie() 等私有方法。
#pragma once

#include "HttpRequest.h"
#include  "HttpResponse.h"

namespace lim
{
class RouterHandler
{
public:
    virtual ~RouterHandler()=default;
    virtual void handle(const HttpRequest& req,HttpResponse* resp)=0;
};
}//namespace lim