// Session.h
//session是根据浏览器软件区分
// 一个会话对象:对应一个登录用户的状态。
// 内部存 KV 数据,业务往里塞 userId、username、各种业务状态。

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace lim
{

class SessionManager;   // 前置声明,Session管理器，负责解析cookie、生成sessionId、过期清理、调 storage

class Session : public std::enable_shared_from_this<Session>
{
public:
    // 构造:sessionId、管理者指针、过期秒数(默认 1 小时)
    Session(const std::string& sessionId,
            SessionManager*    manager,
            int                maxAge = 3600);

    const std::string& getId() const { return sessionId_; }

    // 过期相关
    bool isExpired() const;
    void refresh();    // 重置过期时间(每次访问都刷新)

    void setManager(SessionManager* mgr) { sessionManager_ = mgr; } 
    SessionManager* getManager() const { return sessionManager_; }

    // KV 数据操作
    void        setValue(const std::string& key, const std::string& value);
    std::string getValue(const std::string& key) const;
    void        remove(const std::string& key);
    void        clear();

private:
    std::string sessionId_;
    std::unordered_map<std::string, std::string> data_; //会话状态字典：包含id、name、事件等，业务侧重
    std::chrono::system_clock::time_point expiryTime_;  //超时时间刻
    int             maxAge_;
    SessionManager* sessionManager_;
};

} // namespace lim