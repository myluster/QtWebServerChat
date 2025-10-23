#include "http_session.h"
#include "websocket_session.h"
#include "../utils/crypto_utils.h"
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <random>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include "../utils/database_manager.h"
#include "websocket_manager.h" 
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <openssl/rand.h>
#include <thread>
#include <mutex>

// 静态成员初始化
std::unordered_map<std::string, std::pair<int, std::chrono::steady_clock::time_point>> RateLimiter::request_counts_;
std::mutex RateLimiter::mutex_;

// 请求频率限制实现
bool RateLimiter::is_allowed(const std::string& client_ip, int max_requests, int window_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto it = request_counts_.find(client_ip);
    
    if (it == request_counts_.end()) {
        // 第一次请求，允许并记录
        request_counts_[client_ip] = {1, now};
        return true;
    }
    
    auto& [count, last_request_time] = it->second;
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_request_time).count();
    
    if (elapsed > window_seconds) {
        // 窗口已过期，重置计数
        count = 1;
        last_request_time = now;
        return true;
    }
    
    if (count < max_requests) {
        // 在限制内，增加计数
        count++;
        return true;
    }
    
    // 超过限制
    return false;
}

// 令牌生成函数 - 增强安全性
std::string generate_token(const std::string& userId) {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    
    // 添加随机盐值以增加安全性
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    int salt = dis(gen);
    
    // 添加客户端特定信息
    std::stringstream ss;
    ss << "token_" << userId << "_" << nanoseconds << "_" << salt;
    return ss.str();
}

// 令牌验证函数 - 增强安全性
bool verify_token(const std::string& token, std::string& userId) {
    // 检查令牌是否为空
    if (token.empty()) {
        std::cerr << "Token verification failed: Token is empty" << std::endl;
        return false;
    }
    
    // 检查令牌前缀
    if (token.substr(0, 6) != "token_") {
        std::cerr << "Token verification failed: Invalid token prefix" << std::endl;
        return false;
    }
    
    // 解析令牌格式: token_{userId}_{timestamp}_{salt}
    size_t firstUnderscore = token.find('_');
    size_t secondUnderscore = token.find('_', firstUnderscore + 1);
    size_t thirdUnderscore = token.find('_', secondUnderscore + 1);
    
    // 检查令牌格式是否正确
    if (secondUnderscore == std::string::npos || thirdUnderscore == std::string::npos) {
        std::cerr << "Token verification failed: Invalid token format" << std::endl;
        return false;
    }
    
    // 提取用户ID
    userId = token.substr(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
    
    // 验证用户ID是否为空
    if (userId.empty()) {
        std::cerr << "Token verification failed: User ID is empty" << std::endl;
        return false;
    }
    
    // 这里可以添加更多验证逻辑，比如检查时间戳是否过期等
    // 为了简化，我们只验证格式
    
    return true;
}

// 生成会话ID - 改进的安全实现
std::string http_session::generate_session_id() {
    // 使用更安全的随机数生成方法
    unsigned char buffer[32];  // 256位随机数
    if (RAND_bytes(buffer, sizeof(buffer)) != 1) {
        // 如果RAND_bytes失败，回退到原来的实现
        std::cerr << "RAND_bytes failed, falling back to UUID generator" << std::endl;
        boost::uuids::random_generator gen;
        boost::uuids::uuid id = gen();
        return boost::uuids::to_string(id);
    }
    
    // 将随机字节转换为十六进制字符串
    std::stringstream ss;
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i];
    }
    
    return ss.str();
}

void http_session::run()
{
    // 生成会话ID
    sessionId_ = generate_session_id();
    
    // 获取远程端点信息
    auto ep = stream_.socket().remote_endpoint();
    std::cout << "HTTP connection established with " << ep.address().to_string() 
              << ":" << ep.port() << ", session ID: " << sessionId_ << std::endl;

    // 开始读取请求
    do_read();
}

std::shared_ptr<http_session> http_session::shared_this()
{
    return shared_from_this();
}

