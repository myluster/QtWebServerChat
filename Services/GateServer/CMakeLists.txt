# 网关服务 - 负责处理客户端连接和消息路由

cmake_minimum_required(VERSION 3.16)

project(GateServer VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找所需的库
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    http_session.cpp
    websocket_session.cpp
    listener.cpp
    status_client.cpp
    connection_manager.cpp
    ../utils/database_manager.cpp
    ../utils/crypto_utils.cpp
)

# 链接所需库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Boost_LIBRARIES}
    ${GRPC_LIBRARIES}
    ${MYSQLCLIENT_LIBRARIES}
    ${HIREDIS_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    nlohmann_json::nlohmann_json
    proto-generated
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
    ${OPENSSL_INCLUDE_DIR}
    ${PROTO_GENERATED_DIR}
)