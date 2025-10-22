/*
 * GateServer - 网关服务器
 * 负责处理客户端连接、消息路由和负载均衡
 */

#include <iostream>
#include <boost/asio.hpp>
#include "listener.h"

namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

int main(int argc, char* argv[])
{
    // 检查命令行参数
    if (argc != 3)
    {
        std::cerr <<
            "Usage: GateServer <address> <port>\n" <<
            "Example:\n" <<
            "    GateServer 0.0.0.0 8080\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

    try {
        // 创建io_context
        net::io_context ioc{1};

        // 创建并启动监听器
        std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();

        // 运行io_context
        std::cout << "GateServer listening on " << address.to_string() << ":" << port << std::endl;
        ioc.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}