void http_session::do_read()
{
    // 保持会话活动直到完成
    auto self = shared_this();

    // 从套接字读取请求
    http::async_read(stream_, buffer_, req_,
        [self](beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if(ec == http::error::end_of_stream)
                return self->do_close();
            if(ec)
                return self->fail(ec, "read");
            
            // 尝试升级到WebSocket连接
            if(websocket::is_upgrade(self->req_))
            {
                // 验证WebSocket握手
                if (!self->verify_websocket_handshake()) {
                    // 如果验证失败，返回未授权错误
                    self->send_error_response(http::status::unauthorized, "Unauthorized: Invalid token");
                    return;
                }
                
                // 启动WebSocket会话并传递已解析的请求和剩余的缓冲区数据
                auto ws = std::make_shared<websocket_session>(self->stream_.release_socket());
                ws->setUserId(self->userId_);
                
                // 添加到WebSocket管理器
                WebSocketManager::getInstance().addSession(self->userId_, ws);
                
                // 添加到连接管理器
                ConnectionManager::getInstance().addConnection(self->userId_, self->sessionId_);
                
                // 调用新的 run 重载，传递 req 和 buffer
                // self->buffer_ 中可能包含握手请求之后的数据
                ws->run(std::move(self->req_), std::move(self->buffer_));
                
                std::cout << "WebSocket session created for user ID: " << self->userId_ 
                          << ", session ID: " << self->sessionId_ << std::endl;
                return;
            }
            
            self->on_read();
        });
}

std::map<std::string, std::string> http_session::parse_post_data(const std::string& body) {
    std::map<std::string, std::string> params;
    
    size_t pos = 0;
    while (pos < body.length()) {
        size_t next = body.find('&', pos);
        if (next == std::string::npos) {
            next = body.length();
        }
        
        std::string param = body.substr(pos, next - pos);
        size_t eq_pos = param.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = param.substr(0, eq_pos);
            std::string value = param.substr(eq_pos + 1);
            
            // URL解码（简单实现）
            std::string decoded_value;
            for (size_t i = 0; i < value.length(); ++i) {
                if (value[i] == '%' && i + 2 < value.length()) {
                    std::string hex = value.substr(i + 1, 2);
                    char c = static_cast<char>(std::stoi(hex, nullptr, 16));
                    decoded_value += c;
                    i += 2;
                } else if (value[i] == '+') {
                    decoded_value += ' ';
                } else {
                    decoded_value += value[i];
                }
            }
            
            params[key] = decoded_value;
        }
        
        pos = next + 1;
    }
    
    return params;
}

// 验证请求频率限制
bool http_session::validate_rate_limit() {
    if (!RateLimiter::is_allowed(clientIp_)) {
        std::cerr << "Rate limit exceeded for client: " << clientIp_ << std::endl;
        send_error_response(http::status::too_many_requests, "Too many requests");
        return false;
    }
    return true;
}

// 验证内容类型
bool http_session::validate_content_type(const std::string& expected_type) {
    auto content_type_it = req_.find(http::field::content_type);
    if (content_type_it == req_.end()) {
        send_error_response(http::status::bad_request, "Missing Content-Type header");
        return false;
    }
    
    std::string content_type = std::string(content_type_it->value());
    if (content_type.find(expected_type) == std::string::npos) {
        send_error_response(http::status::bad_request, "Invalid Content-Type");
        return false;
    }
    
    return true;
}

// 发送响应的辅助函数
void http_session::send_response(http::status status, const std::string& content_type, const std::string& body) {
    res_.version(req_.version());
    res_.result(status);
    res_.set(http::field::server, "GateServer");
    res_.set(http::field::content_type, content_type);
    res_.keep_alive(req_.keep_alive());
    res_.body() = body;
    res_.prepare_payload();
    do_write();
}

// 发送JSON响应的辅助函数
void http_session::send_json_response(http::status status, const std::string& json) {
    send_response(status, "application/json", json);
}

// 发送错误响应的辅助函数
void http_session::send_error_response(http::status status, const std::string& message) {
    std::stringstream ss;
    ss << "{\"error\":\"" << message << "\"}";
    send_json_response(status, ss.str());
}

void http_session::handle_login() {
    std::cout << "Handling login request from " << clientIp_ << std::endl;
    
    // 验证内容类型
    if (!validate_content_type("application/x-www-form-urlencoded")) {
        return;
    }
    
    // 解析请求体中的用户名和密码
    std::string body = req_.body();
    std::cout << "Request body: " << body << std::endl;
    
    auto params = parse_post_data(body);
    std::string username = params["username"];
    std::string password = params["password"];
    
    std::cout << "Parsed credentials - Username: " << username << ", Password: [HIDDEN]" << std::endl;
    
    // 获取数据库管理器实例
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 验证用户凭据
    int userId;
    std::string storedPasswordHash;
    if (db.getUserByUsername(username, userId, storedPasswordHash)) {
        // 对输入的密码进行哈希处理
        std::string inputPasswordHash = sha256(password); // 使用新的sha256函数
        
        // 比较哈希值
        if (inputPasswordHash == storedPasswordHash) {
            std::cout << "Credentials validated successfully for user: " << username << std::endl;
            // 生成令牌
            std::string token = generate_token(std::to_string(userId));  // 用户ID为userId
            std::cout << "Generated token for user ID " << userId << std::endl;
            
            std::stringstream ss;
            ss << "{\"type\":\"login_success\",\"token\":\"" << token << "\",\"userId\":\"" << userId << "\"}";
            send_json_response(http::status::ok, ss.str());
        } else {
            std::cout << "Invalid password for user: " << username << std::endl;
            send_error_response(http::status::unauthorized, "Invalid username or password");
        }
    } else {
        std::cout << "User not found: " << username << std::endl;
        send_error_response(http::status::unauthorized, "Invalid username or password");
    }
}

