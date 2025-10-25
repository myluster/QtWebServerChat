#include "load_balancer.h"
#include <algorithm>
#include <iostream>
#include <random>

// 静态实例指针
static LoadBalancer* instance = nullptr;
static std::mutex instanceMutex;

LoadBalancer& LoadBalancer::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (instance == nullptr) {
        instance = new LoadBalancer();
    }
    return *instance;
}

LoadBalancer::LoadBalancer() {}

LoadBalancer::~LoadBalancer() {}

void LoadBalancer::addServiceInstance(const std::string& serviceName, const std::string& host, int port, int weight) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto instance = std::make_shared<ServiceInstance>(serviceName, host, port);
    instance->weight = weight;
    
    serviceInstances_[serviceName].push_back(instance);
    currentIndices_[serviceName] = 0;
    
    LOG_INFO("Added service instance: {} at {}:{} with weight {}", serviceName, host, port, weight);
}

void LoadBalancer::removeServiceInstance(const std::string& serviceName, const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& instances = serviceInstances_[serviceName];
    auto it = std::find_if(instances.begin(), instances.end(),
        [&host, &port](const std::shared_ptr<ServiceInstance>& instance) {
            return instance->host == host && instance->port == port;
        });
        
    if (it != instances.end()) {
        instances.erase(it);
        LOG_INFO("Removed service instance: {} at {}:{}", serviceName, host, port);
    } else {
        LOG_WARN("Attempted to remove non-existent service instance: {} at {}:{}", serviceName, host, port);
    }
}

std::shared_ptr<ServiceInstance> LoadBalancer::getNextHealthyInstance(const std::string& serviceName, const std::string& algorithm) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = serviceInstances_.find(serviceName);
    if (it == serviceInstances_.end() || it->second.empty()) {
        LOG_WARN("No service instances found for service: {}", serviceName);
        return nullptr;
    }
    
    const auto& instances = it->second;
    
    // 根据算法选择实例
    if (algorithm == "weighted_round_robin") {
        return getNextInstanceWeightedRoundRobin(instances);
    } else if (algorithm == "least_connections") {
        return getNextInstanceLeastConnections(instances);
    } else {
        return getNextInstanceRoundRobin(instances);
    }
}

void LoadBalancer::updateHealthStatus(const std::string& serviceName, const std::string& host, int port, bool isHealthy) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = serviceInstances_.find(serviceName);
    if (it == serviceInstances_.end()) {
        LOG_WARN("Service not found when updating health status: {}", serviceName);
        return;
    }
    
    auto& instances = it->second;
    auto instanceIt = std::find_if(instances.begin(), instances.end(),
        [&host, &port](const std::shared_ptr<ServiceInstance>& instance) {
            return instance->host == host && instance->port == port;
        });
        
    if (instanceIt != instances.end()) {
        (*instanceIt)->isHealthy = isHealthy;
        LOG_INFO("Updated health status for {} at {}:{} to {}", serviceName, host, port, 
                 isHealthy ? "healthy" : "unhealthy");
    } else {
        LOG_WARN("Service instance not found when updating health status: {} at {}:{}", serviceName, host, port);
    }
}

std::vector<std::shared_ptr<ServiceInstance>> LoadBalancer::getServiceInstances(const std::string& serviceName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = serviceInstances_.find(serviceName);
    if (it == serviceInstances_.end()) {
        LOG_WARN("No service instances found for service: {}", serviceName);
        return {};
    }
    
    return it->second;
}

std::shared_ptr<ServiceInstance> LoadBalancer::getNextInstanceRoundRobin(const std::vector<std::shared_ptr<ServiceInstance>>& instances) {
    // 过滤出健康的实例
    std::vector<std::shared_ptr<ServiceInstance>> healthyInstances;
    for (const auto& instance : instances) {
        if (instance->isHealthy) {
            healthyInstances.push_back(instance);
        }
    }
    
    if (healthyInstances.empty()) {
        LOG_WARN("No healthy instances available for round-robin selection");
        return nullptr;
    }
    
    // 使用轮询算法选择实例
    std::string serviceName = healthyInstances[0]->serviceName;
    size_t& currentIndex = currentIndices_[serviceName];
    currentIndex = (currentIndex + 1) % healthyInstances.size();
    
    auto selectedInstance = healthyInstances[currentIndex];
    LOG_DEBUG("Selected instance via round-robin: {} at {}:{}", 
              selectedInstance->serviceName, selectedInstance->host, selectedInstance->port);
    
    return selectedInstance;
}

std::shared_ptr<ServiceInstance> LoadBalancer::getNextInstanceWeightedRoundRobin(const std::vector<std::shared_ptr<ServiceInstance>>& instances) {
    // 过滤出健康的实例
    std::vector<std::shared_ptr<ServiceInstance>> healthyInstances;
    for (const auto& instance : instances) {
        if (instance->isHealthy) {
            healthyInstances.push_back(instance);
        }
    }
    
    if (healthyInstances.empty()) {
        LOG_WARN("No healthy instances available for weighted round-robin selection");
        return nullptr;
    }
    
    // 简化的加权轮询算法
    // 实际应用中可以使用更复杂的算法，如平滑加权轮询
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    int totalWeight = 0;
    for (const auto& instance : healthyInstances) {
        totalWeight += instance->weight;
    }
    
    std::uniform_int_distribution<> dis(1, totalWeight);
    int randomWeight = dis(gen);
    
    int currentWeight = 0;
    for (const auto& instance : healthyInstances) {
        currentWeight += instance->weight;
        if (randomWeight <= currentWeight) {
            LOG_DEBUG("Selected instance via weighted round-robin: {} at {}:{} with weight {}", 
                      instance->serviceName, instance->host, instance->port, instance->weight);
            return instance;
        }
    }
    
    auto selectedInstance = healthyInstances.back();
    LOG_DEBUG("Selected instance via weighted round-robin (fallback): {} at {}:{} with weight {}", 
              selectedInstance->serviceName, selectedInstance->host, selectedInstance->port, selectedInstance->weight);
    return selectedInstance;
}

std::shared_ptr<ServiceInstance> LoadBalancer::getNextInstanceLeastConnections(const std::vector<std::shared_ptr<ServiceInstance>>& instances) {
    // 过滤出健康的实例
    std::vector<std::shared_ptr<ServiceInstance>> healthyInstances;
    for (const auto& instance : instances) {
        if (instance->isHealthy) {
            healthyInstances.push_back(instance);
        }
    }
    
    if (healthyInstances.empty()) {
        LOG_WARN("No healthy instances available for least connections selection");
        return nullptr;
    }
    
    // 简化的最少连接数算法：随机选择一个健康的实例
    // 实际应用中应该跟踪每个实例的连接数
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, healthyInstances.size() - 1);
    
    auto selectedInstance = healthyInstances[dis(gen)];
    LOG_DEBUG("Selected instance via least connections (random): {} at {}:{}", 
              selectedInstance->serviceName, selectedInstance->host, selectedInstance->port);
    
    return selectedInstance;
}