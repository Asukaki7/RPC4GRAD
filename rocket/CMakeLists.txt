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
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address  -fsanitize-recover=address  -fno-omit-frame-pointer")

# 头文件包含路径
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/rocket
    ${CMAKE_SOURCE_DIR}/rocket/common
    ${CMAKE_SOURCE_DIR}/rocket/net
    ${CMAKE_SOURCE_DIR}/rocket/net/TCP
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
file(GLOB NET_TCP_SOURCES rocket/net/TCP/*.cpp)

add_library(rocket STATIC ${COMMON_SOURCES} ${NET_SOURCES} ${NET_TCP_SOURCES})
target_link_libraries(rocket ${PROTOBUF_LIB} ${TINYXML_LIB})


set(TEST_CASES
    log         # 对应 testcases/test_log.cpp
    eventloop   # 对应 testcases/test_eventloop.cpp
    IOthread    # 对应 testcases/test_IOthread.cpp
    tcp         # 对应 testcases/test_tcp.cpp
    client
)

# 公共链接库配置
set(TEST_LIBS rocket ${PROTOBUF_LIB} ${TINYXML_LIB} dl pthread)

# 批量生成测试用例
foreach(test_case IN LISTS TEST_CASES)
    # 生成可执行文件名（如 test_log）
    set(target_name "test_${test_case}")
    # 生成源文件路径（如 testcases/test_log.cpp）
    set(source_file "testcases/test_${test_case}.cpp")
    
    # 添加可执行文件
    add_executable(${target_name} ${source_file})
    # 链接库
    target_link_libraries(${target_name} ${TEST_LIBS})
endforeach()


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

install(DIRECTORY rocket/net/TCP
    DESTINATION /usr/include/rocket/net/TCP
    FILES_MATCHING PATTERN "*.h"
)

# 卸载目标（需要手动执行）
add_custom_target(uninstall
    COMMAND rm -rf /usr/include/rocket /usr/lib/librocket.a
    COMMENT "Removing installed files"
)