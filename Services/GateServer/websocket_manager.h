#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <iostream>
#include "websocket_session.h"

class WebSocketManager {
private:
    std::map<std::string, std::shared_ptr<websocket_session>> sessions_;
    std::mutex mutex_;

    // 私有构造函数（单例模式）
    WebSocketManager() = default;

public:
    // 获取单例实例
    static WebSocketManager& getInstance() {
        static WebSocketManager instance;
        return instance;
    }

    // 添加WebSocket会话
    void addSession(const std::string& userId, std::shared_ptr<websocket_session> session) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[userId] = session;
        std::cout << "Added WebSocket session for user ID: " << userId 
                  << ", total sessions: " << sessions_.size() << std::endl;
    }

    // 移除WebSocket会话
    void removeSession(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(userId);
        std::cout << "Removed WebSocket session for user ID: " << userId 
                  << ", total sessions: " << sessions_.size() << std::endl;
    }

    // 获取WebSocket会话
    std::shared_ptr<websocket_session> getSession(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(userId);
        if (it != sessions_.end()) {
            return it->second;
        }
        return nullptr;
    }

    // 获取所有活动会话数量
    size_t getActiveSessionCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.size();
    }

    // 获取所有活动用户ID
    std::vector<std::string> getActiveUserIds() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> userIds;
        for (const auto& pair : sessions_) {
            userIds.push_back(pair.first);
        }
        return userIds;
    }
    
    // 清理所有WebSocket会话
    void cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        // 通知所有会话关闭
        for (auto& pair : sessions_) {
            auto session = pair.second;
            if (session) {
                // 注意：实际清理会由会话自身处理
                std::cout << "Cleaning up session for user ID: " << pair.first << std::endl;
            }
        }
        sessions_.clear();
        std::cout << "WebSocketManager cleanup completed." << std::endl;
    }
};

#endif // WEBSOCKET_MANAGER_H