void http_session::handle_register() {
    std::cout << "Handling register request" << std::endl;
    
    // 解析请求体中的用户名、密码和邮箱
    std::string body = req_.body();
    std::cout << "Request body: " << body << std::endl;
    
    auto params = parse_post_data(body);
    std::string username = params["username"];
    std::string password = params["password"];
    std::string email = params["email"]; // 添加邮箱字段
    
    // 简单验证
    if (username.empty() || password.empty()) {
        res_.version(req_.version());
        res_.result(http::status::bad_request);
        res_.set(http::field::server, "GateServer");
        res_.set(http::field::content_type, "application/json");
        res_.keep_alive(req_.keep_alive());
        res_.body() = "{\"type\":\"register_failed\",\"message\":\"Username and password are required\"}";
        res_.prepare_payload();
        std::cout << "Sending failure response: " << res_.body() << std::endl;
        do_write();
        return;
    }
    
    // 获取数据库管理器实例
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 检查用户是否已存在
    if (db.userExists(username)) {
        res_.version(req_.version());
        res_.result(http::status::conflict);
        res_.set(http::field::server, "GateServer");
        res_.set(http::field::content_type, "application/json");
        res_.keep_alive(req_.keep_alive());
        res_.body() = "{\"type\":\"register_failed\",\"message\":\"Username already exists\"}";
        res_.prepare_payload();
        std::cout << "Sending failure response: " << res_.body() << std::endl;
        do_write();
        return;
    }
    
    // 创建新用户
    int userId;
    if (db.createUser(username, password, email, userId)) {
        std::cout << "User registered successfully" << std::endl;
        res_.version(req_.version());
        res_.result(http::status::ok);
        res_.set(http::field::server, "GateServer");
        res_.set(http::field::content_type, "application/json");
        res_.keep_alive(req_.keep_alive());
        res_.body() = "{\"type\":\"register_success\",\"message\":\"User registered successfully\",\"userId\":\"" + std::to_string(userId) + "\"}";
        res_.prepare_payload();
        std::cout << "Sending success response: " << res_.body() << std::endl;
        do_write();
    } else {
        std::cout << "Failed to register user" << std::endl;
        res_.version(req_.version());
        res_.result(http::status::internal_server_error);
        res_.set(http::field::server, "GateServer");
        res_.set(http::field::content_type, "application/json");
        res_.keep_alive(req_.keep_alive());
        res_.body() = "{\"type\":\"register_failed\",\"message\":\"Failed to register user\"}";
        res_.prepare_payload();
        std::cout << "Sending failure response: " << res_.body() << std::endl;
        do_write();
    }
}

// 处理健康检查请求
void http_session::handle_health_check() {
    std::cout << "Handling health check request from " << clientIp_ << std::endl;
    
    // 获取数据库连接状态
    DatabaseManager& db = DatabaseManager::getInstance();
    bool dbConnected = db.isConnected();
    
    // 获取在线用户数
    int onlineUsers = ConnectionManager::getInstance().getOnlineUsers().size();
    
    std::stringstream ss;
    ss << "{\"status\":\"ok\",\"database_connected\":" << (dbConnected ? "true" : "false") 
       << ",\"online_users\":" << onlineUsers << ",\"timestamp\":\"" << std::time(nullptr) << "\"}";
    
    send_json_response(http::status::ok, ss.str());
}

