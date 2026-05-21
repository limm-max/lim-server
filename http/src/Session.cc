// Session.cc
#include "Session.h"
#include "SessionManager.h"

namespace lim
{

Session::Session(const std::string& sessionId,
                 SessionManager* mgr,
                 int maxAge)
    : sessionId_(sessionId)
    , maxAge_(maxAge)
    , sessionManager_(mgr)
{
    refresh();
}

bool Session::isExpired() const
{
    return std::chrono::system_clock::now() > expiryTime_;
}

void Session::refresh()
{
    expiryTime_ = std::chrono::system_clock::now() + std::chrono::seconds(maxAge_);
}

void Session::setValue(const std::string& key, const std::string& value)
{
    data_[key] = value;
    // 修改后通知 manager 持久化 (内存版无所谓,但接口预留好)
    if (sessionManager_)
    {
        // 这里不直接调,留给 manager 自己决定何时持久化
        // 当前简化:啥也不做,反正是 in-memory,引用就是最新的
    }
}

std::string Session::getValue(const std::string& key) const
{
    auto it = data_.find(key);
    return it != data_.end() ? it->second : std::string();
}

void Session::remove(const std::string& key)
{
    data_.erase(key);
}

void Session::clear()
{
    data_.clear();
}

} // namespace lim