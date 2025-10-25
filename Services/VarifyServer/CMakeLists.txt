# 验证服务 - 负责用户身份验证和会话管理

cmake_minimum_required(VERSION 3.16)

project(VarifyServer VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    ../utils/logger.cpp
    ../utils/load_balancer.cpp
    ../utils/service_registry.cpp
    ../utils/health_checker.cpp
    ../utils/redis_manager.cpp
)

# 链接所需库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Boost_LIBRARIES}
    ${GRPC_LIBRARIES}
    ${MYSQLCLIENT_LIBRARIES}
    ${HIREDIS_LIBRARY}
    Threads::Threads
    spdlog::spdlog
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
)