bool http_session::verify_websocket_handshake() {
    // 检查是否有令牌参数
    std::string token;
    
    // 首先检查URL参数
    std::string target = req_.target();
    std::cout << "WebSocket handshake request target: " << target << std::endl;
    
    size_t tokenPos = target.find("token=");
    if (tokenPos != std::string::npos) {
        size_t tokenEnd = target.find("&", tokenPos);
        if (tokenEnd == std::string::npos) tokenEnd = target.length();
        token = target.substr(tokenPos + 6, tokenEnd - tokenPos - 6);
        std::cout << "Token found in URL parameter: " << token << std::endl;
    }
    
    // 如果URL中没有令牌，检查Authorization头部
    if (token.empty()) {
        auto auth_it = req_.find(http::field::authorization);
        if (auth_it != req_.end()) {
            std::string auth_header = std::string(auth_it->value());
            std::cout << "Authorization header: " << auth_header << std::endl;
            if (auth_header.length() > 7 && auth_header.substr(0, 7) == "Bearer ") {
                token = auth_header.substr(7);
                std::cout << "Token found in Authorization header: " << token << std::endl;
            }
        }
    }
    
    // 如果仍然没有令牌，检查其他可能的头部
    if (token.empty()) {
        auto token_it = req_.find("Token");
        if (token_it != req_.end()) {
            token = std::string(token_it->value());
            std::cout << "Token found in Token header: " << token << std::endl;
        }
    }
    
    // 检查WebSocket协议头部
    auto protocol_it = req_.find(http::field::sec_websocket_protocol);
    if (protocol_it != req_.end()) {
        std::string protocols = std::string(protocol_it->value());
        std::cout << "WebSocket protocols: " << protocols << std::endl;
    }
    
    // 如果没有找到令牌，记录错误并返回false
    if (token.empty()) {
        std::cerr << "No token found in request from " << clientIp_ << std::endl;
        return false;
    }
    
    // 验证令牌
    std::string userId;
    bool valid = verify_token(token, userId);
    std::cout << "Token verification result: " << (valid ? "valid" : "invalid") << std::endl;
    
    if (valid) {
        std::cout << "User ID from token: " << userId << std::endl;
        // 保存用户ID到会话中
        userId_ = userId;  // 保存用户ID以便在创建WebSocket会话时使用
    } else {
        std::cerr << "Token verification failed for token: " << token << std::endl;
    }
    
    return valid;
}

void http_session::on_read()
{
    // 检查请求频率限制
    if (!validate_rate_limit()) {
        return;
    }
    
    // 处理API请求
    if(req_.method() == http::verb::get && req_.target() == "/")
    {
        // 返回简单的API信息
        std::stringstream ss;
        ss << "{\"message\":\"GateServer API is running\",\"version\":\"1.0\",\"client_ip\":\"" << clientIp_ << "\"}";
        send_json_response(http::status::ok, ss.str());
    }
    else if(req_.method() == http::verb::get && req_.target() == "/health")
    {
        // 处理健康检查请求
        handle_health_check();
    }
    else if(req_.method() == http::verb::post && req_.target() == "/login")
    {
        // 处理登录请求
        handle_login();
    }
    else if(req_.method() == http::verb::post && req_.target() == "/register")
    {
        // 处理注册请求
        handle_register();
    }
    else
    {
        // 处理其他请求
        send_error_response(http::status::not_found, "API endpoint not found");
    }
}

void http_session::do_write()
{
    // 使响应保持活动状态，直到完成
    auto self = shared_this();

    // 发送响应
    http::async_write(stream_, res_,
        [self](beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if(ec)
                return self->fail(ec, "write");

            // 如果我们没有关闭连接，则继续读取
            if(self->res_.keep_alive())
            {
                self->req_ = {}; // 重置 req_ 对象，为下一个请求做准备
                self->do_read();
            }
            else
            {
                // 否则关闭连接
                self->do_close();
            }
        });
}

void http_session::do_close()
{
    // 从连接管理器中移除连接
    if (!userId_.empty() && !sessionId_.empty()) {
        ConnectionManager::getInstance().removeConnection(userId_, sessionId_);
        std::cout << "Removed connection for user ID: " << userId_ 
                  << ", session ID: " << sessionId_ << std::endl;
    }
    
    // 发送TCP关闭
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

    // 在这一点上，如果出现错误就忽略了，因为我们无法再做任何事情了
    if(ec == net::error::eof)
    {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }
    if(ec)
        return fail(ec, "shutdown");

    // 如果我们得到这里，那么连接就正常关闭了
}

void http_session::fail(beast::error_code ec, char const* what)
{
    // 从连接管理器中移除连接
    if (!userId_.empty() && !sessionId_.empty()) {
        ConnectionManager::getInstance().removeConnection(userId_, sessionId_);
        std::cout << "Removed connection for user ID: " << userId_ 
                  << ", session ID: " << sessionId_ << " due to error" << std::endl;
    }
    
    // 不报告对等方发起的连接重置
    if(ec == net::error::eof)
        return;

    // 记录详细的错误信息
    std::cerr << "Error in http_session from " << clientIp_ << ": " << what 
              << ": " << ec.message() << " (" << ec.value() << ")" << std::endl;
}