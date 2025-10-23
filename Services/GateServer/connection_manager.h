#pragma once

#include <map>
#include <set>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

/**
 * 连接状态管理器
 * 负责管理用户连接状态，包括：
 * 1. 记录用户ID与会话ID的映射关系
 * 2. 提供用户在线状态查询接口
 * 3. 提供在线用户列表查询接口
 * 4. 支持会话过期检查和清理
 */
class ConnectionManager {
public:
    // 会话信息结构体
    struct SessionInfo {
        std::string sessionId;
        std::chrono::steady_clock::time_point lastActivity;
        std::string ipAddress;
        
        SessionInfo(const std::string& id, const std::string& ip)
            : sessionId(id)
            , lastActivity(std::chrono::steady_clock::now())
            , ipAddress(ip) {}
    };

    // 获取单例实例
    static ConnectionManager& getInstance() {
        static ConnectionManager instance;
        return instance;
    }

    // 添加用户连接
    void addConnection(const std::string& userId, const std::string& sessionId, const std::string& ipAddress = "");
    
    // 移除用户连接
    void removeConnection(const std::string& userId, const std::string& sessionId);
    
    // 检查用户是否在线
    bool isUserOnline(const std::string& userId) const;
    
    // 获取所有在线用户列表
    std::vector<std::string> getOnlineUsers() const;
    
    // 获取用户的会话数量
    size_t getSessionCount(const std::string& userId) const;
    
    // 更新会话活动时间
    void updateSessionActivity(const std::string& userId, const std::string& sessionId);
    
    // 清理过期会话（超过指定秒数未活动的会话）
    void cleanupExpiredSessions(int timeoutSeconds = 3600);

private:
    ConnectionManager() = default;
    ~ConnectionManager() = default;

    // 线程安全的互斥锁
    mutable std::mutex mutex_;
    
    // 用户会话映射表: userId -> map of sessionId to SessionInfo
    std::map<std::string, std::map<std::string, SessionInfo>> userSessions_;
    
    // 会话ID到用户ID的反向映射，便于快速查找
    std::map<std::string, std::string> sessionToUser_;
};