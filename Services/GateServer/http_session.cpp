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

// 简单的令牌生成函数
std::string generate_token(const std::string& userId) {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    
    std::stringstream ss;
    ss << "token_" << userId << "_" << nanoseconds;
    return ss.str();
}

// 简单的令牌验证函数
bool verify_token(const std::string& token, std::string& userId) {
    if (token.substr(0, 6) != "token_") {
        return false;
    }
    
    // 解析令牌格式: token_{userId}_{timestamp}
    size_t firstUnderscore = token.find('_');
    size_t secondUnderscore = token.find('_', firstUnderscore + 1);
    
    if (secondUnderscore == std::string::npos) {
        return false;
    }
    
    userId = token.substr(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
    return !userId.empty();
}

http_session::http_session(tcp::socket&& socket)
    : stream_(std::move(socket))
{
}

void http_session::run()
{
    // 我们需要知道我们的端点，以便在WebSocket升级握手期间使用它。
    auto ep = stream_.socket().remote_endpoint();
    std::cout << "HTTP connection established with " << ep.address().to_string() << ":" << ep.port() << std::endl;

    // 读取请求
    do_read();
}

std::shared_ptr<http_session> http_session::shared_this()
{
    return shared_from_this();
}

void http_session::do_read()
{
    // 使响应保持活动状态，直到完成
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
                    self->res_.version(self->req_.version());
                    self->res_.result(http::status::unauthorized);
                    self->res_.set(http::field::server, "GateServer");
                    self->res_.set(http::field::content_type, "application/json");
                    self->res_.keep_alive(self->req_.keep_alive());
                    self->res_.body() = "{\"error\":\"Unauthorized: Invalid token\"}";
                    self->res_.prepare_payload();
                    self->do_write();
                    return;
                }
                
                // 创建一个新的缓冲区并移动数据
                beast::flat_buffer buffer_copy;
                buffer_copy.commit(self->buffer_.size());
                boost::asio::buffer_copy(buffer_copy.prepare(self->buffer_.size()), self->buffer_.data());
                buffer_copy.commit(self->buffer_.size());
                
                // 清空当前缓冲区
                self->buffer_.consume(self->buffer_.size());
                
                // 启动WebSocket会话
                std::make_shared<websocket_session>(self->stream_.release_socket())->run(std::move(buffer_copy));
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

void http_session::handle_login() {
    std::cout << "Handling login request" << std::endl;
    
    // 解析请求体中的用户名和密码
    std::string body = req_.body();
    std::cout << "Request body: " << body << std::endl;
    
    auto params = parse_post_data(body);
    std::string username = params["username"];
    std::string password = params["password"];
    
    std::cout << "Parsed credentials - Username: " << username << ", Password: " << password << std::endl;
    
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
            std::cout << "Credentials validated successfully" << std::endl;
            // 生成令牌
            std::string token = generate_token(std::to_string(userId));  // 用户ID为userId
            std::cout << "Generated token: " << token << std::endl;
            
            res_.version(req_.version());
            res_.result(http::status::ok);
            res_.set(http::field::server, "GateServer");
            res_.set(http::field::content_type, "application/json");
            res_.keep_alive(req_.keep_alive());
            res_.body() = "{\"type\":\"login_success\",\"token\":\"" + token + "\",\"userId\":\"" + std::to_string(userId) + "\"}";
            res_.prepare_payload();
            std::cout << "Sending success response: " << res_.body() << std::endl;
        } else {
            std::cout << "Invalid password" << std::endl;
            res_.version(req_.version());
            res_.result(http::status::unauthorized);
            res_.set(http::field::server, "GateServer");
            res_.set(http::field::content_type, "application/json");
            res_.keep_alive(req_.keep_alive());
            res_.body() = "{\"type\":\"login_failed\",\"message\":\"Invalid username or password\"}";
            res_.prepare_payload();
            std::cout << "Sending failure response: " << res_.body() << std::endl;
        }
    } else {
        std::cout << "User not found" << std::endl;
        res_.version(req_.version());
        res_.result(http::status::unauthorized);
        res_.set(http::field::server, "GateServer");
        res_.set(http::field::content_type, "application/json");
        res_.keep_alive(req_.keep_alive());
        res_.body() = "{\"type\":\"login_failed\",\"message\":\"Invalid username or password\"}";
        res_.prepare_payload();
        std::cout << "Sending failure response: " << res_.body() << std::endl;
    }
}

void http_session::handle_register() {
    std::cout << "Handling register request" << std::endl;
    
    // 解析请求体中的用户名和密码
    std::string body = req_.body();
    std::cout << "Request body: " << body << std::endl;
    
    auto params = parse_post_data(body);
    std::string username = params["username"];
    std::string password = params["password"];
    
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
    if (db.createUser(username, password, userId)) {
        std::cout << "User registered successfully" << std::endl;
        res_.version(req_.version());
        res_.result(http::status::ok);
        res_.set(http::field::server, "GateServer");
        res_.set(http::field::content_type, "application/json");
        res_.keep_alive(req_.keep_alive());
        res_.body() = "{\"type\":\"register_success\",\"message\":\"User registered successfully\",\"userId\":\"" + std::to_string(userId) + "\"}";
        res_.prepare_payload();
        std::cout << "Sending success response: " << res_.body() << std::endl;
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
    }
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
            if (auth_header.substr(0, 7) == "Bearer ") {
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
    
    if (token.empty()) {
        std::cout << "No token found in request" << std::endl;
        return false;
    }
    
    // 验证令牌
    std::string userId;
    bool valid = verify_token(token, userId);
    std::cout << "Token verification result: " << (valid ? "valid" : "invalid") << std::endl;
    if (valid) {
        std::cout << "User ID from token: " << userId << std::endl;
        // 保存用户ID到会话中
        // 注意：在实际应用中，你可能需要将用户ID传递给websocket_session
    }
    return valid;
}

void http_session::on_read()
{
    // 关闭连接
    if(req_.method() == http::verb::get && req_.target() == "/")
    {
        // 返回一个简单的HTML页面，其中包含用于测试WebSocket连接的JavaScript
        res_.version(req_.version());
        res_.result(http::status::ok);
        res_.set(http::field::server, "GateServer");
        res_.set(http::field::content_type, "text/html");
        res_.keep_alive(req_.keep_alive());
        res_.body() = 
            "<html><head><title>GateServer</title></head><body>"
            "<h1>GateServer WebSocket Test</h1>"
            "<p>WebSocket server is running. Open browser console to test.</p>"
            "<script>"
            "var ws = new WebSocket('ws://localhost:8080/');"
            "ws.onopen = function() { console.log('WebSocket connected'); ws.send('Hello Server!'); };"
            "ws.onmessage = function(event) { console.log('Received: ' + event.data); };"
            "ws.onclose = function() { console.log('WebSocket closed'); };"
            "ws.onerror = function(error) { console.log('WebSocket error: ' + error); };"
            "</script>"
            "</body></html>";
        res_.prepare_payload();
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
        res_.version(req_.version());
        res_.result(http::status::not_found);
        res_.set(http::field::server, "GateServer");
        res_.set(http::field::content_type, "text/plain");
        res_.keep_alive(req_.keep_alive());
        res_.body() = "File not found\r\n";
        res_.prepare_payload();
    }

    // 发送响应
    do_write();
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
    // 不报告对等方发起的连接重置
    if(ec == net::error::eof)
        return;

    std::cerr << what << ": " << ec.message() << "\n";
}