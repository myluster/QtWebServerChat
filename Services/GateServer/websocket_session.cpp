#include "websocket_session.h"
#include <iostream>
#include <boost/beast/core.hpp>
#include "websocket_manager.h" 
#include <boost/asio/steady_timer.hpp>
#include "status_client_manager.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <ctime>
#include <boost/json.hpp>

websocket_session::websocket_session(tcp::socket&& socket)
    : ws_(std::move(socket))
    , userId_("")
    , sessionId_("")
    , client_acquired_(false)
    , redis_(RedisManager::getInstance())
    , db_(DatabaseManager::getInstance())
{
    // 初始化心跳时间
    last_heartbeat_ = std::chrono::steady_clock::now();
    
    // 从池中获取StatusClient实例
    auto& manager = StatusClientManager::getInstance();
    if (manager.isInitialized()) {
        status_client_ = manager.acquireClient();
        client_acquired_ = true;
    } else {
        // 如果管理器未初始化，创建临时实例
        auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
        status_client_ = std::make_shared<StatusClient>(channel);
        client_acquired_ = false;
    }
    
    // 初始化Redis连接
    redis_.initialize("localhost", 6379, 5);  // 5个连接的连接池
}


void websocket_session::run(http::request<http::string_body>&& req, beast::flat_buffer&& buffer)
{
    // 接受WebSocket握手，传递 req 和 buffer
    do_accept(std::move(req), std::move(buffer));
}

std::shared_ptr<websocket_session> websocket_session::shared_this()
{
    return shared_from_this();
}

void websocket_session::do_accept(http::request<http::string_body>&& req, beast::flat_buffer&& buffer)
{
    // 使会话保持活动状态，直到完成
    auto self = shared_this();

    // 接受WebSocket握手
    // 使用接受 req 对象的重载
    ws_.async_accept(
        req, // <--- 传入已解析的请求
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

            // 启动心跳机制
            self->start_heartbeat();
            
            net::post(
                self->ws_.get_executor(), // 获取 asio 上下文
                [self]() { // 捕获 self（复制 shared_ptr）
                    self->updateUserStatus(status::ONLINE);
                }
            );
            
            // 检查缓冲区是否已有来自 http_session 的数据
            if (self->buffer_.size() > 0) {
                // 如果有，立即处理
                self->on_message();
            } else {
                // 否则，开始读取
                self->do_read();
            }
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

            // 更新心跳时间
            self->last_heartbeat_ = std::chrono::steady_clock::now();
            
            // 处理消息
            self->on_message();
        });
}

void websocket_session::on_message()
{
    // 获取消息内容
    auto message = boost::beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());

    std::cout << "Received message from user ID " << userId_ << ": " << message << std::endl;

    // 处理不同类型的消息
    handle_text_message(message);

    // 继续读取更多消息
    do_read();
}

