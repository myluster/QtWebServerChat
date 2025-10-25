#ifndef SERVICE_DISCOVERY_H
#define SERVICE_DISCOVERY_H

#include <string>
#include <vector>
#include "load_balancer.h"

class ServiceDiscovery {
public:
    static ServiceDiscovery& getInstance();
    
    // 从配置或注册中心获取服务地址
    std::vector<std::string> discoverService(const std::string& serviceName);
    
    // 定期更新服务列表
    void updateServiceList();
    
private:
    ServiceDiscovery();
    ~ServiceDiscovery();
    
    std::shared_ptr<LoadBalancer> loadBalancer_;
};

#endif // SERVICE_DISCOVERY_H