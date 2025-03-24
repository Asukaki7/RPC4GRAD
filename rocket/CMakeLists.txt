cmake_minimum_required(VERSION 3.10)
project(rocket)

# 设置编译器
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 全局编译选项
add_compile_options(
    -g
    -O0
    -Wall
    -Wno-deprecated
    -Wno-unused-but-set-variable
)

# 头文件包含路径
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/rocket
    ${CMAKE_SOURCE_DIR}/rocket/common
    ${CMAKE_SOURCE_DIR}/rocket/net
)

# 查找依赖库
find_library(PROTOBUF_LIB NAMES protobuf protobuf.a PATHS /usr/lib)
find_library(TINYXML_LIB NAMES tinyxml tinyxml.a PATHS /usr/lib)

if(NOT PROTOBUF_LIB OR NOT TINYXML_LIB)
    message(FATAL_ERROR "Required libraries not found")
endif()

# 构建静态库 librocket.a
file(GLOB COMMON_SOURCES rocket/common/*.cpp)
file(GLOB NET_SOURCES rocket/net/*.cpp)

add_library(rocket STATIC ${COMMON_SOURCES} ${NET_SOURCES})
target_link_libraries(rocket ${PROTOBUF_LIB} ${TINYXML_LIB})

# 构建测试用例
add_executable(test_log testcases/test_log.cpp)
target_link_libraries(test_log rocket ${PROTOBUF_LIB} ${TINYXML_LIB} dl pthread)

# 安装配置
install(TARGETS rocket
    ARCHIVE DESTINATION /usr/lib
    LIBRARY DESTINATION /usr/lib
)

install(DIRECTORY rocket/common/
    DESTINATION /usr/include/rocket/common
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY rocket/net/
    DESTINATION /usr/include/rocket/net
    FILES_MATCHING PATTERN "*.h"
)

# 卸载目标（需要手动执行）
add_custom_target(uninstall
    COMMAND rm -rf /usr/include/rocket /usr/lib/librocket.a
    COMMENT "Removing installed files"
)