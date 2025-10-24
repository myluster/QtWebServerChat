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
find_package(Boost 1.71 REQUIRED COMPONENTS system thread json)

# 查找gRPC (用于服务间通信)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++ grpc)

# 查找Protobuf
find_package(Protobuf REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)

# 查找MySQL客户端库
pkg_check_modules(MYSQLCLIENT REQUIRED mysqlclient)

# 查找hiredis (Redis C客户端)
find_path(HIREDIS_INCLUDE_DIR NAMES hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY NAMES hiredis)

# 查找nlohmann/json库
find_package(nlohmann_json REQUIRED)

# 查找spdlog库
find_package(spdlog REQUIRED)

# Proto文件设置
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_FILES ${PROTO_DIR}/status.proto)
set(PROTO_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# 创建生成目录
file(MAKE_DIRECTORY ${PROTO_GENERATED_DIR})

# 生成Protobuf和gRPC源文件
set(PROTO_SRCS)
set(PROTO_HDRS)
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(GRPC_SRC ${PROTO_GENERATED_DIR}/${PROTO_NAME}.grpc.pb.cc)
    set(GRPC_HDR ${PROTO_GENERATED_DIR}/${PROTO_NAME}.grpc.pb.h)
    set(PROTO_SRC ${PROTO_GENERATED_DIR}/${PROTO_NAME}.pb.cc)
    set(PROTO_HDR ${PROTO_GENERATED_DIR}/${PROTO_NAME}.pb.h)
    
    add_custom_command(
        OUTPUT ${GRPC_SRC} ${GRPC_HDR} ${PROTO_SRC} ${PROTO_HDR}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --grpc_out=${PROTO_GENERATED_DIR}
             --cpp_out=${PROTO_GENERATED_DIR}
             -I${PROTO_DIR}
             --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
    )
    
    list(APPEND PROTO_SRCS ${GRPC_SRC} ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${GRPC_HDR} ${PROTO_HDR})
endforeach()

# 添加生成的源文件到项目
add_library(proto-generated ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(proto-generated PUBLIC ${PROTO_GENERATED_DIR})
target_link_libraries(proto-generated 
    ${Protobuf_LIBRARIES}
    ${GRPC_LIBRARIES}
)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${GRPC_INCLUDE_DIRS})
include_directories(${MYSQLCLIENT_INCLUDE_DIRS})
include_directories(${HIREDIS_INCLUDE_DIR})
include_directories(${PROTO_GENERATED_DIR})

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
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# 添加子目录（各个微服务）
add_subdirectory(GateServer)
add_subdirectory(StatusServer)
add_subdirectory(VarifyServer)