cmake_minimum_required(VERSION 3.12)

project(Base_SDK_cxx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES Release)

add_definitions(-DCODEIT_1_3_0)
add_definitions(-DCPP_RPC_LIB)

option(SMAL_SYS_ENABLE_PROTOC "Generate topic proto sources" ON)

set(PROTO_DIR "${CMAKE_SOURCE_DIR}/include/robot_state/proto")
set(GENERATED_PROTO_DIR "${CMAKE_BINARY_DIR}/generated_proto")
set(BUNDLED_GENERATED_PROTO_DIR "${CMAKE_SOURCE_DIR}/include/robot_state/proto_generated")
set(GENERATED_SRCS)
file(GLOB BUNDLED_GENERATED_SRCS "${BUNDLED_GENERATED_PROTO_DIR}/*.pb.cc")

if(NOT BUNDLED_GENERATED_SRCS)
    message(FATAL_ERROR "Bundled generated protobuf sources not found: ${BUNDLED_GENERATED_PROTO_DIR}")
endif()

if(SMAL_SYS_ENABLE_PROTOC)
    if(WIN32)
        set(PROTOC_EXECUTABLE "${CMAKE_SOURCE_DIR}/include/robot_state/lib/win/Release/protoc.exe")
        if(NOT EXISTS "${PROTOC_EXECUTABLE}")
            unset(PROTOC_EXECUTABLE)
        endif()
    else()
        find_program(PROTOC_EXECUTABLE protoc)
    endif()

    if(NOT PROTOC_EXECUTABLE)
        message(WARNING "protoc not found; using bundled generated protobuf sources")
        set(GENERATED_SRCS ${BUNDLED_GENERATED_SRCS})
    else()
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
else()
    set(GENERATED_SRCS ${BUNDLED_GENERATED_SRCS})
endif()

if(UNIX)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SMAL_SYS_PROCESSOR)
    if(SMAL_SYS_PROCESSOR MATCHES "^(aarch64|arm64)$")
        set(LIB_ARCH_DIR "arm")
    elseif(SMAL_SYS_PROCESSOR MATCHES "^(x86_64|amd64)$")
        set(LIB_ARCH_DIR "x86")
    else()
        message(FATAL_ERROR
            "Unsupported Linux architecture: ${CMAKE_SYSTEM_PROCESSOR}. "
            "Bundled libraries support x86_64 and ARM64 (aarch64) only.")
    endif()

    set(RPC_LIB_DIR
        "${CMAKE_SOURCE_DIR}/include/robot_command/lib/linux/${LIB_ARCH_DIR}")
    if(NOT EXISTS "${RPC_LIB_DIR}/libcpp_rpc.so")
        message(FATAL_ERROR
            "Bundled RPC library not found: ${RPC_LIB_DIR}/libcpp_rpc.so")
    endif()

    set(TOPIC_LIB_DIR
        "${CMAKE_SOURCE_DIR}/include/robot_state/lib/linux/${LIB_ARCH_DIR}")
    if(NOT EXISTS "${TOPIC_LIB_DIR}/libmessage.so")
        message(FATAL_ERROR
            "Bundled Topic library not found: ${TOPIC_LIB_DIR}/libmessage.so. "
            "Check the robot_state library package and Ubuntu version.")
    endif()

    message(STATUS "Using Linux RPC library: ${RPC_LIB_DIR}")
    message(STATUS "Using Linux Topic library: ${TOPIC_LIB_DIR}")
elseif(WIN32)
    link_directories("${CMAKE_SOURCE_DIR}/include/robot_command/lib/win/Release")
    link_directories("${CMAKE_SOURCE_DIR}/include/robot_state/lib/win/Release")
endif()

set(APP_TARGET Base_SDK_cxx)

add_executable(${APP_TARGET}
    src/main.cpp
    ${GENERATED_SRCS}
)

set_target_properties(${APP_TARGET} PROPERTIES
    OUTPUT_NAME "Base_SDK_c++"
)

target_include_directories(${APP_TARGET} PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/robot_command/include"
    "${CMAKE_SOURCE_DIR}/include/robot_command/include/message"
    "${CMAKE_SOURCE_DIR}/include/robot_state/include"
    "${CMAKE_SOURCE_DIR}/include/robot_state/message_struct"
    "${GENERATED_PROTO_DIR}"
    "${BUNDLED_GENERATED_PROTO_DIR}"
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
        "${RPC_LIB_DIR}/libcpp_rpc.so"
        "${TOPIC_LIB_DIR}/libprotobuf.so"
        "${TOPIC_LIB_DIR}/libzmq.so"
        "${TOPIC_LIB_DIR}/libmessage.so"
        pthread
    )

    set(LINUX_RUNTIME_LIBS
        "${RPC_LIB_DIR}/libcpp_rpc.so"
        "${TOPIC_LIB_DIR}/libprotobuf.so"
        "${TOPIC_LIB_DIR}/libprotobuf.so.32"
        "${TOPIC_LIB_DIR}/libzmq.so"
        "${TOPIC_LIB_DIR}/libzmq.so.5"
        "${TOPIC_LIB_DIR}/libmessage.so"
    )
    foreach(LINUX_RUNTIME_LIB ${LINUX_RUNTIME_LIBS})
        add_custom_command(TARGET ${APP_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LINUX_RUNTIME_LIB}"
                "$<TARGET_FILE_DIR:${APP_TARGET}>"
        )
    endforeach()

    # The executable and all bundled shared libraries are placed side by side.
    # Use a relative RPATH so the build output remains relocatable after copying.
    set_target_properties(${APP_TARGET} PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN"
    )
endif()
