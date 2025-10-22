#ifndef WEBSOCKET_SESSION_H
#define WEBSOCKET_SESSION_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class websocket_session : public std::enable_shared_from_this<websocket_session>
{
private:
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string username_;
    std::string userId_;  // 添加用户ID字段

public:
    // 构造函数需要套接字
    explicit websocket_session(tcp::socket&& socket);

    // 启动会话
    void run(beast::flat_buffer&& buffer);

    // 获取一个指向自身的共享指针
    std::shared_ptr<websocket_session> shared_this();

    // 设置用户ID
    void setUserId(const std::string& userId) { userId_ = userId; }

private:
    void do_accept(beast::flat_buffer&& buffer);
    void do_read();
    void on_message();
    void send_message(const std::string& message);
    void fail(beast::error_code ec, char const* what);
};

#endif // WEBSOCKET_SESSION_H