void websocket_session::handle_text_message(const std::string& message) {
    try {
        // 解析JSON消息
        boost::json::value jv = boost::json::parse(message);
        
        // 检查消息类型
        if (jv.is_object() && jv.as_object().contains("type")) {
            std::string type = jv.as_object().at("type").as_string().c_str();
            
            if (type == "login") {
                std::string response = "{\"type\":\"login_response\",\"success\":true,\"message\":\"登录成功\",\"userId\":\"" + userId_ + "\"}";
                send_message(response);
            } else if (type == "heartbeat") {
                std::string response = "{\"type\":\"heartbeat_response\",\"timestamp\":" + std::to_string(std::time(nullptr)) + "}";
                send_message(response);
            } else if (type == "text_message") {
                // 处理文本消息
                if (jv.as_object().contains("content") && jv.as_object().contains("receiver_id")) {
                    std::string content = jv.as_object().at("content").as_string().c_str();
                    std::string receiver_id_str = jv.as_object().at("receiver_id").as_string().c_str();
                    
                    try {
                        int receiver_id = std::stoi(receiver_id_str);
                        int sender_id = std::stoi(userId_);
                        
                        // 存储消息到数据库
                        if (db_.storeMessage(sender_id, receiver_id, content)) {
                            std::cout << "Message stored successfully from user " << sender_id << " to user " << receiver_id << std::endl;
                            
                            // 转发消息给接收者（如果在线）
                            auto receiver_session = WebSocketManager::getInstance().getSession(receiver_id_str);
                            if (receiver_session) {
                                // 构造转发消息
                                std::string forward_message = "{\"type\":\"text_message\",\"sender_id\":\"" + userId_ + 
                                                              "\",\"content\":\"" + content + "\",\"timestamp\":" + 
                                                              std::to_string(std::time(nullptr)) + "}";
                                receiver_session->send_message(forward_message);
                            }
                        } else {
                            std::cerr << "Failed to store message to database" << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error processing message: " << e.what() << std::endl;
                    }
                }
            }else if(type == "search_user"){
                if (jv.as_object().contains("query")) {
                    std::string query = jv.as_object().at("query").as_string().c_str();
                    LOG_INFO("Processing search_user request with query: {}", query);

                    // 调用 DatabaseManager
                    auto users = db_.searchUsers(query);

                    // 构建JSON响应
                    boost::json::object response;
                    response["type"] = "search_user_response";
                    boost::json::array results;

                    // 填充结果
                    for (const auto& user_pair : users) {
                        // 匹配 QML ListModel
                        boost::json::object user_obj;
                        user_obj["userId"] = std::to_string(user_pair.first);
                        user_obj["userName"] = user_pair.second;
                        user_obj["userStatus"] = "未知"; // 状态服务需要额外查询，此处简化
                        results.push_back(user_obj);
                    }
                    
                    response["results"] = results;
                    
                    LOG_DEBUG("Attempting to send search_user_response with {} results", results.size());
                    
                    // 发送响应
                    send_message(boost::json::serialize(response));
                }
            }else {
                // 其他类型的消息，回显
                std::string response = "{\"type\":\"message\",\"from\":\"server\",\"content\":\"Echo: " + message + "\"}";
                send_message(response);
            }
        } else {
            // 非JSON格式或不包含type字段的消息，回显
            std::string response = "{\"type\":\"message\",\"from\":\"server\",\"content\":\"Echo: " + message + "\"}";
            send_message(response);
        }
    } catch (const std::exception& e) {
        // JSON解析失败，回显原始消息
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        std::string response = "{\"type\":\"message\",\"from\":\"server\",\"content\":\"Echo: " + message + "\"}";
        send_message(response);
    }
}

void websocket_session::send_message(const std::string& message)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    // 将消息添加到队列
    message_queue_.push(message);
    
    // 如果当前没有正在写入，则开始写入
    if (!is_writing_) {
        is_writing_ = true;
        do_write();
    }
}

void websocket_session::do_write()
{
    // 使会话保持活动状态，直到完成
    auto self = shared_this();
    
    std::unique_lock<std::mutex> lock(self->queue_mutex_);
    
    // 如果队列为空，停止写入
    if (self->message_queue_.empty()) {
        self->is_writing_ = false;
        return;
    }
    
    // 获取队列中的第一条消息
    std::string message = self->message_queue_.front();
    self->message_queue_.pop();
    
    // 解锁队列，避免在异步操作期间锁定
    lock.unlock();
    
    // 发送消息到WebSocket
    self->ws_.async_write(
        net::buffer(message),
        [self](beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if(ec)
                return self->fail(ec, "write");
            
            // 继续处理队列中的其他消息
            std::lock_guard<std::mutex> lock(self->queue_mutex_);
            if (!self->message_queue_.empty()) {
                // 需要继续写入队列中的其他消息
                self->do_write();
            } else {
                // 队列为空，停止写入
                self->is_writing_ = false;
            }
        });
}

