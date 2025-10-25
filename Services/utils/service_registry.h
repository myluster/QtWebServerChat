#ifndef SERVICE_REGISTRY_H
#define SERVICE_REGISTRY_H

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include "load_balancer.h"

// 服务注册信息
struct ServiceRegistration {
    std::string serviceName;
    std::string host;
    int port;
    int weight;
    std::string metadata;  // 其他元数据，如版本号等
    
    ServiceRegistration(const std::string& name, const std::string& h, int p, int w = 1, const std::string& meta = "")
        : serviceName(name), host(h), port(p), weight(w), metadata(meta) {}
};

class ServiceRegistry {
public:
    ServiceRegistry(std::shared_ptr<LoadBalancer> loadBalancer);
    ~ServiceRegistry();

    // 注册服务实例
    void registerService(const std::string& serviceName, const std::string& host, int port, int weight = 1, const std::string& metadata = "");
    
    // 取消注册服务实例
    void unregisterService(const std::string& serviceName, const std::string& host, int port);
    
    // 获取指定服务的所有实例
    std::vector<ServiceRegistration> getRegisteredServices(const std::string& serviceName);
    
    // 获取所有注册的服务
    std::map<std::string, std::vector<ServiceRegistration>> getAllRegisteredServices();
    
    // 获取服务实例（用于负载均衡器）
    std::vector<std::shared_ptr<ServiceInstance>> getServiceInstances(const std::string& serviceName);

private:
    std::shared_ptr<LoadBalancer> loadBalancer_;
    std::map<std::string, std::vector<ServiceRegistration>> registeredServices_;
    std::mutex mutex_;
};

#endif // SERVICE_REGISTRY_H