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
set(GENERATED_SRCS)
set(GENERATED_HDRS)

if(protoc)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PROTOC_EXECUTABLE "${CMAKE_SOURCE_DIR}/lib/win/Release/protoc.exe")
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
    set(LIB_BASE_DIR "${CMAKE_SOURCE_DIR}/lib/linux")
    link_directories("${LIB_BASE_DIR}/${LIB_ARCH_DIR}/Release")
elseif(WIN32)
    # Windows 可根据需求添加架构判断，这里保持原样
    link_directories("${CMAKE_SOURCE_DIR}/lib/win/Release")
endif()

if(MSVC)
	add_compile_options(/std:c++17 /Zc:preprocessor)
endif(MSVC)

 SET(test src/topic_sub_direct.cpp)

include_directories(${GENERATED_PROTO_DIR}) 

add_executable(${PROJECT_NAME} ${test} ${GENERATED_SRCS})

if(WIN32)
target_compile_definitions(${PROJECT_NAME} PRIVATE PROTOBUF_USE_DLLS)
target_link_libraries(${PROJECT_NAME} libprotobuf)
target_link_libraries(${PROJECT_NAME} libzmq-v142-mt-4_3_6)
target_link_libraries(${PROJECT_NAME} message)
elseif(UNIX)
target_link_libraries(${PROJECT_NAME} libprotobuf.so)
target_link_libraries(${PROJECT_NAME} libzmq.so)
target_link_libraries(${PROJECT_NAME} libmessage.so)
target_link_libraries(${PROJECT_NAME} pthread)
endif()