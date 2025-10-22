/*
 * GateServer - 网关服务器
 * 负责处理客户端连接、消息路由和负载均衡
 */

#include <iostream>
#include <boost/asio.hpp>
#include "listener.h"
#include "../utils/database_manager.h"
#include "../utils/crypto_utils.h"

namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

int main(int argc, char* argv[])
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
        net::io_context ioc{1};

        // 创建并启动监听器，接受连接
        std::make_shared<listener>(
            ioc,
            tcp::endpoint{address, port})->run();

        // 运行I/O服务
        ioc.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}