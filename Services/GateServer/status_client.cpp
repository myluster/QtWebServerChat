#include "status_client.h"
#include <iostream>

StatusClient::StatusClient(std::shared_ptr<Channel> channel)
    : stub_(StatusService::NewStub(channel)) {}

bool StatusClient::UpdateUserStatus(int32_t user_id, status::UserStatus status, 
                                   const std::string& session_token, std::string& message) {
    UserStatusRequest request;
    UserStatusResponse response;
    ClientContext context;
    
    request.set_user_id(user_id);
    request.set_status(status);
    request.set_session_token(session_token);
    
    Status status_grpc = stub_->UpdateUserStatus(&context, request, &response);
    
    if (!status_grpc.ok()) {
        message = "gRPC error: " + status_grpc.error_message();
        return false;
    }
    
    message = response.message();
    return response.success();
}

bool StatusClient::GetUserStatus(int32_t user_id, status::UserStatus& status, 
                                int64_t& last_seen, std::string& message) {
    GetUserStatusRequest request;
    GetUserStatusResponse response;
    ClientContext context;
    
    request.set_user_id(user_id);
    
    Status status_grpc = stub_->GetUserStatus(&context, request, &response);
    
    if (!status_grpc.ok()) {
        message = "gRPC error: " + status_grpc.error_message();
        return false;
    }
    
    if (!response.success()) {
        message = response.message();
        return false;
    }
    
    status = response.status();
    last_seen = response.last_seen();
    message = response.message();
    return true;
}

bool StatusClient::GetFriendsStatus(int32_t user_id, 
                                   std::vector<status::FriendStatus>& friends_status, 
                                   std::string& message) {
    GetFriendsStatusRequest request;
    GetFriendsStatusResponse response;
    ClientContext context;
    
    request.set_user_id(user_id);
    
    Status status_grpc = stub_->GetFriendsStatus(&context, request, &response);
    
    if (!status_grpc.ok()) {
        message = "gRPC error: " + status_grpc.error_message();
        return false;
    }
    
    if (!response.success()) {
        message = response.message();
        return false;
    }
    
    // 复制好友状态信息
    for (const auto& friend_status : response.friends()) {
        friends_status.push_back(friend_status);
    }
    
    message = response.message();
    return true;
}