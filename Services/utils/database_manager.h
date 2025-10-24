#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <mysql/mysql.h>
#include <string>
#include <memory>
#include <mutex>
#include "crypto_utils.h"

class DatabaseManager {
public:
    static DatabaseManager& getInstance();
    
    bool connect();
    void disconnect();
    bool isConnected() const;

    bool connect_impl();
    bool isConnected_impl() const;
    
    // 获取原始数据库连接（用于直接执行查询）
    void* getConnection() const { return static_cast<void*>(connection_); }
    
    // 用户相关操作
    bool createUser(const std::string& username, const std::string& password, const std::string& email, int& userId); 
    
    bool getUserByUsername(const std::string& username, int& userId, std::string& passwordHash);
    bool userExists(const std::string& username);
    
    // 获取数据库连接信息
    std::string getHost() const { return DB_HOST; }
    std::string getUser() const { return DB_USER; }
    std::string getName() const { return DB_NAME; }
    int getPort() const { return DB_PORT; }
    
    // 获取互斥锁（用于线程安全）
    std::mutex& mutex() const { return mutex_; }

private:
    DatabaseManager();
    ~DatabaseManager();
    
    bool userExistsNoLock(const std::string& username);
    
    MYSQL* connection_;
    bool connected_;
    mutable std::mutex mutex_;  // 用于线程安全
    
    // 数据库连接信息
    static const std::string DB_HOST;
    static const std::string DB_USER;
    static const std::string DB_PASSWORD;
    static const std::string DB_NAME;
    static const int DB_PORT;
};

#endif // DATABASE_MANAGER_H