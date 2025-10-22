#include "websocket_session.h"
#include <iostream>
#include <boost/beast/core.hpp>

websocket_session::websocket_session(tcp::socket&& socket)
    : ws_(std::move(socket))
{
}

void websocket_session::run(beast::flat_buffer&& buffer)
{
    // 接受WebSocket握手
    do_accept(std::move(buffer));
}

std::shared_ptr<websocket_session> websocket_session::shared_this()
{
    return shared_from_this();
}

void websocket_session::do_accept(beast::flat_buffer&& buffer)
{
    // 使会话保持活动状态，直到完成
    auto self = shared_this();

    // 接受WebSocket握手
    ws_.async_accept(
        [self, buffer2 = std::move(buffer)](beast::error_code ec)
        {
            if(ec)
                return self->fail(ec, "accept");

            // 将缓冲区中的数据复制到我们的缓冲区中
            if(buffer2.size() > 0) {
                // 准备缓冲区以接收数据
                auto dest = self->buffer_.prepare(buffer2.size());
                // 将数据从源缓冲区复制到目标缓冲区
                boost::asio::buffer_copy(dest, buffer2.data());
                // 提交数据到缓冲区
                self->buffer_.commit(buffer2.size());
            }

            // 读取消息
            self->do_read();
        });
}

void websocket_session::do_read()
{
    // 使会话保持活动状态，直到完成
    auto self = shared_this();

    // 从WebSocket读取消息
    ws_.async_read(
        buffer_,
        [self](beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if(ec)
                return self->fail(ec, "read");

            // 回显消息
            self->on_message();
        });
}

void websocket_session::on_message()
{
    // 获取消息内容
    auto message = boost::beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());

    std::cout << "Received message: " << message << std::endl;

    // 解析JSON消息
    // 简单处理，实际应用中应该使用JSON解析库
    if (message.find("\"type\":\"login\"") != std::string::npos) {
        // 模拟登录成功响应
        std::string response = "{\"type\":\"login_response\",\"success\":true,\"message\":\"登录成功\",\"userId\":\"" + userId_ + "\"}";
        send_message(response);
    } else {
        // 回显消息
        std::string response = "{\"type\":\"message\",\"from\":\"server\",\"content\":\"Echo: " + message + "\"}";
        send_message(response);
    }

    // 继续读取更多消息
    do_read();
}

void websocket_session::send_message(const std::string& message)
{
    // 使会话保持活动状态，直到完成
    auto self = shared_this();

    // 发送消息到WebSocket
    ws_.async_write(
        net::buffer(message),
        [self](beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if(ec)
                return self->fail(ec, "write");
        });
}

void websocket_session::fail(beast::error_code ec, char const* what)
{
    // 不报告对等方发起的连接重置
    if(ec == net::error::eof)
        return;

    std::cerr << what << ": " << ec.message() << "\n";
}