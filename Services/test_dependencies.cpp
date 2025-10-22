/*
 * 依赖库测试程序
 * 用于验证所有必需的库是否能正确链接
 */

#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <mysql/mysql.h>
#include <hiredis/hiredis.h>
#include <grpcpp/grpcpp.h>

int main() {
    std::cout << "依赖库测试程序" << std::endl;
    
    // 测试 Boost.Asio
    boost::asio::io_context io_context;
    std::cout << "✓ Boost.Asio 可用" << std::endl;
    
    // 测试 Boost.Beast
    boost::beast::http:: verb v;  // 简单声明以验证头文件可用
    std::cout << "✓ Boost.Beast 可用" << std::endl;
    
    // 测试 MySQL 客户端
    MYSQL* mysql = mysql_init(nullptr);
    if (mysql) {
        std::cout << "✓ MySQL 客户端库可用" << std::endl;
        mysql_close(mysql);
    }
    
    // 测试 Redis 客户端
    std::cout << "✓ Redis 客户端库头文件可用" << std::endl;
    // 注意：实际连接测试需要Redis服务器运行
    
    // 测试 gRPC
    grpc::CompletionQueue cq;
    std::cout << "✓ gRPC 库可用" << std::endl;
    
    std::cout << "所有依赖库均已通过编译时检查！" << std::endl;
    
    return 0;
}