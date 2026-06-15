#ifndef ROBOT_HPP
#define ROBOT_HPP

#include <memory>
#include <string>
#include <vector>

// 这里要包含你的底层 RPC 头文件
#include "cpp_rpc.hpp"
#include "resp_dto.h"

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

// ============== 内联实现 ==============

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <cstdlib>
#include <ctime>
#include <iostream>

namespace robot {

inline void delay_ms(unsigned int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

inline std::shared_ptr<cpp_rpc::CPPClient> create_client(
    const std::string& ip
) {
    auto client = std::make_shared<cpp_rpc::CPPClient>(ip, 5868);
    return client;
}

inline void send_rpcsy(
    cpp_rpc::CPPClient& client,
    const std::vector<std::string>& cmd_list,
    int timeout_ms,
    int sleep_time_ms
) {
    for (const auto& cmd : cmd_list) {
        char* fake_argv[] = {const_cast<char*>("dummy"), const_cast<char*>(cmd.c_str())};

        srand(static_cast<unsigned int>(time(nullptr)));
        int seq_id = rand();

        std::string msg(fake_argv[1]);

        core::Msg sync_msg(msg);
        sync_msg.setMsgID(10001);
        sync_msg.setMsgSeqID(seq_id);

        auto res = client.CallAwait<RespDemo>(sync_msg, timeout_ms);
        if (0 == res.first) {
            std::cout << "*************Sync***************" << std::endl;
            std::cout << "model size:" << res.second.size() << std::endl;
            for (const auto& r : res.second) {
                std::cout << "subcmd_index:" << r.subcmd_index << std::endl;
                std::cout << "return_code:" << r.return_code << std::endl;
                std::cout << "return_message:" << r.return_message << std::endl;
            }
            std::cout << "****************************" << std::endl;
        } else {
            std::cout << "同步请求失败!确保timeout大于指令生效时间!错误码: " << res.first << std::endl;
        }

        std::cout << "over!!!" << std::endl;
        std::cout << "****************************" << std::endl;
        delay_ms(sleep_time_ms);
    }
}

inline void send_rpc_async(
    cpp_rpc::CPPClient& client,
    const std::vector<std::string>& cmd_list,
    int timeout_ms,
    int wait_time_ms
) {
    for (const auto& cmd : cmd_list) {
        char* fake_argv[] = {const_cast<char*>("dummy"), const_cast<char*>(cmd.c_str())};

        srand(static_cast<unsigned int>(time(nullptr)));
        int seq_id = rand();

        std::string msg(fake_argv[1]);

        core::Msg message(msg);
        client.CallAsync<RespDemo>(message, timeout_ms, [&](int ret, std::vector<RespDemo> res) {
            std::cout << "**************Async**************" << std::endl;
            if (ret < 0) {
                std::cout << "Async request failed. ret:" << ret << " " << "out time !" << std::endl;
            }

            std::cout << "model size:" << res.size() << std::endl;
            for (const auto& r : res) {
                std::cout << "subcmd_index:" << r.subcmd_index << std::endl;
                std::cout << "return_code:" << r.return_code << std::endl;
                std::cout << "return_message:" << r.return_message << std::endl;
            }
            std::cout << "*********************************" << std::endl;
        });

        core::Msg sync_msg(msg);
        sync_msg.setMsgID(10001);
        sync_msg.setMsgSeqID(seq_id);

        delay_ms(wait_time_ms);
    }
}

} // namespace robot

#endif // ROBOT_HPP
