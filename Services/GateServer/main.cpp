/*
 * GateServer - 网关服务器
 * 负责处理客户端连接、消息路由和负载均衡
 */

#include <iostream>
#include <boost/asio.hpp>
#include <signal.h>
#include <thread>
#include "listener.h"
#include "../utils/database_manager.h"
#include "../utils/crypto_utils.h"
#include "websocket_manager.h"
#include "connection_manager.h"
#include "status_client_manager.h"
#include "../utils/logger.h"

namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// 全局变量用于信号处理
std::shared_ptr<listener> g_listener;
net::io_context* g_ioc = nullptr;

// 信号处理函数
void signalHandler(int signal) {
    LOG_INFO("Received signal {}, shutting down gracefully...", signal);
    
    // 停止监听器
    if (g_listener) {
        g_listener->stop();
    }
    
    // 停止io_context
    if (g_ioc) {
        g_ioc->stop();
    }
    
    // 清理资源
    WebSocketManager::getInstance().cleanup();
    DatabaseManager::getInstance().disconnect();
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    try
    {
        // 检查命令行参数
        if (argc != 3)
        {
            LOG_ERROR("Usage: GateServer <address> <port>");
            LOG_ERROR("Example: GateServer 0.0.0.0 8080");
            return EXIT_FAILURE;
        }
        
        auto const address = net::ip::make_address(argv[1]);
        auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

        // 初始化数据库连接
        DatabaseManager& db = DatabaseManager::getInstance();
        if (!db.connect()) {
            LOG_ERROR("Failed to connect to database");
            return EXIT_FAILURE;
        }
        LOG_INFO("Database connected successfully");

        // 初始化StatusClient管理器
        StatusClientManager& statusManager = StatusClientManager::getInstance();
        statusManager.initialize(4, "localhost:50051"); // 创建4个实例的池
        LOG_INFO("StatusClientManager initialized");

        // io_context是我们所有I/O的入口点
        net::io_context ioc{static_cast<int>(std::thread::hardware_concurrency() > 0 ? 
                           std::thread::hardware_concurrency() : 1)};

        // 设置全局变量用于信号处理
        g_ioc = &ioc;
        
        // 创建并启动监听器，接受连接
        g_listener = std::make_shared<listener>(
            ioc,
            tcp::endpoint{address, port});
        g_listener->run();
        
        LOG_INFO("GateServer started on {}:{}", address.to_string(), port);
        LOG_INFO("Press Ctrl+C to stop the server");

        // 设置信号处理
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // 运行I/O服务
        ioc.run();
        
        LOG_INFO("GateServer stopped");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}