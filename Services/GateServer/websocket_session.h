#ifndef WEBSOCKET_SESSION_H
#define WEBSOCKET_SESSION_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <chrono>
#include "status_client.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // 添加 http 命名空间
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class websocket_session : public std::enable_shared_from_this<websocket_session>
{
private:
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string username_;
    std::string userId_;
    std::string sessionId_;
    
    // 消息队列和线程安全机制
    std::queue<std::string> message_queue_;
    std::mutex queue_mutex_;
    bool is_writing_ = false;
    
    // 心跳机制
    std::chrono::steady_clock::time_point last_heartbeat_;
    static constexpr int HEARTBEAT_INTERVAL = 30; // 30秒心跳间隔
    
    // StatusClient 实例
    std::unique_ptr<StatusClient> status_client_;

public:
    // 构造函数需要套接字
    explicit websocket_session(tcp::socket&& socket);

    // 启动会话
    void run(http::request<http::string_body>&& req, beast::flat_buffer&& buffer);

    // 获取一个指向自身的共享指针
    std::shared_ptr<websocket_session> shared_this();

    // 设置用户ID和会话ID
    void setUserId(const std::string& userId) { userId_ = userId; }
    void setSessionId(const std::string& sessionId) { sessionId_ = sessionId; }
    const std::string& getUserId() const { return userId_; }
    const std::string& getSessionId() const { return sessionId_; }
    
    // 设置StatusClient
    void setStatusClient(std::unique_ptr<StatusClient> client) { status_client_ = std::move(client); }

    // 发送消息的线程安全方法
    void send_message(const std::string& message);
    
    // 心跳检查
    bool is_alive() const;

private:
    void do_accept(http::request<http::string_body>&& req, beast::flat_buffer&& buffer);

    void do_read();
    void on_message();
    void do_write();
    void fail(beast::error_code ec, char const* what);
    
    // 心跳处理
    void start_heartbeat();
    void handle_heartbeat(const beast::error_code& ec);
    
    // 状态更新方法
    void updateUserStatus(status::UserStatus status);
};

#endif // WEBSOCKET_SESSION_H