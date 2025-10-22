cmake_minimum_required(VERSION 3.16)

project(ChatServices VERSION 1.0 LANGUAGES CXX)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# 编译选项
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# 查找所需的包
find_package(Threads REQUIRED)

# 查找Boost库 (用于异步网络编程)
find_package(Boost 1.71 REQUIRED COMPONENTS system thread)

# 查找gRPC (用于服务间通信)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++ grpc)

# 查找MySQL客户端库
pkg_check_modules(MYSQLCLIENT REQUIRED mysqlclient)

# 查找hiredis (Redis C客户端)
find_path(HIREDIS_INCLUDE_DIR NAMES hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY NAMES hiredis)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${GRPC_INCLUDE_DIRS})
include_directories(${MYSQLCLIENT_INCLUDE_DIRS})
include_directories(${HIREDIS_INCLUDE_DIR})

# 链接库目录
link_directories(${GRPC_LIBRARY_DIRS})
link_directories(${MYSQLCLIENT_LIBRARY_DIRS})

# 添加依赖测试程序
add_executable(test_dependencies test_dependencies.cpp)

# 链接测试程序所需的库
target_link_libraries(test_dependencies PRIVATE
    ${Boost_LIBRARIES}
    ${GRPC_LIBRARIES}
    ${MYSQLCLIENT_LIBRARIES}
    ${HIREDIS_LIBRARY}
    Threads::Threads
)

# 添加子目录（各个微服务）
add_subdirectory(GateServer)
add_subdirectory(StatusServer)
add_subdirectory(VarifyServer)