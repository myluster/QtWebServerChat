#ifndef STATUS_CLIENT_H
#define STATUS_CLIENT_H

#include <memory>
#include <string>
#include <vector>
#include <grpcpp/grpcpp.h>
#include "../generated/status.grpc.pb.h"

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
using status::FriendInfo;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class StatusClient {
public:
    StatusClient(std::shared_ptr<Channel> channel);
    
    // 更新用户状态
    bool UpdateUserStatus(int32_t user_id, status::UserStatus status, 
                         const std::string& session_token, std::string& message);
    
    // 获取用户状态
    bool GetUserStatus(int32_t user_id, status::UserStatus& status, 
                      int64_t& last_seen, std::string& message);
    
    // 获取好友列表状态
    bool GetFriendsStatus(int32_t user_id, 
                         std::vector<status::FriendStatus>& friends_status, 
                         std::string& message);
    
    // 添加好友
    bool AddFriend(int32_t user_id, int32_t friend_id, std::string& message);
    
    // 获取好友列表
    bool GetFriendsList(int32_t user_id, std::vector<status::FriendInfo>& friends, std::string& message);

private:
    std::unique_ptr<StatusService::Stub> stub_;
};

#endif // STATUS_CLIENT_H