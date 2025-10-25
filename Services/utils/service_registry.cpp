#include "service_registry.h"
#include <iostream>

ServiceRegistry::ServiceRegistry(std::shared_ptr<LoadBalancer> loadBalancer)
    : loadBalancer_(loadBalancer) {}

ServiceRegistry::~ServiceRegistry() {}

void ServiceRegistry::registerService(const std::string& serviceName, const std::string& host, int port, int weight, const std::string& metadata) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 创建服务注册信息
    ServiceRegistration registration(serviceName, host, port, weight, metadata);
    
    // 添加到注册表
    registeredServices_[serviceName].push_back(registration);
    
    // 同时添加到负载均衡器
    loadBalancer_->addServiceInstance(serviceName, host, port, weight);
    
    std::cout << "Registered service: " << serviceName << " at " << host << ":" << port 
              << " with weight " << weight << std::endl;
}

void ServiceRegistry::unregisterService(const std::string& serviceName, const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从注册表中移除
    auto& services = registeredServices_[serviceName];
    auto it = std::find_if(services.begin(), services.end(),
        [&host, &port](const ServiceRegistration& reg) {
            return reg.host == host && reg.port == port;
        });
        
    if (it != services.end()) {
        services.erase(it);
        std::cout << "Unregistered service: " << serviceName << " at " << host << ":" << port << std::endl;
    }
    
    // 同时从负载均衡器中移除
    loadBalancer_->removeServiceInstance(serviceName, host, port);
}

std::vector<ServiceRegistration> ServiceRegistry::getRegisteredServices(const std::string& serviceName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = registeredServices_.find(serviceName);
    if (it == registeredServices_.end()) {
        return {};
    }
    
    return it->second;
}

std::map<std::string, std::vector<ServiceRegistration>> ServiceRegistry::getAllRegisteredServices() {
    std::lock_guard<std::mutex> lock(mutex_);
    return registeredServices_;
}

std::vector<std::shared_ptr<ServiceInstance>> ServiceRegistry::getServiceInstances(const std::string& serviceName) {
    // 在实际应用中，这里应该查询存储的服务信息
    // 为了简化示例，我们返回所有已知的服务实例
    // 注意：这只是一个简化的实现，实际应用中应根据服务名筛选
    return {};  // 返回空向量，因为我们没有真正的服务存储机制
}