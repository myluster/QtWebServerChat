#include "listener.h"
#include <iostream>

listener::listener(
    net::io_context& ioc,
    tcp::endpoint endpoint)
    : ioc_(ioc)
    , acceptor_(ioc)
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
}

// 启动接受连接
void listener::run()
{
    do_accept();
}

void listener::do_accept()
{
    // 使监听器保持活动状态，直到完成
    auto self = shared_from_this();

    // 接受连接
    acceptor_.async_accept(
        net::make_strand(ioc_),
        [self](beast::error_code ec, tcp::socket socket)
        {
            if(ec)
            {
                self->fail(ec, "accept");
            }
            else
            {
                // 创建会话并运行它
                std::make_shared<http_session>(std::move(socket))->run();
            }

            // 接受更多连接
            self->do_accept();
        });
}

void listener::fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}