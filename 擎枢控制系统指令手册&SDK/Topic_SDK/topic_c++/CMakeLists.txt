cmake_minimum_required(VERSION 3.12)

project(topic)
set(CMAKE_CONFIGURATION_TYPES Release)
set(CMAKE_CXX_STANDARD 17)
add_definitions(-DCODEIT_1_3_0)

# 公共库统一收口在 common/topic/，禁止在 Topic_SDK 内独立复制 include/ 和 lib/
set(TOPIC_COMMON_DIR "${CMAKE_SOURCE_DIR}/../../common/topic")
set(TOPIC_INCLUDE_DIR "${TOPIC_COMMON_DIR}/c++/include")

include_directories("${TOPIC_INCLUDE_DIR}")
include_directories("${TOPIC_INCLUDE_DIR}/message")

option(protoc "Enable proto code generation" ON)

set(PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
set(GENERATED_PROTO_DIR "${CMAKE_BINARY_DIR}/generated_proto")
set(PREGENERATED_PROTO_DIR "${CMAKE_SOURCE_DIR}/proto_generated")
set(GENERATED_SRCS)
set(GENERATED_HDRS)

if(protoc)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PROTOC_EXECUTABLE "${TOPIC_COMMON_DIR}/c++/lib/win/Release/protoc.exe")
  else()
    find_program(PROTOC_EXECUTABLE protoc)
  endif()
  if(NOT PROTOC_EXECUTABLE)
    message(WARNING "protoc not found -- using pre-generated pb files")
  else()
    file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

    # 以纯相对路径调用 protoc，规避绝对路径含非 ASCII（如中文）字符时的解析失败
    file(RELATIVE_PATH PROTO_DIR_REL "${CMAKE_BINARY_DIR}" "${PROTO_DIR}")
    file(RELATIVE_PATH GENERATED_PROTO_DIR_REL "${CMAKE_BINARY_DIR}" "${GENERATED_PROTO_DIR}")

    file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")
    foreach(PROTO_FILE ${PROTO_FILES})
      get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)

      set(GEN_SRC "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.cc")
      set(GEN_HDR "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.h")

      add_custom_command(
        OUTPUT ${GEN_SRC} ${GEN_HDR}
        COMMAND ${PROTOC_EXECUTABLE}
          --proto_path=${PROTO_DIR_REL}
          --cpp_out=${GENERATED_PROTO_DIR_REL}
          ${PROTO_NAME}.proto
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${PROTO_FILE}
        COMMENT "Running protoc on ${PROTO_NAME}.proto"
      )

      list(APPEND GENERATED_SRCS ${GEN_SRC})
      list(APPEND GENERATED_HDRS ${GEN_HDR})
    endforeach()
  endif()
endif()

# protoc 可用时在线生成 pb 代码；否则降级使用 proto_generated/ 预生成文件
if(protoc AND PROTOC_EXECUTABLE AND GENERATED_SRCS)
  set(PROTO_SRCS ${GENERATED_SRCS})
  include_directories(${GENERATED_PROTO_DIR})
else()
  set(PROTO_SRCS
    "${PREGENERATED_PROTO_DIR}/overall_system_rtstate.pb.cc"
    "${PREGENERATED_PROTO_DIR}/overall_system_nrtstate.pb.cc"
  )
  foreach(PREGENERATED_FILE
      "${PREGENERATED_PROTO_DIR}/overall_system_rtstate.pb.h"
      "${PREGENERATED_PROTO_DIR}/overall_system_rtstate.pb.cc"
      "${PREGENERATED_PROTO_DIR}/overall_system_nrtstate.pb.h"
      "${PREGENERATED_PROTO_DIR}/overall_system_nrtstate.pb.cc")
    if(NOT EXISTS "${PREGENERATED_FILE}")
      message(FATAL_ERROR "Pre-generated protobuf file not found: ${PREGENERATED_FILE}")
    endif()
  endforeach()
  include_directories(${PREGENERATED_PROTO_DIR})
endif()

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

    # Linux 库统一使用 Ubuntu 20.04 ABI 基线，目录仅按架构区分
    set(LIB_FULL_DIR "${TOPIC_COMMON_DIR}/c++/lib/linux/${LIB_ARCH_DIR}")
    if(NOT EXISTS "${LIB_FULL_DIR}")
        message(FATAL_ERROR "Library directory not found: ${LIB_FULL_DIR}")
    endif()
    foreach(REQUIRED_LIB libprotobuf.so libzmq.so libmessage.so)
        if(NOT EXISTS "${LIB_FULL_DIR}/${REQUIRED_LIB}")
            message(FATAL_ERROR "Required library not found: ${LIB_FULL_DIR}/${REQUIRED_LIB}")
        endif()
    endforeach()
    message(STATUS "Linux library directory: ${LIB_FULL_DIR} (Ubuntu 20.04 ABI baseline)")
elseif(WIN32)
    set(TOPIC_LIB_DLL_DIR "${TOPIC_COMMON_DIR}/c++/lib/win/Release")
    link_directories("${TOPIC_LIB_DLL_DIR}")
endif()

if(MSVC)
	add_compile_options(/std:c++17 /Zc:preprocessor /utf-8)
endif(MSVC)

 SET(test src/main.cpp)

add_executable(${PROJECT_NAME} ${test} ${PROTO_SRCS})

if(WIN32)
target_compile_definitions(${PROJECT_NAME} PRIVATE PROTOBUF_USE_DLLS)
target_link_libraries(${PROJECT_NAME} libprotobuf)
target_link_libraries(${PROJECT_NAME} libzmq-v142-mt-4_3_6)
target_link_libraries(${PROJECT_NAME} message)

# 构建后将依赖 DLL 复制到可执行文件目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TOPIC_LIB_DLL_DIR}/libprotobuf.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TOPIC_LIB_DLL_DIR}/libprotoc.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TOPIC_LIB_DLL_DIR}/libzmq-v142-mt-4_3_6.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TOPIC_LIB_DLL_DIR}/message.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying runtime DLLs to output directory"
)
elseif(UNIX)
target_link_libraries(${PROJECT_NAME} PRIVATE
    "${LIB_FULL_DIR}/libprotobuf.so"
    "${LIB_FULL_DIR}/libzmq.so"
    "${LIB_FULL_DIR}/libmessage.so"
    pthread
)
endif()
