#include "status_service_impl.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <mysql/mysql.h>
#include "../utils/database_manager.h"
#include "../utils/logger.h"

StatusServiceImpl::StatusServiceImpl() : db_(DatabaseManager::getInstance()) {
    // 构造函数现在只负责初始化引用
    // 实际连接将在第一次调用时按需建立
}

Status StatusServiceImpl::UpdateUserStatus(ServerContext* context, 
                                          const UserStatusRequest* request, 
                                          UserStatusResponse* response) {
    
    // 更新用户状态到数据库
    if (updateUserStatusInDB(request->user_id(), request->status(), request->session_token())) {
        response->set_success(true);
        response->set_message("User status updated successfully");
        
        LOG_INFO("[StatusServer] Updated status for user {} to {}", request->user_id(), request->status());
    } else {
        response->set_success(false);
        response->set_message("Failed to update user status in database");
        LOG_ERROR("[StatusServer] Failed to update status for user {}", request->user_id());
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
    auto friend_ids = getFriendsIds(request->user_id());
    
    response->set_success(true);
    response->set_message("Friends status retrieved successfully");
    
    for (int32_t friend_id : friend_ids) {
        status::UserStatus status;
        std::chrono::time_point<std::chrono::system_clock> last_seen;
        
        if (getUserStatusFromDB(friend_id, status, last_seen)) {
            auto* friend_status = response->add_friends();
            friend_status->set_user_id(friend_id);
            friend_status->set_username("user_" + std::to_string(friend_id));
            friend_status->set_status(status);
            
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
    if (friendExistsInDB(request->user_id(), request->friend_id())) {
        response->set_success(false);
        response->set_message("Friend relationship already exists");
        return Status::OK;
    }
    
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
    auto friend_ids = getFriendsIds(request->user_id());
    
    response->set_success(true);
    response->set_message("Friends list retrieved successfully");
    
    for (int32_t friend_id : friend_ids) {
        auto* friend_info = response->add_friends();
        friend_info->set_user_id(friend_id);
        friend_info->set_username("user_" + std::to_string(friend_id));
    }
    
    return Status::OK;
}

bool StatusServiceImpl::validateSessionToken(int32_t user_id, const std::string& token) {
    std::lock_guard<std::mutex> lock(db_.mutex());
    if (!db_.isConnected_impl() && !db_.connect_impl()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) return false;
    
    std::string query = "SELECT session_token FROM user_status WHERE user_id = " + std::to_string(user_id);
    if (mysql_query(connection, query.c_str())) {
        LOG_ERROR("MySQL query error: {}", mysql_error(connection));
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    bool valid = row && row[0] && token == std::string(row[0]);
    
    mysql_free_result(result);
    return valid;
}

std::vector<int32_t> StatusServiceImpl::getFriendsIds(int32_t user_id) {
    std::vector<int32_t> friend_ids;
    std::lock_guard<std::mutex> lock(db_.mutex());
    
    if (!db_.isConnected_impl() && !db_.connect_impl()) {
        return friend_ids;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) return friend_ids;
    
    std::string query = "SELECT friend_id FROM user_friends WHERE user_id = " + std::to_string(user_id);
    if (mysql_query(connection, query.c_str())) {
        LOG_ERROR("MySQL query error: {}", mysql_error(connection));
        return friend_ids;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) return friend_ids;
    
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
    
    if (!db_.isConnected_impl() && !db_.connect_impl()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) return false;
    
    std::string status_str;
    switch (status) {
        case status::UserStatus::OFFLINE: status_str = "OFFLINE"; break;
        case status::UserStatus::ONLINE: status_str = "ONLINE"; break;
        case status::UserStatus::AWAY: status_str = "AWAY"; break;
        case status::UserStatus::BUSY: status_str = "BUSY"; break;
        default: status_str = "OFFLINE"; break;
    }
    
    std::string query = "INSERT INTO user_status (user_id, status, last_seen, session_token) VALUES (" + 
                        std::to_string(user_id) + ", '" + status_str + "', NOW(), '" + session_token + 
                        "') ON DUPLICATE KEY UPDATE status = '" + status_str + "', last_seen = NOW(), session_token = '" + session_token + "'";
    
    if (mysql_query(connection, query.c_str())) {
        LOG_ERROR("MySQL query error: {}", mysql_error(connection));
        return false;
    }
    
    return true;
}

bool StatusServiceImpl::getUserStatusFromDB(int32_t user_id, status::UserStatus& status, std::chrono::time_point<std::chrono::system_clock>& last_seen) {
    std::lock_guard<std::mutex> lock(db_.mutex());
    
    if (!db_.isConnected_impl() && !db_.connect_impl()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) return false;
    
    std::string query = "SELECT status, last_seen FROM user_status WHERE user_id = " + std::to_string(user_id);
    if (mysql_query(connection, query.c_str())) {
        LOG_ERROR("MySQL query error: {}", mysql_error(connection));
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    
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
    
    if (row[1]) {
        last_seen = std::chrono::system_clock::now();
    } else {
        last_seen = std::chrono::system_clock::now();
    }
    
    mysql_free_result(result);
    return true;
}

bool StatusServiceImpl::addFriendToDB(int32_t user_id, int32_t friend_id) {
    std::lock_guard<std::mutex> lock(db_.mutex());
    
    if (!db_.isConnected_impl() && !db_.connect_impl()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) return false;
    
    std::string query = "INSERT IGNORE INTO user_friends (user_id, friend_id) VALUES (" + 
                        std::to_string(user_id) + ", " + std::to_string(friend_id) + ")";
    
    if (mysql_query(connection, query.c_str())) {
        LOG_ERROR("MySQL query error: {}", mysql_error(connection));
        return false;
    }
    
    return true;
}

bool StatusServiceImpl::friendExistsInDB(int32_t user_id, int32_t friend_id) {
    std::lock_guard<std::mutex> lock(db_.mutex());
    
    if (!db_.isConnected_impl() && !db_.connect_impl()) {
        return false;
    }
    
    MYSQL* connection = static_cast<MYSQL*>(db_.getConnection());
    if (!connection) return false;
    
    std::string query = "SELECT COUNT(*) FROM user_friends WHERE user_id = " + 
                        std::to_string(user_id) + " AND friend_id = " + std::to_string(friend_id);
    
    if (mysql_query(connection, query.c_str())) {
        LOG_ERROR("MySQL query error: {}", mysql_error(connection));
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    bool exists = row && row[0] && std::stoi(row[0]) > 0;
    
    mysql_free_result(result);
    return exists;
}