// SessionManager.h
//
// 总管:
//   • 从请求解析 cookie 拿 sessionId
//   • 没有就生成新的 + Set-Cookie 给响应
//   • 维护 storage,过期清理

#pragma once

#include "SessionStorage.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include <memory>
#include <random>

namespace lim
{

class SessionManager
{
public:
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);

    // 主接口:从请求拿出会话 (没有就新建)
    std::shared_ptr<Session> getSession(const HttpRequest& req, HttpResponse* resp);

    // 销毁会话 (logout 用)
    void destroySession(const std::string& sessionId);

    // 显式保存 (业务改完 session 内容后调)
    void updateSession(std::shared_ptr<Session> session)
    { storage_->save(session); }

private:
    std::string generateSessionId();
    std::string getSessionIdFromCookie(const HttpRequest& req);
    void        setSessionCookie(const std::string& sessionId, HttpResponse* resp);

    std::unique_ptr<SessionStorage> storage_;
    std::mt19937 rng_;
};

} // namespace lim