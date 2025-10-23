/*
 * StatusServer - 状态服务器
 * 负责维护用户在线状态、好友关系和群组信息
 */

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "status_service_impl.h"
#include "../utils/database_manager.h"

// gRPC服务器端口
const int GRPC_PORT = 50051;

void RunServer() {
    std::string server_address("0.0.0.0:" + std::to_string(GRPC_PORT));
    
    StatusServiceImpl service;
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "StatusServer listening on " << server_address << std::endl;
    
    // 初始化数据库连接
    DatabaseManager& db = DatabaseManager::getInstance();
    if (!db.connect()) {
        std::cerr << "Warning: Failed to connect to database\n";
    } else {
        std::cout << "Database connected successfully\n";
    }
    
    // 等待服务器关闭
    server->Wait();
}

int main(int argc, char* argv[]) {
    try {
        std::cout << "Starting StatusServer..." << std::endl;
        
        // 运行gRPC服务器
        RunServer();
        
        std::cout << "StatusServer stopped" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}