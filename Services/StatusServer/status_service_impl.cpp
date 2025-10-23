#include "status_service_impl.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <mysql/mysql.h>
#include "../utils/database_manager.h"

StatusServiceImpl::StatusServiceImpl() : db_(DatabaseManager::getInstance()) {
    // 初始化数据库连接
    if (!db_.isConnected() && !db_.connect()) {
        std::cerr << "Warning: Failed to connect to database in StatusService" << std::endl;
    }
}

Status StatusServiceImpl::UpdateUserStatus(ServerContext* context, 
                                          const UserStatusRequest* request, 
                                          UserStatusResponse* response) {
    // 验证会话令牌
    if (!validateSessionToken(request->user_id(), request->session_token())) {
        response->set_success(false);
        response->set_message("Invalid session token");
        return Status::OK;
    }
    
    // 更新用户状态到数据库
    if (updateUserStatusInDB(request->user_id(), request->status(), request->session_token())) {
        response->set_success(true);
        response->set_message("User status updated successfully");
        
        std::cout << "Updated status for user " << request->user_id() 
                  << " to " << request->status() << std::endl;
    } else {
        response->set_success(false);
        response->set_message("Failed to update user status in database");
        std::cerr << "Failed to update status for user " << request->user_id() << std::endl;
    }
    
    return Status::OK;
}

Status StatusServiceImpl::GetUserStatus(ServerContext* context, 
                                       const GetUserStatusRequest* request, 
                                       GetUserStatusResponse* response) {
    status::UserStatus status;
    std::chrono::time_point<std::chrono::system_clock> last_seen;
    
    if (getUserStatusFromDB(request->user_id(), status, last_seen)) {
        response->set_success(true);
        response->set_message("User status retrieved successfully");
        response->set_status(status);
        
        // 转换时间戳
        auto duration = last_seen.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        response->set_last_seen(millis);
    } else {
        response->set_success(false);
        response->set_message("User not found");
    }
    
    return Status::OK;
}

Status StatusServiceImpl::GetFriendsStatus(ServerContext* context, 
                                          const GetFriendsStatusRequest* request, 
                                          GetFriendsStatusResponse* response) {
    // 获取用户的好友ID列表
    auto friend_ids = getFriendsIds(request->user_id());
    
    response->set_success(true);
    response->set_message("Friends status retrieved successfully");
    
    // 为每个好友填充状态信息
    for (int32_t friend_id : friend_ids) {
        status::UserStatus status;
        std::chrono::time_point<std::chrono::system_clock> last_seen;
        
        if (getUserStatusFromDB(friend_id, status, last_seen)) {
            auto* friend_status = response->add_friends();
            friend_status->set_user_id(friend_id);
            friend_status->set_username("user_" + std::to_string(friend_id)); // 简化实现
            friend_status->set_status(status);
            
            // 转换时间戳
            auto duration = last_seen.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            friend_status->set_last_seen(millis);
        }
    }
    
    return Status::OK;
}

Status StatusServiceImpl::AddFriend(ServerContext* context, 
                                   const AddFriendRequest* request, 
                                   AddFriendResponse* response) {
    // 检查好友关系是否已存在
    if (friendExistsInDB(request->user_id(), request->friend_id())) {
        response->set_success(false);
        response->set_message("Friend relationship already exists");
        return Status::OK;
    }
    
    // 添加好友关系到数据库（双向）
    if (addFriendToDB(request->user_id(), request->friend_id()) && 
        addFriendToDB(request->friend_id(), request->user_id())) {
        response->set_success(true);
        response->set_message("Friend added successfully");
    } else {
        response->set_success(false);
        response->set_message("Failed to add friend relationship to database");
    }
    
    return Status::OK;
}

Status StatusServiceImpl::GetFriendsList(ServerContext* context, 
                                        const GetFriendsListRequest* request, 
                                        GetFriendsListResponse* response) {
    // 获取用户的好友ID列表
    auto friend_ids = getFriendsIds(request->user_id());
    
    response->set_success(true);
    response->set_message("Friends list retrieved successfully");
    
    // 为每个好友填充基本信息
    for (int32_t friend_id : friend_ids) {
        auto* friend_info = response->add_friends();
        friend_info->set_user_id(friend_id);
        friend_info->set_username("user_" + std::to_string(friend_id)); // 简化实现
    }
    
    return Status::OK;
}

