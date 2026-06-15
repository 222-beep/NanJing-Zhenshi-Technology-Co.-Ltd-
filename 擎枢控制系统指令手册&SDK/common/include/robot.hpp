#ifndef ROBOT_HPP
#define ROBOT_HPP

#include <memory>
#include <string>
#include <vector>

// 这里要包含你的底层 RPC 头文件
// 这个名字要根据你实际工程里的文件名修改
// 如果你的 CPPClient 定义在 cpp_rpc.hpp 里，就用这个
#include "cpp_rpc.hpp"

#if defined(_WIN32) && defined(ROBOT_SDK_BUILD_DLL)
#define ROBOT_API __declspec(dllexport)
#elif defined(_WIN32)
#define ROBOT_API __declspec(dllimport)
#else
#define ROBOT_API
#endif

namespace robot {

/**
 * @brief 创建 RPC 客户端
 *
 * @param ip 控制器 IP 地址
 * @return std::shared_ptr<cpp_rpc::CPPClient>
 */
ROBOT_API std::shared_ptr<cpp_rpc::CPPClient> create_client(
    const std::string& ip
);

/**
 * @brief 同步发送 RPC 指令
 *
 * @param client RPC 客户端
 * @param cmd_list 指令列表
 * @param timeout_ms 超时时间，单位 ms
 * @param sleep_time_ms 每条指令发送后的等待时间，单位 ms
 */
ROBOT_API void send_rpcsy(
    cpp_rpc::CPPClient& client,
    const std::vector<std::string>& cmd_list,
    int timeout_ms,
    int sleep_time_ms
);

/**
 * @brief 异步发送 RPC 指令
 *
 * @param client RPC 客户端
 * @param cmd_list 指令列表
 * @param timeout_ms 超时时间，单位 ms
 * @param wait_time_ms 每条指令发送间隔，单位 ms
 */
ROBOT_API void send_rpc_async(
    cpp_rpc::CPPClient& client,
    const std::vector<std::string>& cmd_list,
    int timeout_ms,
    int wait_time_ms
);

} // namespace robot

#undef ROBOT_API

#endif // ROBOT_HPP