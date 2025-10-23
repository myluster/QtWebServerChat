#include "listener.h"
#include <iostream>

listener::listener(
    net::io_context& ioc,
    tcp::endpoint endpoint)
    : ioc_(ioc)
    , acceptor_(ioc)
    , accepting_(false)
    , address_(endpoint.address().to_string())
    , port_(endpoint.port())
{
    beast::error_code ec;

    // 打开接受器
    acceptor_.open(endpoint.protocol(), ec);
    if(ec)
    {
        fail(ec, "open");
        return;
    }

    // 允许地址重用
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec)
    {
        fail(ec, "set_option");
        return;
    }

    // 绑定到服务器地址
    acceptor_.bind(endpoint, ec);
    if(ec)
    {
        fail(ec, "bind");
        return;
    }

    // 开始监听连接
    acceptor_.listen(
        net::socket_base::max_listen_connections, ec);
    if(ec)
    {
        fail(ec, "listen");
        return;
    }
    
    std::cout << "Listener started on " << address_ << ":" << port_ << std::endl;
}

// 启动接受连接
void listener::run()
{
    accepting_ = true;
    do_accept();
}

// 停止接受连接
void listener::stop()
{
    accepting_ = false;
    beast::error_code ec;
    acceptor_.cancel(ec);
    acceptor_.close(ec);
    std::cout << "Listener stopped on " << address_ << ":" << port_ << std::endl;
}

void listener::do_accept()
{
    // 检查是否应该继续接受连接
    if (!accepting_) {
        return;
    }
    
    // 使监听器保持活动状态，直到完成
    auto self = shared_from_this();

    // 接受连接
    acceptor_.async_accept(
        net::make_strand(ioc_),
        [self](beast::error_code ec, tcp::socket socket)
        {
            if(ec)
            {
                // 只有在不是因为取消操作导致的错误时才报告错误
                if (ec != net::error::operation_aborted) {
                    self->fail(ec, "accept");
                }
            }
            else
            {
                // 创建会话并运行它
                std::make_shared<http_session>(std::move(socket))->run();
            }

            // 接受更多连接（如果仍在运行）
            if (self->accepting_) {
                self->do_accept();
            }
        });
}

void listener::fail(beast::error_code ec, char const* what)
{
    // 不报告操作被取消的错误（这是正常停止的一部分）
    if (ec == net::error::operation_aborted) {
        return;
    }
    
    std::cerr << "Listener error on " << address_ << ":" << port_ 
              << " - " << what << ": " << ec.message() << "\n";
}