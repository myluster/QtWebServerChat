#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <chrono>
#include "connection_manager.h"
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

// 令牌验证和生成函数声明
bool verify_token(const std::string& token, std::string& userId);
std::string generate_token(const std::string& userId);

// 请求频率限制管理器
class RateLimiter {
private:
    static std::unordered_map<std::string, std::pair<int, std::chrono::steady_clock::time_point>> request_counts_;
    static std::mutex mutex_;
    
public:
    static bool is_allowed(const std::string& client_ip, int max_requests = 10, int window_seconds = 60);
};

class http_session : public std::enable_shared_from_this<http_session>
{
private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    std::string userId_;
    std::string sessionId_;
    std::string clientIp_;  // 客户端IP地址

public:
    explicit http_session(tcp::socket&& socket)
        : stream_(std::move(socket))
        , userId_("")
        , sessionId_("")
    {
        // 获取客户端IP地址
        try {
            clientIp_ = stream_.socket().remote_endpoint().address().to_string();
        } catch (...) {
            clientIp_ = "unknown";
        }
    }

    void run(); 

    std::shared_ptr<http_session> shared_this();
    const std::string& getUserId() const { return userId_; }
    const std::string& getClientIp() const { return clientIp_; }

private:
    void do_read();
    void on_read();
    void do_write();
    void do_close();
    void fail(beast::error_code ec, char const* what);
    void handle_login();
    void handle_register();
    void handle_health_check();  // 新增健康检查处理
    bool verify_websocket_handshake();
    std::map<std::string, std::string> parse_post_data(const std::string& body);
    std::string generate_session_id();
    
    // 响应发送辅助函数
    void send_response(http::status status, const std::string& content_type, const std::string& body);
    void send_json_response(http::status status, const std::string& json);
    void send_error_response(http::status status, const std::string& message);
    
    // 请求验证辅助函数
    bool validate_rate_limit();
    bool validate_content_type(const std::string& expected_type);
};

#endif // HTTP_SESSION_H