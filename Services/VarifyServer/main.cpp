/*
 * VarifyServer - 验证服务器
 * 负责用户认证、权限验证和安全检查
 */

#include <boost/asio.hpp>
#include <iostream>
#include "../utils/logger.h"
#include "../utils/load_balancer.h"

// 主函数
int main(int argc, char* argv[]) {
    try {
        LOG_INFO("Starting VarifyServer...");
        
        // 获取负载均衡器实例
        LoadBalancer& loadBalancer = LoadBalancer::getInstance();
        
        // 注册一些示例服务实例到负载均衡器
        loadBalancer.addServiceInstance("AuthService", "192.168.1.30", 8080, 2);
        loadBalancer.addServiceInstance("AuthService", "192.168.1.31", 8080, 1);
        loadBalancer.addServiceInstance("SecurityService", "192.168.1.40", 9090, 3);
        
        // 模拟使用负载均衡器选择服务实例
        auto authInstance = loadBalancer.getNextHealthyInstance("AuthService");
        if (authInstance) {
            LOG_INFO("Selected auth service instance: {}:{}", authInstance->host, authInstance->port);
        } else {
            LOG_WARN("No healthy auth service instances available");
        }
        
        auto securityInstance = loadBalancer.getNextHealthyInstance("SecurityService");
        if (securityInstance) {
            LOG_INFO("Selected security service instance: {}:{}", securityInstance->host, securityInstance->port);
        } else {
            LOG_WARN("No healthy security service instances available");
        }
        
        LOG_INFO("VarifyServer started successfully!");
        
        // 保持程序运行
        std::cin.get();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error: {}", e.what());
        return 1;
    }
    
    return 0;
}