bool StatusServiceImpl::validateSessionToken(int32_t user_id, const std::string& token) {
    // 查询数据库验证会话令牌
    std::lock_guard<std::mutex> lock(db_.mutex());
    if (!db_.isConnected() && !db_.connect()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) {
        return false;
    }
    
    std::string query = "SELECT session_token FROM user_sessions WHERE user_id = " + std::to_string(user_id);
    if (mysql_query(connection, query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    bool valid = row && row[0] && token == std::string(row[0]);
    
    mysql_free_result(result);
    return valid;
}

std::vector<int32_t> StatusServiceImpl::getFriendsIds(int32_t user_id) {
    std::vector<int32_t> friend_ids;
    
    std::lock_guard<std::mutex> lock(db_.mutex());
    if (!db_.isConnected() && !db_.connect()) {
        return friend_ids;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) {
        return friend_ids;
    }
    
    std::string query = "SELECT friend_id FROM user_friends WHERE user_id = " + std::to_string(user_id);
    if (mysql_query(connection, query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        return friend_ids;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        return friend_ids;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        if (row[0]) {
            friend_ids.push_back(std::stoi(row[0]));
        }
    }
    
    mysql_free_result(result);
    return friend_ids;
}

bool StatusServiceImpl::updateUserStatusInDB(int32_t user_id, status::UserStatus status, const std::string& session_token) {
    std::lock_guard<std::mutex> lock(db_.mutex());
    if (!db_.isConnected() && !db_.connect()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) {
        return false;
    }
    
    // 将枚举值转换为字符串
    std::string status_str;
    switch (status) {
        case status::UserStatus::OFFLINE: status_str = "OFFLINE"; break;
        case status::UserStatus::ONLINE: status_str = "ONLINE"; break;
        case status::UserStatus::AWAY: status_str = "AWAY"; break;
        case status::UserStatus::BUSY: status_str = "BUSY"; break;
        default: status_str = "OFFLINE"; break;
    }
    
    // 使用INSERT ... ON DUPLICATE KEY UPDATE来插入或更新用户状态
    std::string query = "INSERT INTO user_status (user_id, status, last_seen, session_token) VALUES (" + 
                        std::to_string(user_id) + ", '" + status_str + "', NOW(), '" + session_token + 
                        "') ON DUPLICATE KEY UPDATE status = '" + status_str + "', last_seen = NOW(), session_token = '" + session_token + "'";
    
    if (mysql_query(connection, query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        return false;
    }
    
    return true;
}

bool StatusServiceImpl::getUserStatusFromDB(int32_t user_id, status::UserStatus& status, std::chrono::time_point<std::chrono::system_clock>& last_seen) {
    std::lock_guard<std::mutex> lock(db_.mutex());
    if (!db_.isConnected() && !db_.connect()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) {
        return false;
    }
    
    std::string query = "SELECT status, last_seen FROM user_status WHERE user_id = " + std::to_string(user_id);
    if (mysql_query(connection, query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    
    // 解析状态
    if (row[0]) {
        std::string status_str(row[0]);
        if (status_str == "OFFLINE") status = status::UserStatus::OFFLINE;
        else if (status_str == "ONLINE") status = status::UserStatus::ONLINE;
        else if (status_str == "AWAY") status = status::UserStatus::AWAY;
        else if (status_str == "BUSY") status = status::UserStatus::BUSY;
        else status = status::UserStatus::OFFLINE;
    } else {
        status = status::UserStatus::OFFLINE;
    }
    
    // 解析时间戳
    if (row[1]) {
        // 将MySQL时间字符串转换为时间点
        std::string time_str(row[1]);
        // 简化处理，实际应用中应该使用更精确的解析方法
        last_seen = std::chrono::system_clock::now();
    } else {
        last_seen = std::chrono::system_clock::now();
    }
    
    mysql_free_result(result);
    return true;
}

bool StatusServiceImpl::addFriendToDB(int32_t user_id, int32_t friend_id) {
    std::lock_guard<std::mutex> lock(db_.mutex());
    if (!db_.isConnected() && !db_.connect()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) {
        return false;
    }
    
    std::string query = "INSERT IGNORE INTO user_friends (user_id, friend_id) VALUES (" + 
                        std::to_string(user_id) + ", " + std::to_string(friend_id) + ")";
    
    if (mysql_query(connection, query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        return false;
    }
    
    return true;
}

bool StatusServiceImpl::friendExistsInDB(int32_t user_id, int32_t friend_id) {
    std::lock_guard<std::mutex> lock(db_.mutex());
    if (!db_.isConnected() && !db_.connect()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) {
        return false;
    }
    
    std::string query = "SELECT COUNT(*) FROM user_friends WHERE user_id = " + 
                        std::to_string(user_id) + " AND friend_id = " + std::to_string(friend_id);
    
    if (mysql_query(connection, query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    bool exists = row && row[0] && std::stoi(row[0]) > 0;
    
    mysql_free_result(result);
    return exists;
}