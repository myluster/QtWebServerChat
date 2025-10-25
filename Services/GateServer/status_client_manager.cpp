#include "status_client_manager.h"
#include <iostream>
#include <grpcpp/grpcpp.h>
#include "../utils/load_balancer.h"  // 添加这一行以包含LoadBalancer

// 静态实例
StatusClientManager& StatusClientManager::getInstance() {
    static StatusClientManager instance;
    return instance;
}

// 构造函数
StatusClientManager::StatusClientManager()
    : initialized_(false), server_address_("localhost:50051") {  // 初始化server_address_
}

// 析构函数
StatusClientManager::~StatusClientManager() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    client_pool_.clear();
}

// 初始化管理器
void StatusClientManager::initialize(size_t pool_size, const std::string& serviceName) {
    if (initialized_) {
        return; // 已经初始化过了
    }
    
    service_name_ = serviceName;  // 保存服务名称
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // 使用负载均衡器获取服务实例
    auto& loadBalancer = LoadBalancer::getInstance();
    
    // 创建指定数量的StatusClient实例
    for (size_t i = 0; i < pool_size; ++i) {
        // 通过负载均衡器获取服务实例
        auto instance = loadBalancer.getNextHealthyInstance(serviceName);
        if (instance) {
            std::string address = instance->host + ":" + std::to_string(instance->port);
            auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
            auto client = std::make_shared<StatusClient>(channel);
            client_pool_.push_back(client);
        }
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
        // 如果池为空，尝试通过负载均衡器创建新实例
        auto& loadBalancer = LoadBalancer::getInstance();
        auto instance = loadBalancer.getNextHealthyInstance(service_name_);
        if (instance) {
            std::string address = instance->host + ":" + std::to_string(instance->port);
            auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
            return std::make_shared<StatusClient>(channel);
        } else {
            // 如果无法通过负载均衡器获取实例，使用默认地址
            auto channel = grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());
            return std::make_shared<StatusClient>(channel);
        }
    }
}

// 归还StatusClient实例到池中
void StatusClientManager::releaseClient(std::shared_ptr<StatusClient> client) {
    if (!client) {
        return; // 客户端为空，直接返回
    }
    
    if (!initialized_) {
        // 未初始化，不需要归还到池中
        return;
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    client_pool_.push_back(client);
}