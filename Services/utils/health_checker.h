#ifndef HEALTH_CHECKER_H
#define HEALTH_CHECKER_H

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "load_balancer.h"
#include "service_registry.h"

class HealthChecker {
public:
    HealthChecker(std::shared_ptr<LoadBalancer> loadBalancer, std::shared_ptr<ServiceRegistry> serviceRegistry);
    ~HealthChecker();

    // 启动健康检查
    void startHealthChecking(int intervalSeconds = 30);
    
    // 停止健康检查
    void stopHealthChecking();
    
    // 执行单次健康检查
    void performHealthCheck(const std::string& serviceName, const std::string& host, int port);

private:
    std::shared_ptr<LoadBalancer> loadBalancer_;
    std::shared_ptr<ServiceRegistry> serviceRegistry_;
    std::unique_ptr<std::thread> healthCheckThread_;
    bool shouldStop_;
    
    // 实际的健康检查实现
    bool checkServiceHealth(const std::string& host, int port);
};

#endif // HEALTH_CHECKER_H