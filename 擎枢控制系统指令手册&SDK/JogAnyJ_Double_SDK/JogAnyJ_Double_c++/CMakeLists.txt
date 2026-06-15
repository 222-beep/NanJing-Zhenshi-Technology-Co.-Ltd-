cmake_minimum_required(VERSION 3.12)

project(message)
#set(CMAKE_CONFIGURATION_TYPES Release)

set(COMMON_DIR "${CMAKE_SOURCE_DIR}/../../common/rpc/c++")
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
    # Windows 可根据需求添加架构判断，这里保持原样
    set(SDK_LIB_DIR "${COMMON_DIR}/lib/win/${SDK_LIB_VARIANT}")
    link_directories("${SDK_LIB_DIR}")
endif()

# 兼容新版本codeit
add_definitions(-DCODEIT_1_3_0)
add_definitions(-DCPP_RPC_LIB)
if(UNIX)
add_definitions("-g")
endif(UNIX)

if(MSVC)
	# /Zc:preprocessor 需要 VS 2019 16.6+ (MSVC_VERSION >= 1926)
	# VS 2017 不支持此选项，仅启用 /std:c++17 /utf-8
	if(MSVC_VERSION GREATER_EQUAL 1926)
		add_compile_options(/std:c++17 /Zc:preprocessor /utf-8)
	else()
		add_compile_options(/std:c++17 /utf-8)
	endif()
endif(MSVC)

add_executable(Hello_JogAnyJ3_Double src/main.cpp)

if(WIN32)
target_link_libraries(Hello_JogAnyJ3_Double cpp_rpc)

# 编译后自动把 dll 复制到 exe 旁边（否则运行时会报"找不到 dll"）
add_custom_command(TARGET Hello_JogAnyJ3_Double POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDK_LIB_DIR}/cpp_rpc.dll"
        "$<TARGET_FILE_DIR:Hello_JogAnyJ3_Double>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDK_LIB_DIR}/robot_sdk.dll"
        "$<TARGET_FILE_DIR:Hello_JogAnyJ3_Double>"
)

elseif(UNIX)
target_link_libraries(Hello_JogAnyJ3_Double cpp_rpc pthread)

endif()