void websocket_session::fail(beast::error_code ec, char const* what)
{
    // 不报告对等方发起的连接重置
    if(ec == net::error::eof)
    {
        // 连接正常关闭，从管理器中移除会话
        if (!userId_.empty()) {
            // 更新用户状态为离线
            updateUserStatus(status::OFFLINE);
            WebSocketManager::getInstance().removeSession(userId_);
        }
        
        // 归还StatusClient到池中
        if (client_acquired_ && status_client_) {
            StatusClientManager::getInstance().releaseClient(status_client_);
            status_client_.reset();
            client_acquired_ = false;
        }
        return;
    }

    std::cerr << "WebSocket error for user ID " << userId_ << ": " << what << ": " << ec.message() << "\n";
    
    // 发生错误时也从管理器中移除会话
    if (!userId_.empty()) {
        // 更新用户状态为离线
        updateUserStatus(status::OFFLINE);
        WebSocketManager::getInstance().removeSession(userId_);
    }
    
    // 归还StatusClient到池中
    if (client_acquired_ && status_client_) {
        StatusClientManager::getInstance().releaseClient(status_client_);
        status_client_.reset();
        client_acquired_ = false;
    }
}

bool websocket_session::is_alive() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat_).count();
    return elapsed < (HEARTBEAT_INTERVAL * 3); // 3倍心跳间隔内认为存活
}

void websocket_session::start_heartbeat() {
    auto self = shared_this();
    
    // 创建一个定时器用于心跳
    auto timer = std::make_shared<boost::asio::steady_timer>(ws_.get_executor());
    
    // 设置定时器
    timer->expires_after(std::chrono::seconds(HEARTBEAT_INTERVAL));
    
    // 启动异步等待
    timer->async_wait([self, timer](const beast::error_code& ec) {
        if (ec) {
            // 定时器被取消或出现错误
            return;
        }
        
        // 检查会话是否仍然存活
        if (!self->is_alive()) {
            std::cerr << "WebSocket session timeout for user ID: " << self->userId_ << std::endl;
            // 更新用户状态为离线
            self->updateUserStatus(status::OFFLINE);
            // 使用 fail 来关闭会话，而不是直接返回
            self->fail(beast::error_code(boost::system::errc::timed_out, boost::system::system_category()), "heartbeat_timeout");
            return;
        }
        
        // 发送心跳消息
        std::string heartbeat_msg = "{\"type\":\"heartbeat\",\"timestamp\":" + std::to_string(std::time(nullptr)) + "}";
        self->send_message(heartbeat_msg);
        
        // 重新启动心跳
        self->start_heartbeat();
    });
}

void websocket_session::handle_heartbeat(const beast::error_code& ec) {
    if (ec) {
        // 心跳处理出现错误
        return;
    }
    
    // 更新心跳时间
    last_heartbeat_ = std::chrono::steady_clock::now();
}

void websocket_session::updateUserStatus(status::UserStatus status) {
    if (!status_client_ || userId_.empty()) {
        return;
    }
    
    try {
        std::string message;
        // 假设 sessionID_ 应该被传递
        bool result = status_client_->UpdateUserStatus(std::stoi(userId_), status, sessionId_, message);
        if (!result) {
            std::cerr << "Failed to update user status for user ID " << userId_ << ": " << message << std::endl;
        } else {
            std::cout << "Successfully updated user status for user ID " << userId_ << " to " << status << std::endl;
            
            // 同时更新Redis缓存
            std::string key = "user:status:" + userId_;
            std::string status_str;
            switch (status) {
                case status::UserStatus::OFFLINE: status_str = "OFFLINE"; break;
                case status::UserStatus::ONLINE: status_str = "ONLINE"; break;
                case status::UserStatus::AWAY: status_str = "AWAY"; break;
                case status::UserStatus::BUSY: status_str = "BUSY"; break;
                default: status_str = "OFFLINE"; break;
            }
            
            // 更新Redis中的用户状态
            redis_.hset(key, "status", status_str);
            redis_.hset(key, "last_updated", std::to_string(std::time(nullptr)));
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception while updating user status for user ID " << userId_ << ": " << e.what() << std::endl;
    }
}

// 新增方法：设置StatusClient
void websocket_session::setStatusClient(std::shared_ptr<StatusClient> client) {
    status_client_ = client;
}