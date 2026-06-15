#ifndef ROBOT_HPP
#define ROBOT_HPP

#include <memory>
#include <string>
#include <vector>

#include "cpp_rpc.hpp"
#include "resp_dto.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <cstdlib>
#include <ctime>
#include <iostream>

namespace robot {

// ======================================================================
//  跨平台延时
// ======================================================================

inline void delay_ms(unsigned int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

// ======================================================================
//  创建 RPC 客户端
// ======================================================================

inline std::shared_ptr<cpp_rpc::CPPClient> create_client(const std::string& ip) {
    auto client = std::make_shared<cpp_rpc::CPPClient>(ip, 5868);
    return client;
}

// ======================================================================
//  同步发送 RPC 指令（模板，支持扩展响应类型）
//
//  用法:
//    robot::send_rpcsy(*client, cmds, timeout_ms, sleep_ms);
//    robot::send_rpcsy<PointChooseIDMoveResp>(*client, cmds, timeout_ms, sleep_ms);
// ======================================================================

template<typename RespType = RespDemo>
inline void send_rpcsy(
    cpp_rpc::CPPClient& client,
    const std::vector<std::string>& cmd_list,
    int timeout_ms,
    int sleep_time_ms
) {
    for (const auto& cmd : cmd_list) {
        std::cout << std::endl;
        std::cout << "send: " << cmd << std::endl;

        srand(static_cast<unsigned int>(time(nullptr)));
        int seq_id = rand();

        core::Msg sync_msg(cmd);
        sync_msg.setMsgID(10001);
        sync_msg.setMsgSeqID(seq_id);

        auto res = client.CallAwait<RespType>(sync_msg, timeout_ms);
        if (0 == res.first) {
            std::cout << "*************Sync***************" << std::endl;
            std::cout << "model size:" << res.second.size() << std::endl;
            for (const auto& r : res.second) {
                std::cout << "subcmd_index:" << r.subcmd_index << std::endl;
                std::cout << "return_code:" << r.return_code << std::endl;
                std::cout << "return_message:" << r.return_message << std::endl;
                RespPrinter<RespType>::print_extra(r);
            }
            std::cout << "*********over!!!**************" << std::endl;
            std::cout << std::endl;
        } else {
            std::cout << "同步请求失败! 确保timeout大于指令生效时间! 错误码: " << res.first << std::endl;
        }

        delay_ms(sleep_time_ms);
    }
}

// ======================================================================
//  异步发送 RPC 指令（使用 CallAsyncRaw + callback）
//
//  用法:
//    robot::send_rpc_async(*client, cmds, timeout_ms, wait_ms);
// ======================================================================

inline void send_rpc_async(
    cpp_rpc::CPPClient& client,
    const std::vector<std::string>& cmd_list,
    int timeout_ms,
    int wait_time_ms
) {
    for (const auto& cmd : cmd_list) {
        std::cout << std::endl;
        std::cout << "send: " << cmd << std::endl;

        core::Msg message(cmd);

        client.CallAsyncRaw(message, timeout_ms, [](int ret, const core::Msg& msg_resp) {
            std::cout << "**************Async**************" << std::endl;
            if (ret < 0) {
                std::cout << "Async request failed. ret:" << ret << " out time !" << std::endl;
            }
            std::string body(msg_resp.data(), msg_resp.size());
            std::cout << "response: " << body << std::endl;
            std::cout << "*********************************" << std::endl << std::endl;
        });

        delay_ms(wait_time_ms);
    }
}

} // namespace robot

#endif // ROBOT_HPP
