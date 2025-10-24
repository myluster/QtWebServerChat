#include "database_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <cstdlib>
#include <cstring>
#include "logger.h"

// 数据库连接信息
const std::string DatabaseManager::DB_HOST = "127.0.0.1";
const std::string DatabaseManager::DB_USER = "im_user";
const std::string DatabaseManager::DB_PASSWORD = "password";
const std::string DatabaseManager::DB_NAME = "im_database";
const int DatabaseManager::DB_PORT = 3307;

DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() : connection_(nullptr), connected_(false) {
    // 初始化MySQL库
    mysql_library_init(0, nullptr, nullptr);
}

DatabaseManager::~DatabaseManager() {
    LOG_INFO("DatabaseManager destructor called");
    disconnect();
    mysql_library_end();
    LOG_INFO("DatabaseManager destructor finished");
}

void DatabaseManager::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_) {
        LOG_INFO("Disconnecting from database");
        mysql_close(connection_);
        connection_ = nullptr;
        connected_ = false;
        LOG_INFO("Disconnected from database");
    }
}

// ==========================================================
// 私有的、无锁的 _impl 函数
// ==========================================================

/**
 * @brief 私有的、无锁的连接实现
 * 假设调用者已经持有了 mutex_
 */
bool DatabaseManager::connect_impl() {
    if (connected_) {
        return true;
    }
    
    connection_ = mysql_init(nullptr);
    if (!connection_) {
        LOG_ERROR("mysql_init() failed");
        return false;
    }
    
    // 设置连接选项
    mysql_options(connection_, MYSQL_OPT_CONNECT_TIMEOUT, "10");
    mysql_options(connection_, MYSQL_OPT_READ_TIMEOUT, "10");
    mysql_options(connection_, MYSQL_OPT_WRITE_TIMEOUT, "10");
    
    // 强制使用TCP协议
    enum mysql_protocol_type protocol = MYSQL_PROTOCOL_TCP;
    mysql_options(connection_, MYSQL_OPT_PROTOCOL, (void*)&protocol);

    // 打印调试信息
    LOG_INFO("--- MySQL Client Debug Info ---");
    LOG_INFO("  Client Version: {}", mysql_get_client_info());
    LOG_INFO("  Connecting with:");
    LOG_INFO("    Host: {}", DB_HOST.c_str());
    LOG_INFO("    User: {}", DB_USER.c_str());
    LOG_INFO("    DB:   {}", DB_NAME.c_str());
    LOG_INFO("    Port: {}", DB_PORT);
    LOG_INFO("    Protocol: TCP");
    LOG_INFO("-----------------------------------");
    
    // 连接到数据库
    LOG_INFO("Attempting to connect to database...");
    
    MYSQL* result = mysql_real_connect(connection_, DB_HOST.c_str(), DB_USER.c_str(), 
                           DB_PASSWORD.c_str(), DB_NAME.c_str(), DB_PORT, nullptr, 0);
    
    if (!result) {
        LOG_ERROR("mysql_real_connect() failed: {}", mysql_error(connection_));
        mysql_close(connection_);
        connection_ = nullptr;
        return false;
    }
    
    connected_ = true;
    LOG_INFO("Connected to database successfully");
    return true;
}

/**
 * @brief 私有的、无锁的连接状态检查实现
 * 假设调用者已经持有了 mutex_
 */
bool DatabaseManager::isConnected_impl() const {
    return connected_ && mysql_ping(connection_) == 0;
}


// ==========================================================
// 公有的、加锁的包装函数
// ==========================================================

bool DatabaseManager::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    return connect_impl();
}

bool DatabaseManager::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isConnected_impl();
}

// ==========================================================
// 依赖其他调用的公有函数 (已修复)
// ==========================================================

bool DatabaseManager::createUser(const std::string& username, const std::string& password, const std::string& email, int& userId) {
    std::lock_guard<std::mutex> lock(mutex_); // <-- 在此加锁
    
    // vvvv 修复点 vvvv
    // 调用无锁的 _impl 版本
    if (!isConnected_impl() && !connect_impl()) {
    // ^^^^ 修复点 ^^^^
        return false;
    }
    
    // 检查用户是否已存在 (调用无锁版本)
    if (userExistsNoLock(username)) {
        LOG_ERROR("User already exists: {}", username);
        return false;
    }
    
    // 对密码进行哈希处理
    std::string passwordHash = sha256(password);
    
    // 准备SQL语句，包含邮箱字段
    std::string query = "INSERT INTO users (username, password, email) VALUES ('" + 
                        username + "', '" + passwordHash + "', '" + email + "')";
    
    // 执行查询
    if (mysql_query(connection_, query.c_str())) {
        LOG_ERROR("Failed to execute query: {}", mysql_error(connection_));
        return false;
    }
    
    // 获取插入的用户ID
    userId = (int)mysql_insert_id(connection_);
    LOG_INFO("User created successfully with ID: {}", userId);
    return true;
}

