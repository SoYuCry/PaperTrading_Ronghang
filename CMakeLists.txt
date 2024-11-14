cmake_minimum_required(VERSION 3.10)
project(CMakeDemo)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)

# 添加 spdlog 和 API 的 include 路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/spdlog-1.x/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/API)

# 设置库文件所在目录
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/API)

# 添加可执行文件
add_executable(my_program main.cpp)

# 包含 libcurl 的头文件路径
target_include_directories(my_program PRIVATE ${CURL_INCLUDE_DIR})

# 查找 libcurl
set(CMAKE_PREFIX_PATH "D:/Documents/GooleChromDownloads/curl-8.11.0/vcpkg/installed/x64-windows")
find_package(CURL REQUIRED)
message(STATUS " CURL_INCLUDE_DIR: ${CURL_INCLUDE_DIR}, ${CURL_LIBRARIES}")

# 静态链接设置（可选）
set(CURL_STATICLIB ON)

# 统一使用关键字形式链接库
target_link_libraries(my_program PRIVATE CURL::libcurl)

# 链接 CTP 库
target_link_libraries(my_program PRIVATE thostmduserapi_se thosttraderapi_se)
