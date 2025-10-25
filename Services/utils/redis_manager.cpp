#include "redis_manager.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief 获取RedisManager单例实例
 * 使用局部静态变量实现线程安全的单例模式
 * @return RedisManager实例的引用
 */
RedisManager& RedisManager::getInstance() {
    static RedisManager instance;
    return instance;
}

/**
 * @brief RedisManager构造函数
 */
RedisManager::RedisManager() : initialized_(false) {
    // 初始化连接池为空
    connectionPool_.clear();
}

/**
 * @brief RedisManager析构函数
 * 清理资源，断开所有Redis连接
 */
RedisManager::~RedisManager() {
    disconnect();
}

/**
 * @brief 初始化Redis连接
 * @param host Redis服务器主机地址
 * @param port Redis服务器端口
 * @param poolSize 连接池大小
 * @return 初始化成功返回true，否则返回false
 */
bool RedisManager::initialize(const std::string& host, int port, int poolSize) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        LOG_WARN("RedisManager already initialized");
        return true;
    }
    
    host_ = host;
    port_ = port;
    poolSize_ = poolSize;
    
    // 创建连接池
    for (int i = 0; i < poolSize_; ++i) {
        redisContext* ctx = createConnection(host, port);
        if (ctx) {
            connectionPool_.push_back(ctx);
        } else {
            LOG_ERROR("Failed to create Redis connection {}/{}", i + 1, poolSize_);
            // 清理已创建的连接
            disconnect();
            return false;
        }
    }
    
    initialized_ = true;
    LOG_INFO("RedisManager initialized with {} connections to {}:{}",
             poolSize_, host_, port_);
    return true;
}

/**
 * @brief 创建Redis连接
 * @param host Redis服务器主机地址
 * @param port Redis服务器端口
 * @return Redis连接上下文指针
 */
redisContext* RedisManager::createConnection(const std::string& host, int port) {
    struct timeval timeout = { 2, 500000 }; // 2.5 seconds
    redisContext* ctx = redisConnectWithTimeout(host.c_str(), port, timeout);
    
    if (ctx == nullptr || ctx->err) {
        if (ctx) {
            LOG_ERROR("Redis connection error: {}", ctx->errstr);
            redisFree(ctx);
        } else {
            LOG_ERROR("Redis connection error: can't allocate redis context");
        }
        return nullptr;
    }
    
    LOG_INFO("Redis connection created to {}:{}", host, port);
    return ctx;
}

/**
 * @brief 断开所有Redis连接
 */
void RedisManager::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (redisContext* ctx : connectionPool_) {
        if (ctx) {
            redisFree(ctx);
        }
    }
    
    connectionPool_.clear();
    initialized_ = false;
    LOG_INFO("RedisManager disconnected");
}

/**
 * @brief 检查Redis连接状态
 * @return 连接有效返回true，否则返回false
 */
bool RedisManager::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_ || connectionPool_.empty()) {
        return false;
    }
    
    // 检查第一个连接是否有效
    redisContext* ctx = connectionPool_[0];
    return isConnectionValid(ctx);
}

/**
 * @brief 检查连接是否有效
 * @param ctx Redis连接上下文指针
 * @return 连接有效返回true，否则返回false
 */
bool RedisManager::isConnectionValid(redisContext* ctx) const {
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "PING");
    if (reply == nullptr) {
        return false;
    }
    
    bool valid = (reply->type == REDIS_REPLY_STATUS && 
                  std::string(reply->str) == "PONG");
    
    freeReplyObject(reply);
    return valid;
}

/**
 * @brief 从连接池获取连接
 * @return Redis连接上下文指针
 */
redisContext* RedisManager::getConnection() {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    if (connectionPool_.empty()) {
        // 如果连接池为空，创建新连接
        redisContext* ctx = createConnection(host_, port_);
        return ctx;
    }
    
    // 从连接池取出一个连接
    redisContext* ctx = connectionPool_.back();
    connectionPool_.pop_back();
    
    // 检查连接是否仍然有效
    if (!isConnectionValid(ctx)) {
        redisFree(ctx);
        ctx = createConnection(host_, port_);
    }
    
    return ctx;
}

/**
 * @brief 将连接归还到连接池
 * @param ctx Redis连接上下文指针
 */
void RedisManager::returnConnection(redisContext* ctx) {
    if (!ctx) return;
    
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    // 如果连接池已满，释放连接
    if (connectionPool_.size() >= (size_t)poolSize_) {
        redisFree(ctx);
        return;
    }
    
    connectionPool_.push_back(ctx);
}

// ==================== 字符串操作 ====================

bool RedisManager::set(const std::string& key, const std::string& value) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "SET %s %s", 
                                                   key.c_str(), value.c_str());
    
    bool success = (reply != nullptr && reply->type == REDIS_REPLY_STATUS);
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

bool RedisManager::get(const std::string& key, std::string& value) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "GET %s", key.c_str());
    
    bool success = false;
    if (reply && reply->type == REDIS_REPLY_STRING) {
        value = std::string(reply->str, reply->len);
        success = true;
    }
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

