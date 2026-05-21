// SessionStorage.cc
#include "SessionStorage.h"

namespace lim
{

void MemorySessionStorage::save(std::shared_ptr<Session> session)
{
    sessions_[session->getId()] = session;
}

std::shared_ptr<Session> MemorySessionStorage::load(const std::string& sessionId)
{
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return nullptr;

    // 加载时顺便检查过期 (过期就 remove)
    if (it->second->isExpired())
    {
        sessions_.erase(it);
        return nullptr;
    }
    return it->second;
}

void MemorySessionStorage::remove(const std::string& sessionId)
{
    sessions_.erase(sessionId);
}

} // namespace lim