/*
 * StatusServer - 状态服务器
 * 负责维护用户在线状态、好友关系和群组信息
 */

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "status_service_impl.h"
#include "../utils/logger.h"
#include "../utils/redis_manager.h"

std::string get_cmd_option(int argc, char* argv[], const std::string& option, const std::string& default_val) {
    std::string cmd;
    for (int i = 1; i < argc; ++i) {
        cmd = argv[i];
        if (cmd.rfind(option, 0) == 0) {
            // 找到了匹配的选项 (例如: --port=)
            return cmd.substr(option.length());
        }
    }
    return default_val; // 未找到，返回默认值
}

void RunServer(const std::string& port) {
    std::string server_address("0.0.0.0:" + port);
    
    StatusServiceImpl service;
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    
    // 确保日志输出的是实际监听的端口
    LOG_INFO("StatusServer listening on {}", server_address);
    
    // 等待服务器关闭
    server->Wait();
}

int main(int argc, char* argv[]) {
    try {
        LOG_INFO("Starting StatusServer...");

        // 解析 "--port=" 参数
        // 如果在命令行中找不到 "--port="，则默认为 "50051"
        std::string port = get_cmd_option(argc, argv, "--port=", "50051");
        
        // 初始化Redis连接
        RedisManager& redis = RedisManager::getInstance();
        if (!redis.initialize("localhost", 6379, 10)) {
            LOG_WARN("Failed to connect to Redis, continuing without Redis support");
        } else {
            LOG_INFO("Redis connected successfully");
        }
        
        // 运行gRPC服务器并传入解析到的端口
        RunServer(port);
        
        // 断开Redis连接
        redis.disconnect();
        
        LOG_INFO("StatusServer stopped");
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error: {}", e.what());
        return 1;
    }
    
    return 0;
}