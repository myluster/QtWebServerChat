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

namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// 全局变量用于信号处理
std::shared_ptr<listener> g_listener;
net::io_context* g_ioc = nullptr;

// 信号处理函数
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    
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
            std::cerr << "Usage: GateServer <address> <port>\n";
            std::cerr << "Example: GateServer 0.0.0.0 8080\n";
            return EXIT_FAILURE;
        }
        
        auto const address = net::ip::make_address(argv[1]);
        auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

        // 初始化数据库连接
        DatabaseManager& db = DatabaseManager::getInstance();
        if (!db.connect()) {
            std::cerr << "Failed to connect to database\n";
            return EXIT_FAILURE;
        }
        std::cout << "Database connected successfully\n";

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
        
        std::cout << "GateServer started on " << address.to_string() << ":" << port << std::endl;
        std::cout << "Press Ctrl+C to stop the server\n";

        // 设置信号处理
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // 运行I/O服务
        ioc.run();
        
        std::cout << "GateServer stopped\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}