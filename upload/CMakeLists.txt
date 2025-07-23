cmake_minimum_required(VERSION 3.10)
project(dnsrelay C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# 源文件列表，包含子目录下的所有源文件
set(SOURCES
    main.c
    DNSHandle/DNSHandle.c
    DNSHandle/dns_message.c
    DNSHandle/IdConversion.c
    Initialization/io.c
    Initialization/multiThread.c
    LookUp/data_struct.c
    Platform/platformSocket.c
    Platform/platformThread.c
    Debug/debug.c
)

# 头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/DNSHandle
    ${CMAKE_SOURCE_DIR}/Initialization
    ${CMAKE_SOURCE_DIR}/LookUp
    ${CMAKE_SOURCE_DIR}/Log
    ${CMAKE_SOURCE_DIR}/Platform
    ${CMAKE_SOURCE_DIR}/Debug
)

# 可执行文件
add_executable(dnsrelay ${SOURCES})

# 链接 Windows socket 库
if (WIN32)
    target_link_libraries(dnsrelay wsock32 ws2_32)
endif()