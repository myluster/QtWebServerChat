#include "database_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <cstdlib>
#include <cstring>
#include "logger.h"
#include "load_balancer.h"

// 服务名称常量，用于在负载均衡器中标识数据库服务
const std::string DatabaseManager::SERVICE_NAME = "DatabaseService";

/**
 * @brief 获取DatabaseManager单例实例
 * 使用局部静态变量实现线程安全的单例模式
 * @return DatabaseManager实例的引用
 */
DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

/**
 * @brief DatabaseManager构造函数
 * 初始化MySQL库并获取负载均衡器实例引用
 */
DatabaseManager::DatabaseManager() : connection_(nullptr), connected_(false),
                                   loadBalancer_(LoadBalancer::getInstance()) {
    // 初始化MySQL客户端库
    mysql_library_init(0, nullptr, nullptr);
    
    // 可以在这里添加默认的数据库实例
    // addDatabaseInstance("127.0.0.1", 3307, "im_user", "password", "im_database", 1);
}

/**
 * @brief DatabaseManager析构函数
 * 清理资源，断开数据库连接并关闭MySQL库
 */
DatabaseManager::~DatabaseManager() {
    LOG_INFO("DatabaseManager destructor called");
    disconnect();
    mysql_library_end();
    LOG_INFO("DatabaseManager destructor finished");
}

/**
 * @brief 添加数据库实例到负载均衡器
 * @param host 数据库主机地址
 * @param port 数据库端口
 * @param user 用户名
 * @param password 密码
 * @param database 数据库名称
 * @param weight 负载均衡权重
 */
void DatabaseManager::addDatabaseInstance(const std::string& host, int port, 
                                         const std::string& user, const std::string& password, 
                                         const std::string& database, int weight) {
    // 将数据库实例添加到负载均衡器中
    loadBalancer_.addServiceInstance(SERVICE_NAME, host, port, weight);
    LOG_INFO("Added database instance: {}:{} with weight {}", host, port, weight);
}

/**
 * @brief 更新数据库实例的健康状态
 * @param host 数据库主机地址
 * @param port 数据库端口
 * @param isHealthy 健康状态
 */
void DatabaseManager::updateInstanceHealth(const std::string& host, int port, bool isHealthy) {
    // 更新负载均衡器中对应实例的健康状态
    loadBalancer_.updateHealthStatus(SERVICE_NAME, host, port, isHealthy);
    LOG_INFO("Updated database instance health status: {}:{} to {}", host, port, 
             isHealthy ? "healthy" : "unhealthy");
}

/**
 * @brief 断开当前数据库连接
 * 线程安全地断开当前数据库连接并释放资源
 */
void DatabaseManager::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_) {
        LOG_INFO("Disconnecting from database {}:{}", currentHost_, currentPort_);
        mysql_close(connection_);
        connection_ = nullptr;
        connected_ = false;
        LOG_INFO("Disconnected from database {}:{}", currentHost_, currentPort_);
    }
}

// ==========================================================
// 私有的、无锁的 _impl 函数
// ==========================================================

/**
 * @brief 私有的、无锁的连接实现
 * 假设调用者已经持有了 mutex_
 * 使用负载均衡器选择一个健康的数据库实例进行连接
 */
bool DatabaseManager::connect_impl() {
    if (connected_) {
        return true;
    }
    
    // 使用负载均衡器选择一个健康的数据库实例
    auto dbInstance = loadBalancer_.getNextHealthyInstance(SERVICE_NAME);
    if (!dbInstance) {
        LOG_ERROR("No healthy database instances available");
        return false;
    }
    
    // 更新当前连接信息
    currentHost_ = dbInstance->host;
    currentPort_ = dbInstance->port;
    
    // 注意：在实际应用中，您可能需要存储每个实例的用户和密码信息
    // 这里为了简化，使用默认值
    currentUser_ = "im_user";
    currentPassword_ = "password";
    currentDatabase_ = "im_database";
    
    connection_ = mysql_init(nullptr);
    if (!connection_) {
        LOG_ERROR("mysql_init() failed");
        // 标记实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
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
    LOG_INFO("    Host: {}", currentHost_.c_str());
    LOG_INFO("    User: {}", currentUser_.c_str());
    LOG_INFO("    DB:   {}", currentDatabase_.c_str());
    LOG_INFO("    Port: {}", currentPort_);
    LOG_INFO("    Protocol: TCP");
    LOG_INFO("-----------------------------------");
    
    // 连接到数据库
    LOG_INFO("Attempting to connect to database...");
    
    MYSQL* result = mysql_real_connect(connection_, currentHost_.c_str(), currentUser_.c_str(), 
                           currentPassword_.c_str(), currentDatabase_.c_str(), currentPort_, nullptr, 0);
    
    if (!result) {
        LOG_ERROR("mysql_real_connect() failed: {}", mysql_error(connection_));
        mysql_close(connection_);
        connection_ = nullptr;
        // 标记实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
        return false;
    }
    
    connected_ = true;
    LOG_INFO("Connected to database {}:{} successfully", currentHost_, currentPort_);
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

/**
 * @brief 连接到数据库（线程安全版本）
 * 使用互斥锁保护连接过程
 * @return 连接成功返回true，否则返回false
 */
bool DatabaseManager::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    return connect_impl();
}

/**
 * @brief 检查数据库连接状态（线程安全版本）
 * 使用互斥锁保护状态检查过程
 * @return 连接有效返回true，否则返回false
 */
bool DatabaseManager::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isConnected_impl();
}

