#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <map>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// 简单的令牌验证函数声明
bool verify_token(const std::string& token, std::string& userId);
std::string generate_token(const std::string& userId);

class http_session : public std::enable_shared_from_this<http_session>
{
private:
    // 这是用于发送和接收消息的流套接字
    beast::tcp_stream stream_;
    // 缓冲区保存从客户端读取的数据
    beast::flat_buffer buffer_;
    // 保存从客户端读取的消息
    http::request<http::string_body> req_;
    // 发送给客户端的响应消息
    http::response<http::string_body> res_;

public:
    // 构造函数需要套接字
    explicit http_session(tcp::socket&& socket);

    // 启动会话
    void run();

    // 获取一个指向自身的共享指针
    std::shared_ptr<http_session> shared_this();

private:
    void do_read();
    void on_read();
    void do_write();
    void do_close();
    void fail(beast::error_code ec, char const* what);
    
    // 添加处理登录的方法
    void handle_login();
    
    // 添加处理注册的方法
    void handle_register();
    
    // 添加验证WebSocket握手的方法
    bool verify_websocket_handshake();
    
    // 解析POST请求体中的参数
    std::map<std::string, std::string> parse_post_data(const std::string& body);
};

#endif // HTTP_SESSION_H