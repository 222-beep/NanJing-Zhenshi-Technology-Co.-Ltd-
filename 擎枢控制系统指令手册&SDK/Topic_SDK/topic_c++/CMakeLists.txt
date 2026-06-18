cmake_minimum_required(VERSION 3.12)

project(topic)
set(CMAKE_CONFIGURATION_TYPES Release)
set(CMAKE_CXX_STANDARD 17)
add_definitions(-DCODEIT_1_3_0)

set(TOPIC_COMMON_DIR "${CMAKE_SOURCE_DIR}/../../common/topic")
set(TOPIC_INCLUDE_DIR "${TOPIC_COMMON_DIR}/c++/include")

include_directories("${TOPIC_INCLUDE_DIR}")
include_directories("${TOPIC_INCLUDE_DIR}/message")

option(protoc "Enable proto code generation" ON)

set(PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
set(GENERATED_PROTO_DIR "${CMAKE_BINARY_DIR}/generated_proto")
set(GENERATED_SRCS)
set(GENERATED_HDRS)

if(protoc)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PROTOC_EXECUTABLE "${TOPIC_COMMON_DIR}/c++/lib/win/Release/protoc.exe")
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
    # Ubuntu 版本选择（20.04 / 22.04）
    set(UBUNTU_VERSION "20.04" CACHE STRING "Ubuntu version (20.04 or 22.04)")
    if(NOT UBUNTU_VERSION MATCHES "20.04|22.04")
        message(WARNING "Unknown Ubuntu version: ${UBUNTU_VERSION}, using 20.04")
        set(UBUNTU_VERSION "20.04")
    endif()

    # 架构检测
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
        set(LIB_ARCH_DIR "arm")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|i386|i686")
        set(LIB_ARCH_DIR "x86")
    else()
        message(WARNING "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}, using x86")
        set(LIB_ARCH_DIR "x86")
    endif()

    # 设置库目录：shared 存放跨语言共用依赖，c++/lib 存放 C++ 专属或不可共用依赖
    set(TOPIC_SHARED_FULL_DIR "${TOPIC_COMMON_DIR}/shared/lib/linux/${LIB_ARCH_DIR}/${UBUNTU_VERSION}/Release")
    set(TOPIC_CPP_LIB_FULL_DIR "${TOPIC_COMMON_DIR}/c++/lib/linux/${LIB_ARCH_DIR}/${UBUNTU_VERSION}/Release")
    if(EXISTS "${TOPIC_SHARED_FULL_DIR}")
        link_directories("${TOPIC_SHARED_FULL_DIR}")
    endif()
    if(EXISTS "${TOPIC_CPP_LIB_FULL_DIR}")
        link_directories("${TOPIC_CPP_LIB_FULL_DIR}")
    else()
        message(WARNING "C++ library directory not found: ${TOPIC_CPP_LIB_FULL_DIR}")
    endif()
elseif(WIN32)
    set(TOPIC_SHARED_FULL_DIR "${TOPIC_COMMON_DIR}/shared/lib/win/Release")
    set(TOPIC_CPP_LIB_FULL_DIR "${TOPIC_COMMON_DIR}/c++/lib/win/Release")
    link_directories("${TOPIC_CPP_LIB_FULL_DIR}")
    if(EXISTS "${TOPIC_SHARED_FULL_DIR}")
        link_directories("${TOPIC_SHARED_FULL_DIR}")
    endif()
endif()

if(MSVC)
	add_compile_options(/std:c++17 /Zc:preprocessor /utf-8)
endif(MSVC)

 SET(test src/main.cpp)

include_directories(${GENERATED_PROTO_DIR}) 

add_executable(${PROJECT_NAME} ${test} ${GENERATED_SRCS})

if(WIN32)
target_compile_definitions(${PROJECT_NAME} PRIVATE PROTOBUF_USE_DLLS)
target_link_libraries(${PROJECT_NAME} libprotobuf)
target_link_libraries(${PROJECT_NAME} libzmq-v142-mt-4_3_6)
target_link_libraries(${PROJECT_NAME} message)

# 构建后将依赖 DLL 复制到可执行文件目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TOPIC_CPP_LIB_FULL_DIR}/libprotobuf.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TOPIC_CPP_LIB_FULL_DIR}/libprotoc.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TOPIC_SHARED_FULL_DIR}/libzmq-v142-mt-4_3_6.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TOPIC_CPP_LIB_FULL_DIR}/message.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying runtime DLLs to output directory"
)
elseif(UNIX)
target_link_libraries(${PROJECT_NAME} libprotobuf.so)
target_link_libraries(${PROJECT_NAME} libzmq.so)
target_link_libraries(${PROJECT_NAME} libmessage.so)
target_link_libraries(${PROJECT_NAME} pthread)
endif()