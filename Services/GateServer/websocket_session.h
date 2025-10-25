#ifndef WEBSOCKET_SESSION_H
#define WEBSOCKET_SESSION_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <memory>
#include <queue>
#include <mutex>
#include <chrono>
#include "status_client.h"
#include "../utils/redis_manager.h"
#include "../utils/database_manager.h"  // 添加这一行

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// 心跳间隔（秒）
const int HEARTBEAT_INTERVAL = 30;

class websocket_session : public std::enable_shared_from_this<websocket_session>
{
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::string userId_;
    std::string sessionId_;
    std::queue<std::string> message_queue_;
    std::mutex queue_mutex_;
    bool is_writing_ = false;
    std::chrono::steady_clock::time_point last_heartbeat_;
    std::shared_ptr<StatusClient> status_client_;
    bool client_acquired_;
    
    // Redis管理器引用
    RedisManager& redis_;
    
    // 数据库管理器引用
    DatabaseManager& db_;  // 添加这一行

public:
    // 接受并启动WebSocket会话
    explicit websocket_session(tcp::socket&& socket);

    // 启动会话
    void run(http::request<http::string_body>&& req, beast::flat_buffer&& buffer);

    // 发送消息
    void send_message(const std::string& message);
    
    // 获取共享指针
    std::shared_ptr<websocket_session> shared_this();
    
    // 设置用户ID
    void setUserId(const std::string& userId) { userId_ = userId; }
    
    // 设置会话ID
    void setSessionId(const std::string& sessionId) { sessionId_ = sessionId; }
    
    // 获取用户ID
    const std::string& getUserId() const { return userId_; }
    
    // 检查会话是否存活
    bool is_alive() const;
    
    // 更新用户状态
    void updateUserStatus(status::UserStatus status);
    
    // 设置StatusClient
    void setStatusClient(std::shared_ptr<StatusClient> client);

private:
    void do_accept(http::request<http::string_body>&& req, beast::flat_buffer&& buffer);
    void do_read();
    void on_message();
    void do_write();
    void fail(beast::error_code ec, char const* what);
    
    // 心跳机制
    void start_heartbeat();
    void handle_heartbeat(const beast::error_code& ec);
    
    // 消息处理
    void handle_text_message(const std::string& message);  // 添加这一行
};

#endif // WEBSOCKET_SESSION_H