#include "database_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <cstdlib>  // 添加这行用于 getenv

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
    std::cout << "DatabaseManager destructor called" << std::endl;
    disconnect();
    mysql_library_end();
    std::cout << "DatabaseManager destructor finished" << std::endl;
}

void DatabaseManager::disconnect() {
    if (connection_) {
        std::cout << "Disconnecting from database" << std::endl;
        mysql_close(connection_);
        connection_ = nullptr;
        connected_ = false;
        std::cout << "Disconnected from database" << std::endl;
    }
}

bool DatabaseManager::connect() {
    if (connected_) {
        return true;
    }
    
    connection_ = mysql_init(nullptr);
    if (!connection_) {
        std::cerr << "mysql_init() failed" << std::endl;
        return false;
    }
    
    // 设置连接选项，确保使用TCP连接而不是socket
    mysql_options(connection_, MYSQL_OPT_CONNECT_TIMEOUT, "10");
    mysql_options(connection_, MYSQL_OPT_READ_TIMEOUT, "10");
    mysql_options(connection_, MYSQL_OPT_WRITE_TIMEOUT, "10");
    
    // 强制使用TCP协议而不是Unix套接字
    enum mysql_protocol_type protocol = MYSQL_PROTOCOL_TCP;
    mysql_options(connection_, MYSQL_OPT_PROTOCOL, (void*)&protocol);

    std::cout << "--- MySQL Client Debug Info ---" << std::endl;
    std::cout << "  Client Version: " << mysql_get_client_info() << std::endl;
    std::cout << "  Connecting with:" << std::endl;
    std::cout << "    Host: " << DB_HOST.c_str() << std::endl;
    std::cout << "    User: " << DB_USER.c_str() << std::endl;
    std::cout << "    DB:   " << DB_NAME.c_str() << std::endl;
    std::cout << "    Port: " << DB_PORT << std::endl;
    std::cout << "    Protocol: TCP" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    
    // 连接到数据库
    std::cout << "Attempting to connect to database..." << std::endl;
    
    // ================== 增加的调试代码 ==================
    std::cout << "DEBUG: Attempting connection with PWD=[" << DB_PASSWORD << "]" << std::endl;
    // ================== 调试代码结束 ==================

    // 连接到数据库
    std::cout << "Attempting to connect to database..." << std::endl;

    MYSQL* result = mysql_real_connect(connection_, DB_HOST.c_str(), DB_USER.c_str(), 
                           DB_PASSWORD.c_str(), DB_NAME.c_str(), DB_PORT, nullptr, 0);
    
    if (!result) {
        std::cerr << "mysql_real_connect() failed: " << mysql_error(connection_) << std::endl;
        mysql_close(connection_);
        connection_ = nullptr;
        return false;
    }
    
    connected_ = true;
    std::cout << "Connected to database successfully" << std::endl;
    return true;
}

bool DatabaseManager::isConnected() const {
    return connected_ && mysql_ping(connection_) == 0;
}

bool DatabaseManager::createUser(const std::string& username, const std::string& password, int& userId) {
    if (!isConnected() && !connect()) {
        return false;
    }
    
    // 检查用户是否已存在
    if (userExists(username)) {
        std::cerr << "User already exists: " << username << std::endl;
        return false;
    }
    
    // 对密码进行哈希处理
    std::string passwordHash = sha256(password);
    
    // 准备SQL语句
    std::string query = "INSERT INTO users (username, password) VALUES ('" + 
                        username + "', '" + passwordHash + "')";
    
    // 执行查询
    if (mysql_query(connection_, query.c_str())) {
        std::cerr << "Failed to execute query: " << mysql_error(connection_) << std::endl;
        return false;
    }
    
    // 获取插入的用户ID
    userId = (int)mysql_insert_id(connection_);
    std::cout << "User created successfully with ID: " << userId << std::endl;
    return true;
}

bool DatabaseManager::getUserByUsername(const std::string& username, int& userId, std::string& passwordHash) {
    if (!isConnected() && !connect()) {
        return false;
    }
    
    // 准备SQL语句
    std::string query = "SELECT id, password FROM users WHERE username = '" + username + "'";
    
    // 执行查询
    if (mysql_query(connection_, query.c_str())) {
        std::cerr << "Failed to execute query: " << mysql_error(connection_) << std::endl;
        return false;
    }
    
    // 获取结果
    MYSQL_RES* result = mysql_store_result(connection_);
    if (!result) {
        std::cerr << "Failed to store result: " << mysql_error(connection_) << std::endl;
        return false;
    }
    
    // 检查是否有结果
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    
    // 获取用户信息
    userId = atoi(row[0]);
    passwordHash = row[1] ? row[1] : "";
    
    mysql_free_result(result);
    return true;
}

bool DatabaseManager::userExists(const std::string& username) {
    if (!isConnected() && !connect()) {
        return false;
    }
    
    // 准备SQL语句
    std::string query = "SELECT id FROM users WHERE username = '" + username + "' LIMIT 1";
    
    // 执行查询
    if (mysql_query(connection_, query.c_str())) {
        std::cerr << "Failed to execute query: " << mysql_error(connection_) << std::endl;
        return false;
    }
    
    // 获取结果
    MYSQL_RES* result = mysql_store_result(connection_);
    if (!result) {
        std::cerr << "Failed to store result: " << mysql_error(connection_) << std::endl;
        return false;
    }
    
    // 检查是否有结果
    bool exists = mysql_num_rows(result) > 0;
    mysql_free_result(result);
    
    return exists;
}