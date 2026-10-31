cmake_minimum_required(VERSION 3.12)

project(topic)
set(CMAKE_CONFIGURATION_TYPES Release)
set(CMAKE_CXX_STANDARD 17)
add_definitions(-DCODEIT_1_3_0)

include_directories(include)
include_directories(include/message)

option(protoc "Enable proto code generation" ON)

set(PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
set(GENERATED_PROTO_DIR "${CMAKE_BINARY_DIR}/generated_proto")
set(PREGENERATED_PROTO_DIR "${CMAKE_SOURCE_DIR}/proto_generated")
set(GENERATED_SRCS)
set(GENERATED_HDRS)

if(protoc)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PROTOC_EXECUTABLE "${CMAKE_SOURCE_DIR}/lib/win/Release/protoc.exe")
  else()
    find_program(PROTOC_EXECUTABLE protoc)
  endif()
  if(NOT PROTOC_EXECUTABLE)
    message(WARNING "protoc not found -- using pre-generated pb files")
  else()
    file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

    file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")
    foreach(PROTO_FILE ${PROTO_FILES})
      get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)

      set(GEN_SRC "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.cc")
      set(GEN_HDR "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.h")

      add_custom_command(
        OUTPUT ${GEN_SRC} ${GEN_HDR}
        COMMAND ${PROTOC_EXECUTABLE}
          --proto_path=${PROTO_DIR}
          --cpp_out=${GENERATED_PROTO_DIR}
          ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Running protoc on ${PROTO_NAME}.proto"
      )

      list(APPEND GENERATED_SRCS ${GEN_SRC})
      list(APPEND GENERATED_HDRS ${GEN_HDR})
    endforeach()
  endif()
endif()

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
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
        set(LIB_ARCH_DIR "arm")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
        set(LIB_ARCH_DIR "x86")
    else()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}; supported: x86_64, AArch64")
    endif()

    # Linux 库统一使用 Ubuntu 20.04 ABI 基线，目录仅按架构区分。
    set(LIB_BASE_DIR "${CMAKE_SOURCE_DIR}/lib/linux")
    set(LIB_FULL_DIR "${LIB_BASE_DIR}/${LIB_ARCH_DIR}")
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
    # Windows 可根据需求添加架构判断，这里保持原样
    link_directories("${CMAKE_SOURCE_DIR}/lib/win/Release")
endif()

if(MSVC)
	add_compile_options(/std:c++17 /Zc:preprocessor /utf-8)
endif(MSVC)

add_executable(${PROJECT_NAME} src/main.cpp ${PROTO_SRCS})
add_executable(topic_frequency src/topic_frequency.cpp ${PROTO_SRCS})
add_executable(topic_sub_direct src/topic_sub_direct.cpp ${PROTO_SRCS})
add_executable(topic_sub_snapshot src/topic_sub_snapshot.cpp ${PROTO_SRCS})
set(TOPIC_TARGETS
    ${PROJECT_NAME}
    topic_frequency
    topic_sub_direct
    topic_sub_snapshot
)

if(WIN32)
foreach(TARGET_NAME ${TOPIC_TARGETS})
target_compile_definitions(${TARGET_NAME} PRIVATE PROTOBUF_USE_DLLS)
target_link_libraries(${TARGET_NAME} libprotobuf libzmq-v142-mt-4_3_6 message)
endforeach()

# 构建后将依赖 DLL 复制到可执行文件目录
set(LIB_DLL_DIR "${CMAKE_SOURCE_DIR}/lib/win/Release")
foreach(TARGET_NAME ${TOPIC_TARGETS})
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIB_DLL_DIR}/libprotobuf.dll"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIB_DLL_DIR}/libprotoc.dll"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIB_DLL_DIR}/libzmq-v142-mt-4_3_6.dll"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIB_DLL_DIR}/message.dll"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>"
    COMMENT "Copying runtime DLLs to output directory"
)
endforeach()
elseif(UNIX)
foreach(TARGET_NAME ${TOPIC_TARGETS})
target_link_libraries(${TARGET_NAME} PRIVATE
    "${LIB_FULL_DIR}/libprotobuf.so"
    "${LIB_FULL_DIR}/libzmq.so"
    "${LIB_FULL_DIR}/libmessage.so"
    pthread
)
endforeach()
endif()
