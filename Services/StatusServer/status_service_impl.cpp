#include "status_service_impl.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <mysql/mysql.h>
#include "../utils/database_manager.h"
#include "../utils/redis_manager.h"
#include "../utils/logger.h"

StatusServiceImpl::StatusServiceImpl() : db_(DatabaseManager::getInstance()), redis_(RedisManager::getInstance()) {
    // 构造函数现在只负责初始化引用
    // 实际连接将在第一次调用时按需建立
    
    // 注册数据库实例到DatabaseManager的负载均衡器中
    // 使用Docker中运行的MySQL服务地址和端口
    db_.addDatabaseInstance("localhost", 3307, "im_user", "password", "im_database", 2);  // 权重为2
    
    // 初始化Redis连接
    redis_.initialize("localhost", 6379, 10);  // 10个连接的连接池
    
    LOG_INFO("StatusServiceImpl initialized with integrated load balancing and Redis support");
}

Status StatusServiceImpl::UpdateUserStatus(ServerContext* context, 
                                          const UserStatusRequest* request, 
                                          UserStatusResponse* response) {
    
    // 现在DatabaseManager内部会自动处理负载均衡
    // 我们只需要调用数据库操作方法即可
    
    LOG_DEBUG("Updating user status for user ID: {}", request->user_id());
    
    // 更新用户状态到数据库和Redis缓存
    bool dbSuccess = updateUserStatusInDB(request->user_id(), request->status(), request->session_token());
    bool cacheSuccess = updateUserStatusInCache(request->user_id(), request->status(), request->session_token());
    
    if (dbSuccess) {
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

bool StatusServiceImpl::updateUserStatusInCache(int32_t user_id, status::UserStatus status, const std::string& session_token) {
    try {
        // 使用哈希存储用户状态信息
        std::string key = "user:status:" + std::to_string(user_id);
        
        // 转换状态为字符串
        std::string status_str;
        switch (status) {
            case status::UserStatus::OFFLINE: status_str = "OFFLINE"; break;
            case status::UserStatus::ONLINE: status_str = "ONLINE"; break;
            case status::UserStatus::AWAY: status_str = "AWAY"; break;
            case status::UserStatus::BUSY: status_str = "BUSY"; break;
            default: status_str = "OFFLINE"; break;
        }
        
        // 存储到Redis哈希中
        bool result = true;
        result &= redis_.hset(key, "status", status_str);
        result &= redis_.hset(key, "session_token", session_token);
        result &= redis_.hset(key, "last_updated", std::to_string(std::time(nullptr)));
        
        // 设置过期时间（例如5分钟）
        // 注意：这里需要直接使用hiredis API来设置过期时间
        // 或者我们可以添加一个expire方法到RedisManager
        
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to update user status in cache: {}", e.what());
        return false;
    }
}

bool StatusServiceImpl::getUserStatusFromCache(int32_t user_id, status::UserStatus& status, std::string& session_token) {
    try {
        std::string key = "user:status:" + std::to_string(user_id);
        std::unordered_map<std::string, std::string> result;
        
        if (!redis_.hgetall(key, result)) {
            return false;
        }
        
        // 解析状态
        auto status_it = result.find("status");
        if (status_it != result.end()) {
            if (status_it->second == "ONLINE") {
                status = status::UserStatus::ONLINE;
            } else if (status_it->second == "AWAY") {
                status = status::UserStatus::AWAY;
            } else if (status_it->second == "BUSY") {
                status = status::UserStatus::BUSY;
            } else {
                status = status::UserStatus::OFFLINE;
            }
        } else {
            status = status::UserStatus::OFFLINE;
        }
        
        // 获取会话令牌
        auto token_it = result.find("session_token");
        if (token_it != result.end()) {
            session_token = token_it->second;
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get user status from cache: {}", e.what());
        return false;
    }
}

bool StatusServiceImpl::cacheFriendsList(int32_t user_id, const std::vector<int32_t>& friend_ids) {
    try {
        std::string key = "user:friends:" + std::to_string(user_id);
        
        // 先清空旧的好友列表
        redis_.del(key);
        
        // 将好友ID添加到有序集合中
        for (size_t i = 0; i < friend_ids.size(); ++i) {
            redis_.zadd(key, static_cast<double>(i), std::to_string(friend_ids[i]));
        }
        
        // 设置过期时间（例如30分钟）
        // 同样需要直接使用hiredis API或者添加expire方法
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to cache friends list: {}", e.what());
        return false;
    }
}

bool StatusServiceImpl::getCachedFriendsList(int32_t user_id, std::vector<int32_t>& friend_ids) {
    try {
        std::string key = "user:friends:" + std::to_string(user_id);
        std::vector<std::string> result;
        
        // 获取有序集合中的所有元素
        if (!redis_.zrange(key, 0, -1, result)) {
            return false;
        }
        
        // 转换为整数ID
        friend_ids.clear();
        for (const auto& id_str : result) {
            try {
                friend_ids.push_back(std::stoi(id_str));
            } catch (const std::exception&) {
                // 忽略无效的ID
                continue;
            }
        }
        
        return !friend_ids.empty();
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get cached friends list: {}", e.what());
        return false;
    }
}

Status StatusServiceImpl::GetUserStatus(ServerContext* context, 
                                       const GetUserStatusRequest* request, 
                                       GetUserStatusResponse* response) {
    
    LOG_DEBUG("Getting user status for user ID: {}", request->user_id());
    
    // 首先尝试从缓存获取
    status::UserStatus status;
    std::string session_token;
    if (getUserStatusFromCache(request->user_id(), status, session_token)) {
        response->set_success(true);
        response->set_message("User status retrieved from cache");
        response->set_status(status);
        response->set_last_seen(std::time(nullptr) * 1000); // 简化处理
        LOG_DEBUG("Retrieved user status from cache for user ID: {}", request->user_id());
        return Status::OK;
    }
    
    // 缓存未命中，从数据库获取
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
    
    LOG_DEBUG("Getting friends status for user ID: {}", request->user_id());
    
    // 首先尝试从缓存获取好友列表
    std::vector<int32_t> friend_ids;
    if (!getCachedFriendsList(request->user_id(), friend_ids)) {
        // 缓存未命中，从数据库获取
        friend_ids = getFriendsIds(request->user_id());
        // 将好友列表缓存起来
        cacheFriendsList(request->user_id(), friend_ids);
    }
    
    response->set_success(true);
    response->set_message("Friends status retrieved successfully");
    
    for (int32_t friend_id : friend_ids) {
        status::UserStatus status;
        std::string session_token;
        
        // 尝试从缓存获取好友状态
        if (!getUserStatusFromCache(friend_id, status, session_token)) {
            // 缓存未命中，从数据库获取
            std::chrono::time_point<std::chrono::system_clock> last_seen;
            if (getUserStatusFromDB(friend_id, status, last_seen)) {
                // 更新缓存
                updateUserStatusInCache(friend_id, status, session_token);
            } else {
                continue; // 跳过无效用户
            }
        }
        
        auto* friend_status = response->add_friends();
        friend_status->set_user_id(friend_id);
        friend_status->set_username("user_" + std::to_string(friend_id));
        friend_status->set_status(status);
        friend_status->set_last_seen(std::time(nullptr) * 1000); // 简化处理
    }
    
    return Status::OK;
}

Status StatusServiceImpl::AddFriend(ServerContext* context, 
                                   const AddFriendRequest* request, 
                                   AddFriendResponse* response) {
    
    LOG_DEBUG("Adding friend relationship between user {} and user {}", 
              request->user_id(), request->friend_id());
    
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
    
    LOG_DEBUG("Getting friends list for user ID: {}", request->user_id());
    
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
        // DatabaseManager内部会自动处理负载均衡和故障转移
        LOG_ERROR("Database query failed: {}", mysql_error(connection));
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
    
    // 解析状态
    if (row[0]) {
        std::string status_str(row[0]);
        if (status_str == "ONLINE") {
            status = status::UserStatus::ONLINE;
        } else if (status_str == "AWAY") {
            status = status::UserStatus::AWAY;
        } else if (status_str == "BUSY") {
            status = status::UserStatus::BUSY;
        } else {
            status = status::UserStatus::OFFLINE;
        }
    } else {
        status = status::UserStatus::OFFLINE;
    }
    
    // 解析最后在线时间
    if (row[1]) {
        // 这里简化处理，实际应用中需要正确解析时间戳
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