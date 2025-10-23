# 状态服务 - 负责管理用户在线状态

cmake_minimum_required(VERSION 3.16)

project(StatusServer VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    status_service_impl.cpp
    ../utils/database_manager.cpp
    ../utils/crypto_utils.cpp
)

# 链接所需库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Boost_LIBRARIES}
    ${GRPC_LIBRARIES}
    ${MYSQLCLIENT_LIBRARIES}
    ${HIREDIS_LIBRARY}
    Threads::Threads
    proto-generated
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
    ${PROTO_GENERATED_DIR}
)

# 链接Protobuf和gRPC库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Protobuf_LIBRARIES}
    ${GRPC_LIBRARIES}
)