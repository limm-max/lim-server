// SessionManager.cc
#include "SessionManager.h"

#include <iomanip>
#include <sstream>

namespace lim
{

SessionManager::SessionManager(std::unique_ptr<SessionStorage> storage)
    : storage_(std::move(storage))
    , rng_(std::random_device{}())
{}

// getSession 是最核心的方法
//这里的设计是被动检查，在访问的时候才去检查是否过期
//当项目大了之后，需要设计一个额外的定时线程去主动检查清理过期的session
//   1. 请求里有 sessionId 且有效 → 返回它对应的 Session
//   2. 没有或失效 → 生成新 sessionId、创建新 Session、Set-Cookie 给响应
std::shared_ptr<Session> SessionManager::getSession(const HttpRequest& req,
                                                    HttpResponse* resp)
{
    std::string sessionId = getSessionIdFromCookie(req);
    std::shared_ptr<Session> session;

    if (!sessionId.empty())
    {
        session = storage_->load(sessionId);
    }

    if (!session || session->isExpired())
    {
        // 新建一个
        sessionId = generateSessionId();
        session = std::make_shared<Session>(sessionId, this);
        setSessionCookie(sessionId, resp);
    }
    else
    {
        session->setManager(this);
    }

    session->refresh();          // 每次访问都续期
    storage_->save(session);     // 保存(内存版无所谓,但接口形式上要)
    return session;
}

void SessionManager::destroySession(const std::string& sessionId)
{
    storage_->remove(sessionId);
}

// 生成 32 字符的十六进制随机串作为 sessionId
//   例:f3a7b8c92d4e5...
//   够用,16^32 = 2^128 种可能,碰撞概率近乎零
std::string SessionManager::generateSessionId()
{
    std::stringstream ss;
    std::uniform_int_distribution<> dist(0, 15);
    for (int i = 0; i < 32; ++i)
    {
        ss << std::hex << dist(rng_);
    }
    return ss.str();
}

// 从请求的 Cookie 头里解出 sessionId
//   Cookie 格式:"sessionId=abc...; theme=dark; lang=zh"
std::string SessionManager::getSessionIdFromCookie(const HttpRequest& req)
{
    std::string cookie = req.getHeader("Cookie");
    if (cookie.empty()) return "";

    const std::string key = "sessionId=";
    size_t pos = cookie.find(key);
    if (pos == std::string::npos) return "";

    pos += key.length();
    size_t end = cookie.find(';', pos);
    if (end == std::string::npos)
        return cookie.substr(pos);
    return cookie.substr(pos, end - pos);
}

// 给响应加 Set-Cookie 头
void SessionManager::setSessionCookie(const std::string& sessionId,
                                       HttpResponse* resp)
{
    std::string cookie = "sessionId=" + sessionId + "; Path=/; HttpOnly";
    resp->addHeader("Set-Cookie", cookie);
}

} // namespace lim