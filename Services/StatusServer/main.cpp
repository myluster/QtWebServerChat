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

// gRPC服务器端口
const int GRPC_PORT = 50051;

void RunServer() {
    std::string server_address("0.0.0.0:" + std::to_string(GRPC_PORT));
    
    StatusServiceImpl service;
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOG_INFO("StatusServer listening on {}", server_address);
    
    // 等待服务器关闭
    server->Wait();
}

int main(int argc, char* argv[]) {
    try {
        LOG_INFO("Starting StatusServer...");
        
        // 运行gRPC服务器
        RunServer();
        
        LOG_INFO("StatusServer stopped");
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error: {}", e.what());
        return 1;
    }
    
    return 0;
}