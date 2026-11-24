cmake_minimum_required(VERSION 3.12)

project(Base_function_SDK_cxx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES Release)

add_definitions(-DCODEIT_1_3_0)
add_definitions(-DCPP_RPC_LIB)

option(BASE_FUNCTION_SDK_ENABLE_PROTOC "Generate topic proto sources" ON)

set(APP_TARGET Base_function_SDK_cxx)
set(PROTO_DIR "${CMAKE_SOURCE_DIR}/include/robot_state/proto")
set(GENERATED_PROTO_DIR "${CMAKE_BINARY_DIR}/generated_proto")
set(GENERATED_SRCS)

if(BASE_FUNCTION_SDK_ENABLE_PROTOC)
    if(WIN32)
        set(PROTOC_EXECUTABLE "${CMAKE_SOURCE_DIR}/include/robot_state/lib/win/Release/protoc.exe")
    else()
        find_program(PROTOC_EXECUTABLE protoc)
    endif()

    if(NOT PROTOC_EXECUTABLE)
        message(FATAL_ERROR "protoc not found")
    endif()

    file(MAKE_DIRECTORY "${GENERATED_PROTO_DIR}")
    file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")
    foreach(PROTO_FILE ${PROTO_FILES})
        get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)
        set(GEN_SRC "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.cc")
        set(GEN_HDR "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.h")

        add_custom_command(
            OUTPUT "${GEN_SRC}" "${GEN_HDR}"
            COMMAND "${PROTOC_EXECUTABLE}"
                --proto_path="${PROTO_DIR}"
                --cpp_out="${GENERATED_PROTO_DIR}"
                "${PROTO_FILE}"
            DEPENDS "${PROTO_FILE}"
            COMMENT "Running protoc on ${PROTO_NAME}.proto"
        )
        list(APPEND GENERATED_SRCS "${GEN_SRC}")
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

    set(UBUNTU_VERSION "20.04" CACHE STRING "Ubuntu version (20.04 or 22.04)")
    link_directories("${CMAKE_SOURCE_DIR}/include/robot_command/lib/linux/${LIB_ARCH_DIR}/Release")
    link_directories("${CMAKE_SOURCE_DIR}/include/robot_state/lib/linux/${LIB_ARCH_DIR}/${UBUNTU_VERSION}/Release")
elseif(WIN32)
    link_directories("${CMAKE_SOURCE_DIR}/include/robot_command/lib/win/Release")
    link_directories("${CMAKE_SOURCE_DIR}/include/robot_state/lib/win/Release")
endif()

add_executable(${APP_TARGET}
    src/main.cpp
    include/robot_command/src/command.cpp
    include/robot_command/src/multi_arm_command.cpp
    include/robot_command/src/dual_arm_command.cpp
    ${GENERATED_SRCS}
)

set_target_properties(${APP_TARGET} PROPERTIES
    OUTPUT_NAME "Base_function_SDK_c++"
)

target_include_directories(${APP_TARGET} PRIVATE
    "${CMAKE_SOURCE_DIR}/include/robot_command/include"
    "${CMAKE_SOURCE_DIR}/include/robot_command/include/message"
    "${CMAKE_SOURCE_DIR}/include/robot_command/include/util/reflection"
    "${CMAKE_SOURCE_DIR}/include/robot_state/include"
    "${CMAKE_SOURCE_DIR}/include/robot_state/message_struct"
    "${GENERATED_PROTO_DIR}"
)

if(MSVC)
    if(MSVC_VERSION GREATER_EQUAL 1926)
        target_compile_options(${APP_TARGET} PRIVATE /std:c++17 /Zc:preprocessor /utf-8)
    else()
        target_compile_options(${APP_TARGET} PRIVATE /std:c++17 /utf-8)
    endif()
    target_compile_definitions(${APP_TARGET} PRIVATE PROTOBUF_USE_DLLS WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(WIN32)
    target_link_libraries(${APP_TARGET} PRIVATE
        cpp_rpc
        libprotobuf
        libzmq-v142-mt-4_3_6
        message
    )

    set(RPC_DLL_DIR "${CMAKE_SOURCE_DIR}/include/robot_command/lib/win/Release")
    set(TOPIC_DLL_DIR "${CMAKE_SOURCE_DIR}/include/robot_state/lib/win/Release")
    add_custom_command(TARGET ${APP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RPC_DLL_DIR}/cpp_rpc.dll"
            "$<TARGET_FILE_DIR:${APP_TARGET}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TOPIC_DLL_DIR}/libprotobuf.dll"
            "$<TARGET_FILE_DIR:${APP_TARGET}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TOPIC_DLL_DIR}/libprotoc.dll"
            "$<TARGET_FILE_DIR:${APP_TARGET}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TOPIC_DLL_DIR}/libzmq-v142-mt-4_3_6.dll"
            "$<TARGET_FILE_DIR:${APP_TARGET}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TOPIC_DLL_DIR}/message.dll"
            "$<TARGET_FILE_DIR:${APP_TARGET}>"
    )
elseif(UNIX)
    target_link_libraries(${APP_TARGET} PRIVATE
        libcpp_rpc.so
        libprotobuf.so
        libzmq.so
        libmessage.so
        pthread
    )
endif()