bool RedisManager::incr(const std::string& key, long long& result) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "INCR %s", key.c_str());
    
    bool success = false;
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        result = reply->integer;
        success = true;
    }
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

bool RedisManager::del(const std::string& key) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "DEL %s", key.c_str());
    
    bool success = (reply != nullptr && reply->type == REDIS_REPLY_INTEGER);
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

// ==================== 哈希操作 ====================

bool RedisManager::hset(const std::string& key, const std::string& field, const std::string& value) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "HSET %s %s %s", 
                                                   key.c_str(), field.c_str(), value.c_str());
    
    bool success = (reply != nullptr && 
                   (reply->type == REDIS_REPLY_INTEGER || reply->type == REDIS_REPLY_STATUS));
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

bool RedisManager::hget(const std::string& key, const std::string& field, std::string& value) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "HGET %s %s", 
                                                   key.c_str(), field.c_str());
    
    bool success = false;
    if (reply && reply->type == REDIS_REPLY_STRING) {
        value = std::string(reply->str, reply->len);
        success = true;
    }
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

bool RedisManager::hdel(const std::string& key, const std::string& field) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "HDEL %s %s", 
                                                   key.c_str(), field.c_str());
    
    bool success = (reply != nullptr && reply->type == REDIS_REPLY_INTEGER);
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

bool RedisManager::hgetall(const std::string& key, std::unordered_map<std::string, std::string>& result) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "HGETALL %s", key.c_str());
    
    bool success = false;
    if (reply && reply->type == REDIS_REPLY_ARRAY && reply->elements % 2 == 0) {
        result.clear();
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (reply->element[i]->type == REDIS_REPLY_STRING && 
                reply->element[i+1]->type == REDIS_REPLY_STRING) {
                std::string field(reply->element[i]->str, reply->element[i]->len);
                std::string value(reply->element[i+1]->str, reply->element[i+1]->len);
                result[field] = value;
            }
        }
        success = true;
    }
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

// ==================== 有序集合操作 ====================

bool RedisManager::zadd(const std::string& key, double score, const std::string& member) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "ZADD %s %f %s", 
                                                   key.c_str(), score, member.c_str());
    
    bool success = (reply != nullptr && reply->type == REDIS_REPLY_INTEGER);
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

bool RedisManager::zrange(const std::string& key, int start, int stop, std::vector<std::string>& result) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "ZRANGE %s %d %d", 
                                                   key.c_str(), start, stop);
    
    bool success = false;
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        result.clear();
        for (size_t i = 0; i < reply->elements; ++i) {
            if (reply->element[i]->type == REDIS_REPLY_STRING) {
                result.emplace_back(reply->element[i]->str, reply->element[i]->len);
            }
        }
        success = true;
    }
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

// ==================== 发布/订阅操作 ====================

bool RedisManager::publish(const std::string& channel, const std::string& message) {
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx, "PUBLISH %s %s", 
                                                   channel.c_str(), message.c_str());
    
    bool success = (reply != nullptr && reply->type == REDIS_REPLY_INTEGER);
    
    if (reply) freeReplyObject(reply);
    returnConnection(ctx);
    return success;
}

bool RedisManager::subscribe(const std::vector<std::string>& channels, 
                             std::function<void(const std::string&, const std::string&)> messageCallback) {
    if (channels.empty()) return false;
    
    redisContext* ctx = getConnection();
    if (!ctx) return false;
    
    // 构建SUBSCRIBE命令
    std::string command = "SUBSCRIBE";
    for (const auto& channel : channels) {
        command += " " + channel;
    }
    
    // 这里需要使用redisGetReply来处理订阅消息
    // 注意：订阅会阻塞当前连接，应该在单独线程中运行
    redisReply* reply;
    int status = redisAppendCommand(ctx, command.c_str());
    if (status != REDIS_OK) {
        returnConnection(ctx);
        return false;
    }
    
    // 处理订阅响应和消息
    while (true) {
        status = redisGetReply(ctx, (void**)&reply);
        if (status != REDIS_OK) {
            if (reply) freeReplyObject(reply);
            returnConnection(ctx);
            return false;
        }
        
        // 检查回复类型
        if (reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3) {
            // 解析消息: [type, channel, message]
            if (reply->element[0]->type == REDIS_REPLY_STRING &&
                reply->element[1]->type == REDIS_REPLY_STRING &&
                reply->element[2]->type == REDIS_REPLY_STRING) {
                
                std::string type(reply->element[0]->str, reply->element[0]->len);
                std::string channel(reply->element[1]->str, reply->element[1]->len);
                std::string message(reply->element[2]->str, reply->element[2]->len);
                
                // 如果是消息类型，调用回调函数
                if (type == "message" && messageCallback) {
                    messageCallback(channel, message);
                }
            }
        }
        
        freeReplyObject(reply);
    }
    
    returnConnection(ctx);
    return true;
}