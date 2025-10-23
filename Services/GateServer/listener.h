#ifndef LISTENER_H
#define LISTENER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <memory>
#include "http_session.h"
#include <atomic>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class listener : public std::enable_shared_from_this<listener>
{
private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::atomic<bool> accepting_;       // 标识是否正在接受连接
    std::string address_;               // 监听地址
    unsigned short port_;               // 监听端口

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint);

    // 启动接受连接
    void run();
    
    // 停止接受连接
    void stop();
    
    // 获取监听地址和端口
    std::string getAddress() const { return address_; }
    unsigned short getPort() const { return port_; }
    
    // 检查是否正在运行
    bool isRunning() const { return accepting_; }

private:
    void do_accept();
    void fail(beast::error_code ec, char const* what);
};

#endif // LISTENER_H