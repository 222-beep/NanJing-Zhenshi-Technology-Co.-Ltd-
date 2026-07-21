cmake_minimum_required(VERSION 3.12)

project(message)
#set(CMAKE_CONFIGURATION_TYPES Release)

# 统一 C++17 标准（GCC/Clang/MSVC 均生效，避免 inline 变量、结构化绑定等特性报警告）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_DIR "${CMAKE_SOURCE_DIR}/../../common/rpc/c++")
include_directories("${COMMON_DIR}/include")
include_directories("${COMMON_DIR}/include/message")

if(UNIX)
    # 架构检测
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
        set(LIB_ARCH_DIR "arm")
        set(DEFAULT_LIB_UBUNTU_VERSION "2204")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|i386|i686")
        set(LIB_ARCH_DIR "x86")
        set(DEFAULT_LIB_UBUNTU_VERSION "2004")
    else()
        message(WARNING "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}, using x86")
        set(LIB_ARCH_DIR "x86")
        set(DEFAULT_LIB_UBUNTU_VERSION "2004")
    endif()

    # Ubuntu version directory. Override with:
    #   cmake -DLIB_UBUNTU_VERSION=2204 ..
    if(NOT DEFINED LIB_UBUNTU_VERSION)
        if(EXISTS "/etc/os-release")
            file(STRINGS "/etc/os-release" OS_RELEASE_VERSION REGEX "^VERSION_ID=")
            string(REGEX REPLACE "^VERSION_ID=\"?([0-9]+)\\.([0-9]+).*" "\\1\\2" DETECTED_UBUNTU_VERSION "${OS_RELEASE_VERSION}")
            if(DETECTED_UBUNTU_VERSION MATCHES "^(2004|2204)$")
                set(LIB_UBUNTU_VERSION "${DETECTED_UBUNTU_VERSION}")
            else()
                set(LIB_UBUNTU_VERSION "${DEFAULT_LIB_UBUNTU_VERSION}")
            endif()
        else()
            set(LIB_UBUNTU_VERSION "${DEFAULT_LIB_UBUNTU_VERSION}")
        endif()
    endif()

    # 设置库目录
    set(LIB_BASE_DIR "${COMMON_DIR}/lib/linux")
    set(SDK_LIB_DIR "${LIB_BASE_DIR}/${LIB_ARCH_DIR}/${LIB_UBUNTU_VERSION}/Release")
    message(STATUS "Using Linux rpc library: ${SDK_LIB_DIR}")
    link_directories("${SDK_LIB_DIR}")
elseif(WIN32)
    set(SDK_LIB_DIR "${COMMON_DIR}/lib/win/Release")
    link_directories("${SDK_LIB_DIR}")
endif()

# 兼容新版本codeit
add_definitions(-DCODEIT_1_3_0)
add_definitions(-DCPP_RPC_LIB)
if(UNIX)
add_definitions("-g")
endif(UNIX)

if(MSVC)
	add_compile_definitions(NOMINMAX)
	# /Zc:preprocessor 需要 VS 2019 16.6+ (MSVC_VERSION >= 1926)
	# VS 2017 不支持此选项，仅启用 /std:c++17 /utf-8
	if(MSVC_VERSION GREATER_EQUAL 1926)
		add_compile_options(/std:c++17 /Zc:preprocessor /utf-8)
	else()
		add_compile_options(/std:c++17 /utf-8)
	endif()
endif(MSVC)

add_executable(Hello_ForcePositionHybridControl src/main.cpp)

if(WIN32)
target_link_libraries(Hello_ForcePositionHybridControl cpp_rpc)

# 编译后自动把 dll 复制到 exe 旁边（否则运行时会报"找不到 dll"）
add_custom_command(TARGET Hello_ForcePositionHybridControl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDK_LIB_DIR}/cpp_rpc.dll"
        "$<TARGET_FILE_DIR:Hello_ForcePositionHybridControl>"
)

elseif(UNIX)
target_link_libraries(Hello_ForcePositionHybridControl cpp_rpc pthread)

endif()