// ==========================================================
// 依赖其他调用的公有函数
// ==========================================================

/**
 * @brief 创建新用户
 * @param username 用户名
 * @param password 密码（明文，将被哈希存储）
 * @param email 邮箱地址
 * @param userId 输出参数，成功时返回新用户的ID
 * @return 成功返回true，否则返回false
 */
bool DatabaseManager::createUser(const std::string& username, const std::string& password, const std::string& email, int& userId) {
    std::lock_guard<std::mutex> lock(mutex_); // 加锁保护整个操作
    
    // 调用无锁的 _impl 版本
    if (!isConnected_impl() && !connect_impl()) {
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
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
        return false;
    }
    
    // 获取插入的用户ID
    userId = (int)mysql_insert_id(connection_);
    LOG_INFO("User created successfully with ID: {}", userId);
    return true;
}

/**
 * @brief 根据用户名获取用户信息
 * @param username 用户名
 * @param userId 输出参数，成功时返回用户ID
 * @param passwordHash 输出参数，成功时返回密码哈希值
 * @return 成功返回true，否则返回false
 */
bool DatabaseManager::getUserByUsername(const std::string& username, int& userId, std::string& passwordHash) {
    std::lock_guard<std::mutex> lock(mutex_); // 加锁保护整个操作
    
    // 调用无锁的 _impl 版本
    if (!isConnected_impl() && !connect_impl()) {
        return false;
    }
    
    // 使用预处理语句防止SQL注入
    const char* query = "SELECT id, password FROM users WHERE username = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(connection_);
    if (!stmt) {
        LOG_ERROR("mysql_stmt_init() failed: {}", mysql_error(connection_));
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
        return false;
    }
    
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        LOG_ERROR("mysql_stmt_prepare() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
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
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
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
    result_bind[0].is_null = 0;
    result_bind[0].length = 0;
    result_bind[0].error = 0;
    
    result_bind[1].buffer_type = MYSQL_TYPE_STRING;
    result_bind[1].buffer = hash;
    result_bind[1].buffer_length = sizeof(hash);
    result_bind[1].length = &hash_length;
    result_bind[1].is_null = 0;
    result_bind[1].error = 0;
    
    if (mysql_stmt_bind_result(stmt, result_bind)) {
        LOG_ERROR("mysql_stmt_bind_result() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
        return false;
    }
    
    // 执行查询
    if (mysql_stmt_execute(stmt)) {
        LOG_ERROR("mysql_stmt_execute() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
        return false;
    }
    
    // 获取结果
    int fetch_result = mysql_stmt_fetch(stmt);
    if (fetch_result == MYSQL_NO_DATA) {
        mysql_stmt_close(stmt);
        return false;
    } else if (fetch_result == MYSQL_DATA_TRUNCATED) {
        LOG_WARN("Data truncated when fetching user data");
    } else if (fetch_result != 0) {
        LOG_ERROR("mysql_stmt_fetch() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
        return false;
    }
    
    // 获取用户信息
    userId = id;
    passwordHash = std::string(hash, hash_length);
    
    mysql_stmt_close(stmt);
    return true;
}

/**
 * @brief 检查用户名是否存在
 * @param username 用户名
 * @return 存在返回true，否则返回false
 */
bool DatabaseManager::userExists(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 这个调用是正确的，因为 userExistsNoLock 会在同一个锁的保护下运行
    return userExistsNoLock(username); 
}

/**
 * @brief 私有的无锁版本，供内部调用避免递归锁死
 * @param username 用户名
 * @return 存在返回true，否则返回false
 */
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
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
        return false;
    }
    
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        LOG_ERROR("mysql_stmt_prepare() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
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
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
        return false;
    }
    
    // 执行查询
    if (mysql_stmt_execute(stmt)) {
        LOG_ERROR("mysql_stmt_execute() failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        // 标记当前实例为不健康
        updateInstanceHealth(currentHost_, currentPort_, false);
        return false;
    }
    
    // 检查是否有结果
    bool exists = mysql_stmt_fetch(stmt) == 0;
    
    mysql_stmt_close(stmt);
    return exists;
}