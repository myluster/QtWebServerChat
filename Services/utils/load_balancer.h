#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <map>
#include "logger.h"

// 服务实例信息
struct ServiceInstance {
    std::string serviceName;  // 服务名称
    std::string host;         // 主机地址
    int port;                 // 端口号
    bool isHealthy;           // 健康状态
    int weight;               // 权重（用于加权负载均衡算法）
    
    ServiceInstance(const std::string& name, const std::string& h, int p)
        : serviceName(name), host(h), port(p), isHealthy(true), weight(1) {}
};

class LoadBalancer {
public:
    // 单例模式：获取实例
    static LoadBalancer& getInstance();
    
    // 删除拷贝构造函数和赋值操作符，确保单例唯一性
    LoadBalancer(const LoadBalancer&) = delete;
    LoadBalancer& operator=(const LoadBalancer&) = delete;

    // 添加服务实例
    void addServiceInstance(const std::string& serviceName, const std::string& host, int port, int weight = 1);
    
    // 移除服务实例
    void removeServiceInstance(const std::string& serviceName, const std::string& host, int port);
    
    // 获取下一个健康的服务实例（支持多种算法）
    std::shared_ptr<ServiceInstance> getNextHealthyInstance(const std::string& serviceName, const std::string& algorithm = "round_robin");
    
    // 更新服务实例的健康状态
    void updateHealthStatus(const std::string& serviceName, const std::string& host, int port, bool isHealthy);
    
    // 获取指定服务的所有实例
    std::vector<std::shared_ptr<ServiceInstance>> getServiceInstances(const std::string& serviceName);

private:
    LoadBalancer();  // 私有化构造函数
    ~LoadBalancer(); // 私有化析构函数

    // 轮询算法
    std::shared_ptr<ServiceInstance> getNextInstanceRoundRobin(const std::vector<std::shared_ptr<ServiceInstance>>& instances);
    
    // 加权轮询算法
    std::shared_ptr<ServiceInstance> getNextInstanceWeightedRoundRobin(const std::vector<std::shared_ptr<ServiceInstance>>& instances);
    
    // 最少连接数算法（简化实现）
    std::shared_ptr<ServiceInstance> getNextInstanceLeastConnections(const std::vector<std::shared_ptr<ServiceInstance>>& instances);
    
    // 存储所有服务实例
    std::map<std::string, std::vector<std::shared_ptr<ServiceInstance>>> serviceInstances_;
    
    // 用于轮询算法的索引
    std::map<std::string, size_t> currentIndices_;
    
    // 线程安全的互斥锁
    std::mutex mutex_;
};

#endif // LOAD_BALANCER_H