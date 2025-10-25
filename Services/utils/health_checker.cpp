#include "health_checker.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

HealthChecker::HealthChecker(std::shared_ptr<LoadBalancer> loadBalancer, std::shared_ptr<ServiceRegistry> serviceRegistry)
    : loadBalancer_(loadBalancer), serviceRegistry_(serviceRegistry), shouldStop_(false) {}

HealthChecker::~HealthChecker() {
    stopHealthChecking();
}

void HealthChecker::startHealthChecking(int intervalSeconds) {
    if (healthCheckThread_ && healthCheckThread_->joinable()) {
        return; // 已经在运行中
    }
    
    shouldStop_ = false;
    healthCheckThread_ = std::make_unique<std::thread>([this, intervalSeconds]() {
        while (!shouldStop_) {
            // 获取所有注册的服务
            auto allServices = serviceRegistry_->getAllRegisteredServices();
            
            // 对每个服务实例执行健康检查
            for (const auto& servicePair : allServices) {
                const std::string& serviceName = servicePair.first;
                const auto& instances = servicePair.second;
                
                for (const auto& instance : instances) {
                    performHealthCheck(serviceName, instance.host, instance.port);
                }
            }
            
            // 等待指定的时间间隔
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
        }
    });
    
    std::cout << "Started health checking with interval " << intervalSeconds << " seconds" << std::endl;
}

void HealthChecker::stopHealthChecking() {
    shouldStop_ = true;
    
    if (healthCheckThread_ && healthCheckThread_->joinable()) {
        healthCheckThread_->join();
        healthCheckThread_.reset();
    }
    
    std::cout << "Stopped health checking" << std::endl;
}

void HealthChecker::performHealthCheck(const std::string& serviceName, const std::string& host, int port) {
    bool isHealthy = checkServiceHealth(host, port);
    loadBalancer_->updateHealthStatus(serviceName, host, port, isHealthy);
    
    if (isHealthy) {
        std::cout << "Health check passed for " << serviceName << " at " << host << ":" << port << std::endl;
    } else {
        std::cout << "Health check failed for " << serviceName << " at " << host << ":" << port << std::endl;
    }
}

bool HealthChecker::checkServiceHealth(const std::string& host, int port) {
    // 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }
    
    // 设置超时
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5秒超时
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    // 解析主机地址
    struct hostent* he = gethostbyname(host.c_str());
    if (!he) {
        close(sock);
        return false;
    }
    
    // 设置地址信息
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    
    // 尝试连接
    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);
    
    return (result == 0);
}