bool DatabaseManager::getUserByUsername(const std::string& username, int& userId, std::string& passwordHash) {
    std::lock_guard<std::mutex> lock(mutex_); // <-- 在此加锁
    
    // 调用无锁的 _impl 版本
    if (!isConnected_impl() && !connect_impl()) {
        return false;
    }
    
    // 使用预处理语句防止SQL注入
    const char* query = "SELECT id, password FROM users WHERE username = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(connection_);
    if (!stmt) {
        LOG_ERROR("mysql_stmt_init() failed: {}", mysql_error(connection_));
        return false;
    }
    
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        LOG_ERROR("mysql_stmt_prepare() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    // 绑定参数
    MYSQL_BIND param_bind[1];
    memset(param_bind, 0, sizeof(param_bind));
    
    param_bind[0].buffer_type = MYSQL_TYPE_STRING;
    param_bind[0].buffer = (char*)username.c_str();
    param_bind[0].buffer_length = username.length();
    
    if (mysql_stmt_bind_param(stmt, param_bind)) {
        LOG_ERROR("mysql_stmt_bind_param() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    // 绑定结果
    MYSQL_BIND result_bind[2];
    memset(result_bind, 0, sizeof(result_bind));
    
    int id;
    char hash[256];
    unsigned long hash_length;
    
    result_bind[0].buffer_type = MYSQL_TYPE_LONG;
    result_bind[0].buffer = (char*)&id;
    // ... (result_bind[0] 剩余设置)
    
    result_bind[1].buffer_type = MYSQL_TYPE_STRING;
    result_bind[1].buffer = hash;
    result_bind[1].buffer_length = sizeof(hash);
    result_bind[1].length = &hash_length;
    // ... (result_bind[1] 剩余设置)
    
    if (mysql_stmt_bind_result(stmt, result_bind)) {
        LOG_ERROR("mysql_stmt_bind_result() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    // 执行查询
    if (mysql_stmt_execute(stmt)) {
        LOG_ERROR("mysql_stmt_execute() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    // 获取结果
    if (mysql_stmt_fetch(stmt) == MYSQL_NO_DATA) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    // 获取用户信息
    userId = id;
    passwordHash = std::string(hash, hash_length);
    
    mysql_stmt_close(stmt);
    return true;
}

bool DatabaseManager::userExists(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 这个调用是正确的，因为 userExistsNoLock 会在同一个锁的保护下运行
    return userExistsNoLock(username); 
}

// 私有的无锁版本，供内部调用避免递归锁死
bool DatabaseManager::userExistsNoLock(const std::string& username) {
    // 注意：此函数假设调用者已持有mutex_锁
    
    // 必须调用无锁的 _impl 版本
    if (!isConnected_impl() && !connect_impl()) {
        return false;
    }
    
    // 使用预处理语句防止SQL注入
    const char* query = "SELECT id FROM users WHERE username = ? LIMIT 1";
    MYSQL_STMT* stmt = mysql_stmt_init(connection_);
    if (!stmt) {
        LOG_ERROR("mysql_stmt_init() failed: {}", mysql_error(connection_));
        return false;
    }
    
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        LOG_ERROR("mysql_stmt_prepare() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    // 绑定参数
    MYSQL_BIND param_bind[1];
    memset(param_bind, 0, sizeof(param_bind));
    
    param_bind[0].buffer_type = MYSQL_TYPE_STRING;
    param_bind[0].buffer = (char*)username.c_str();
    param_bind[0].buffer_length = username.length();
    
    if (mysql_stmt_bind_param(stmt, param_bind)) {
        LOG_ERROR("mysql_stmt_bind_param() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    // 执行查询
    if (mysql_stmt_execute(stmt)) {
        LOG_ERROR("mysql_stmt_execute() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    // 检查是否有结果
    bool exists = mysql_stmt_fetch(stmt) == 0;
    
    mysql_stmt_close(stmt);
    return exists;
}