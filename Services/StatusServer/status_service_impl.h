#ifndef STATUS_SERVICE_IMPL_H
#define STATUS_SERVICE_IMPL_H

#include <grpcpp/grpcpp.h>
#include "status.grpc.pb.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include "../utils/database_manager.h"

using grpc::ServerContext;
using grpc::Status;
using status::StatusService;
using status::UserStatusRequest;
using status::UserStatusResponse;
using status::GetUserStatusRequest;
using status::GetUserStatusResponse;
using status::GetFriendsStatusRequest;
using status::GetFriendsStatusResponse;
using status::AddFriendRequest;
using status::AddFriendResponse;
using status::GetFriendsListRequest;
using status::GetFriendsListResponse;

class StatusServiceImpl final : public StatusService::Service {
public:
    StatusServiceImpl();
    
    // 更新用户在线状态
    Status UpdateUserStatus(ServerContext* context, const UserStatusRequest* request, 
                           UserStatusResponse* response) override;
    
    // 获取用户状态
    Status GetUserStatus(ServerContext* context, const GetUserStatusRequest* request, 
                        GetUserStatusResponse* response) override;
    
    // 获取好友列表状态
    Status GetFriendsStatus(ServerContext* context, const GetFriendsStatusRequest* request, 
                           GetFriendsStatusResponse* response) override;
    
    // 添加好友
    Status AddFriend(ServerContext* context, const AddFriendRequest* request, 
                    AddFriendResponse* response) override;
    
    // 获取好友列表
    Status GetFriendsList(ServerContext* context, const GetFriendsListRequest* request, 
                         GetFriendsListResponse* response) override;

private:
    // 数据库管理器引用
    DatabaseManager& db_;
    
    // 内部辅助方法
    bool validateSessionToken(int32_t user_id, const std::string& token);
    std::vector<int32_t> getFriendsIds(int32_t user_id);
    
    // 数据库操作方法
    bool updateUserStatusInDB(int32_t user_id, status::UserStatus status, const std::string& session_token);
    bool getUserStatusFromDB(int32_t user_id, status::UserStatus& status, std::chrono::time_point<std::chrono::system_clock>& last_seen);
    bool addFriendToDB(int32_t user_id, int32_t friend_id);
    bool friendExistsInDB(int32_t user_id, int32_t friend_id);
};

#endif // STATUS_SERVICE_IMPL_H