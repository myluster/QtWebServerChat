#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <mysql/mysql.h>
#include <string>
#include <memory>
#include "crypto_utils.h"

class DatabaseManager {
public:
    static DatabaseManager& getInstance();
    
    bool connect();
    void disconnect();
    bool isConnected() const;
    
    // 用户相关操作
    bool createUser(const std::string& username, const std::string& password, int& userId);
    bool getUserByUsername(const std::string& username, int& userId, std::string& passwordHash);
    bool userExists(const std::string& username);

private:
    DatabaseManager();
    ~DatabaseManager();
    
    MYSQL* connection_;
    bool connected_;
    
    // 数据库连接信息
    static const std::string DB_HOST;
    static const std::string DB_USER;
    static const std::string DB_PASSWORD;
    static const std::string DB_NAME;
    static const int DB_PORT;
};

#endif // DATABASE_MANAGER_H