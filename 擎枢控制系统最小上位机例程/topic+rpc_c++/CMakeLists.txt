cmake_minimum_required(VERSION 3.12)

project(topic_rpc_demo CXX)

set(CMAKE_CONFIGURATION_TYPES Release)
set(CMAKE_CXX_STANDARD 17)

# Topic SDK（订阅状态）与 RPC SDK（指令通道）共用宏
add_definitions(-DCODEIT_1_3_0)

include_directories(include)
# Do not add include/message globally: a file named qos.h there shadows <qos.h> from the Windows SDK.

set(SDK_LIB_VARIANT "Release" CACHE STRING "预编译库子目录名（如 Release / Debug）")

option(protoc "Enable proto code generation" ON)

set(PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
set(GENERATED_PROTO_DIR "${CMAKE_BINARY_DIR}/generated_proto")
set(GENERATED_SRCS)
set(GENERATED_HDRS)

if(protoc)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PROTOC_EXECUTABLE "${CMAKE_SOURCE_DIR}/lib/win/${SDK_LIB_VARIANT}/protoc.exe")
  else()
    find_program(PROTOC_EXECUTABLE protoc)
  endif()
  if(NOT PROTOC_EXECUTABLE)
    message(FATAL_ERROR "protoc not found")
  endif()

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

if(UNIX)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
        set(LIB_ARCH_DIR "arm")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|i386|i686")
        set(LIB_ARCH_DIR "x86")
    else()
        message(WARNING "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}, using x86")
        set(LIB_ARCH_DIR "x86")
    endif()

    set(LIB_BASE_DIR "${CMAKE_SOURCE_DIR}/lib/linux")
    set(LINUX_PREBUILT_LIB_DIR "${LIB_BASE_DIR}/${LIB_ARCH_DIR}/${SDK_LIB_VARIANT}")
    message(STATUS "Linux prebuilt libs: ${LINUX_PREBUILT_LIB_DIR}")

    foreach(_prebuilt IN ITEMS libprotobuf.so libzmq.so libmessage.so libcpp_rpc.so librobot_sdk.so)
        set(_prebuilt_path "${LINUX_PREBUILT_LIB_DIR}/${_prebuilt}")
        if(NOT EXISTS "${_prebuilt_path}")
            message(FATAL_ERROR "Missing prebuilt library: ${_prebuilt_path}")
        endif()
    endforeach()
elseif(WIN32)
    link_directories("${CMAKE_SOURCE_DIR}/lib/win/${SDK_LIB_VARIANT}")
endif()

if(MSVC)
	add_compile_options(/std:c++17 /Zc:preprocessor /utf-8)
endif(MSVC)

set(MAIN_SRC src/main.cpp)

include_directories(${GENERATED_PROTO_DIR})

add_executable(${PROJECT_NAME} ${MAIN_SRC} ${GENERATED_SRCS})

if(WIN32)
# cpp_rpc.hpp defines methods inline; do not mark the class dllimport on the exe target.
target_compile_definitions(${PROJECT_NAME} PRIVATE PROTOBUF_USE_DLLS CPP_RPC_NO_IMPORT)
target_link_libraries(${PROJECT_NAME}
    libprotobuf
    libzmq-v142-mt-4_3_6
    message
    cpp_rpc
    robot_sdk
)
elseif(UNIX)
target_compile_definitions(${PROJECT_NAME} PRIVATE CPP_RPC_LIB)
target_link_libraries(${PROJECT_NAME} PRIVATE
    "${LINUX_PREBUILT_LIB_DIR}/libprotobuf.so"
    "${LINUX_PREBUILT_LIB_DIR}/libzmq.so"
    "${LINUX_PREBUILT_LIB_DIR}/libmessage.so"
    "${LINUX_PREBUILT_LIB_DIR}/libcpp_rpc.so"
    "${LINUX_PREBUILT_LIB_DIR}/librobot_sdk.so"
    pthread
)
endif()
