#include "status_client_manager.h"
#include <iostream>
#include <grpcpp/grpcpp.h>

// 静态实例
StatusClientManager& StatusClientManager::getInstance() {
    static StatusClientManager instance;
    return instance;
}

// 构造函数
StatusClientManager::StatusClientManager()
    : initialized_(false) {
}

// 析构函数
StatusClientManager::~StatusClientManager() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    client_pool_.clear();
}

// 初始化管理器
void StatusClientManager::initialize(size_t pool_size, const std::string& server_address) {
    if (initialized_) {
        return; // 已经初始化过了
    }
    
    server_address_ = server_address;
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // 创建指定数量的StatusClient实例
    for (size_t i = 0; i < pool_size; ++i) {
        auto channel = grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());
        auto client = std::make_shared<StatusClient>(channel);
        client_pool_.push_back(client);
    }
    
    initialized_ = true;
    std::cout << "StatusClientManager initialized with pool size: " << pool_size << std::endl;
}

// 获取一个StatusClient实例
std::shared_ptr<StatusClient> StatusClientManager::acquireClient() {
    if (!initialized_) {
        // 如果未初始化，创建一个临时实例
        auto channel = grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());
        return std::make_shared<StatusClient>(channel);
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    if (!client_pool_.empty()) {
        // 从池中取出一个实例
        auto client = client_pool_.back();
        client_pool_.pop_back();
        return client;
    } else {
        // 如果池为空，创建一个新实例
        auto channel = grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());
        return std::make_shared<StatusClient>(channel);
    }
}

// 归还StatusClient实例到池中
void StatusClientManager::releaseClient(std::shared_ptr<StatusClient> client) {
    if (!initialized_ || !client) {
        return; // 未初始化或客户端为空，直接返回
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    client_pool_.push_back(client);
}