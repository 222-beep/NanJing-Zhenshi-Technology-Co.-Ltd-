cmake_minimum_required(VERSION 3.12)

project(MoveSeriesToppJ2)
#set(CMAKE_CONFIGURATION_TYPES Release)

set(COMMON_DIR "${CMAKE_SOURCE_DIR}/../../common")
include_directories("${COMMON_DIR}/include")
include_directories("${COMMON_DIR}/include/message")

set(SDK_LIB_VARIANT "Release" CACHE STRING "SDK library variant directory (for example: Release/Debug/V1)")

if(UNIX)
    # 架构检测
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
        set(LIB_ARCH_DIR "arm")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|i386|i686")
        set(LIB_ARCH_DIR "x86")
    else()
        message(WARNING "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}, using x86")
        set(LIB_ARCH_DIR "x86")
    endif()

    # 设置库目录
    set(LIB_BASE_DIR "${COMMON_DIR}/lib/linux")
    set(SDK_LIB_DIR "${LIB_BASE_DIR}/${LIB_ARCH_DIR}/${SDK_LIB_VARIANT}")
    link_directories("${SDK_LIB_DIR}")
elseif(WIN32)
    # Windows 平台库目录
    set(SDK_LIB_DIR "${COMMON_DIR}/lib/win/${SDK_LIB_VARIANT}")
    link_directories("${SDK_LIB_DIR}")
endif()

# 兼容新版本codeit
add_definitions(-DCODEIT_1_3_0)
add_definitions(-DCPP_RPC_LIB)
if(UNIX)
add_definitions("-g")
endif(UNIX)

# MSVC 必须使用符合标准的预处理器，否则 JSON_HELP 宏中的 FOR_EACH 无法正确展开
if(MSVC)
    add_compile_options(/std:c++17 /Zc:preprocessor)
endif(MSVC)

add_executable(MoveSeriesToppJ2 src/main.cpp)

if(WIN32)
    target_link_libraries(MoveSeriesToppJ2 cpp_rpc robot_sdk)
elseif(UNIX)
    target_link_libraries(MoveSeriesToppJ2 cpp_rpc robot_sdk pthread)
endif()
