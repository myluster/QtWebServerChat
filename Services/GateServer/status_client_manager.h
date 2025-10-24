#ifndef STATUS_CLIENT_MANAGER_H
#define STATUS_CLIENT_MANAGER_H

#include <memory>
#include <vector>
#include <mutex>
#include "status_client.h"

// StatusClient单例管理器
// 用于创建和管理全局共享的StatusClient实例池
class StatusClientManager {
public:
    // 获取单例实例
    static StatusClientManager& getInstance();
    
    // 初始化管理器，创建指定数量的StatusClient实例
    void initialize(size_t pool_size = 4, const std::string& server_address = "localhost:50051");
    
    // 获取一个StatusClient实例（从池中获取）
    std::shared_ptr<StatusClient> acquireClient();
    
    // 归还StatusClient实例到池中
    void releaseClient(std::shared_ptr<StatusClient> client);
    
    // 检查管理器是否已初始化
    bool isInitialized() const { return initialized_; }

private:
    StatusClientManager();
    ~StatusClientManager();
    
    // 禁止拷贝和赋值
    StatusClientManager(const StatusClientManager&) = delete;
    StatusClientManager& operator=(const StatusClientManager&) = delete;
    
    // 状态客户端池
    std::vector<std::shared_ptr<StatusClient>> client_pool_;
    
    // 用于线程安全的互斥锁
    mutable std::mutex pool_mutex_;
    
    // 服务器地址
    std::string server_address_;
    
    // 是否已初始化
    bool initialized_;
};

#endif // STATUS_CLIENT_MANAGER_H