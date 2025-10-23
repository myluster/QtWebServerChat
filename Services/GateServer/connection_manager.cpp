#include "connection_manager.h"
#include <algorithm>
#include <iostream>

/**
 * 添加用户连接
 * @param userId 用户ID
 * @param sessionId 会话ID
 * @param ipAddress 客户端IP地址
 */
void ConnectionManager::addConnection(const std::string& userId, const std::string& sessionId, const std::string& ipAddress) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 使用 insert 或 emplace 来避免需要默认构造函数
    userSessions_[userId].emplace(sessionId, SessionInfo(sessionId, ipAddress));
    sessionToUser_[sessionId] = userId;
}

/**
 * 移除用户连接
 * @param userId 用户ID
 * @param sessionId 会话ID
 */
void ConnectionManager::removeConnection(const std::string& userId, const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto userIt = userSessions_.find(userId);
    if (userIt != userSessions_.end()) {
        userIt->second.erase(sessionId);
        // 如果该用户没有其他会话，移除该用户记录
        if (userIt->second.empty()) {
            userSessions_.erase(userIt);
        }
    }
    
    // 从反向映射中移除
    sessionToUser_.erase(sessionId);
}

/**
 * 检查用户是否在线
 * @param userId 用户ID
 * @return true表示在线，false表示离线
 */
bool ConnectionManager::isUserOnline(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return userSessions_.find(userId) != userSessions_.end();
}

/**
 * 获取所有在线用户列表
 * @return 在线用户ID列表
 */
std::vector<std::string> ConnectionManager::getOnlineUsers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> users;
    users.reserve(userSessions_.size());
    
    for (const auto& pair : userSessions_) {
        users.push_back(pair.first);
    }
    
    return users;
}

/**
 * 获取用户的会话数量
 * @param userId 用户ID
 * @return 会话数量
 */
size_t ConnectionManager::getSessionCount(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = userSessions_.find(userId);
    if (it != userSessions_.end()) {
        return it->second.size();
    }
    return 0;
}

/**
 * 更新会话活动时间
 * @param userId 用户ID
 * @param sessionId 会话ID
 */
void ConnectionManager::updateSessionActivity(const std::string& userId, const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto userIt = userSessions_.find(userId);
    if (userIt != userSessions_.end()) {
        auto sessionIt = userIt->second.find(sessionId);
        if (sessionIt != userIt->second.end()) {
            sessionIt->second.lastActivity = std::chrono::steady_clock::now();
        }
    }
}

/**
 * 清理过期会话（超过指定秒数未活动的会话）
 * @param timeoutSeconds 超时时间（秒）
 */
void ConnectionManager::cleanupExpiredSessions(int timeoutSeconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    std::vector<std::pair<std::string, std::string>> expiredSessions;
    
    // 查找过期会话
    for (auto& userPair : userSessions_) {
        for (auto& sessionPair : userPair.second) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - sessionPair.second.lastActivity).count();
            if (elapsed > timeoutSeconds) {
                expiredSessions.emplace_back(userPair.first, sessionPair.first);
            }
        }
    }
    
    // 移除过期会话
    for (const auto& expired : expiredSessions) {
        removeConnection(expired.first, expired.second);
        std::cout << "Removed expired session for user: " << expired.first << std::endl;
    }
}