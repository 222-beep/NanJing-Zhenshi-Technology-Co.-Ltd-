// rpc_client.h  ——  RPC 通信封装（同步 / 异步）
#pragma once

#include "resp_dto.h"
#include "util.hpp"
#include "cpp_rpc.hpp"
#include <atomic>
#include <future>
#include <iostream>

// ======================================================================
//  跨平台延时
// ======================================================================

#ifdef _WIN32
#include <windows.h>
inline void delay_ms(unsigned int ms) { Sleep(ms); }
#else
#include <unistd.h>
inline void delay_ms(unsigned int ms) { usleep(ms * 1000); }
#endif

// ======================================================================
//  自增消息序列 ID（从 1 开始）
// ======================================================================

inline int next_msg_seq_id() {
    static std::atomic<int> id{0};
    return ++id;
}

// ======================================================================
//  通用同步 RPC（模板，支持扩展响应类型）
//
//  用法:
//    send_rpcsy<RespDemo>(client, cmds, interval_ms, timeout_ms);
//    send_rpcsy<PointChooseIDMoveResp>(client, cmds, interval_ms, timeout_ms);
// ======================================================================

template<typename RespType>
auto send_rpcsy(cpp_rpc::CPPClient& client, const std::vector<std::string>& cmd_cmd, int sleep_num = 0, int outim_num = 864000000) -> std::vector<RespType> {
    std::vector<RespType> all_results;
    for (const auto& cmd : cmd_cmd) {
        // 连接已断开，不再继续发送后续指令
        if (!client.IsConnected()) {
            std::cerr << "Connection lost! Aborting remaining commands." << std::endl;
            std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
            break;
        }

        int i = next_msg_seq_id();

        std::cout << std::endl;
        std::cout << "send[seq=" << i << "]: " << cmd << std::endl;

        core::Msg sync_msg(cmd);
        sync_msg.setMsgID(10001);
        sync_msg.setMsgSeqID(i);

        auto res = client.CallAwait<RespType>(sync_msg, outim_num);
        if (0 == res.first) {
            std::cout << "*************Sync[seq=" << i << "]***************" << std::endl;
            std::cout << "model size:" << res.second.size() << std::endl;
            for (const auto& r : res.second) {
                std::cout << "subcmd_index:" << r.subcmd_index << std::endl;
                std::cout << "return_code:" << r.return_code << std::endl;
                std::cout << "return_message:" << r.return_message << std::endl;
                RespPrinter<RespType>::print_extra(r);
            }
            std::cout << "*********over!!!**************" << std::endl;
            std::cout << std::endl;
            all_results.insert(all_results.end(), res.second.begin(), res.second.end());
        }
        else {
            std::cout << "Synchronous request failed! "
                         "Ensure that the timeout is greater than the command execution time! "
                         "Error code: " << res.first << std::endl;

            // 发送失败后检查连接状态，若已断开则停止后续指令
            if (!client.IsConnected()) {
                std::cerr << "Connection lost after send failure! Aborting remaining commands." << std::endl;
                break;
            }
        }

        delay_ms(sleep_num);
    }
    return all_results;
}

// ======================================================================
//  通用异步 RPC
//
//  用法:
//    send_rpcAsy(client, cmds, wait_ms, timeout_ms);
// ======================================================================

inline void send_rpcAsy(cpp_rpc::CPPClient& client, const std::vector<std::string>& cmd_cmd, int wait_num = 0, int outim_num = 864000000) {
    for (const auto& cmd : cmd_cmd) {
        // 连接已断开，不再继续发送后续指令
        if (!client.IsConnected()) {
            std::cerr << "Connection lost! Aborting remaining commands." << std::endl;
            break;
        }

        int i = next_msg_seq_id();

        std::cout << std::endl;
        std::cout << "send[seq=" << i << "]: " << cmd << std::endl;

        core::Msg message(cmd);
        message.setMsgID(10001);
        message.setMsgSeqID(i);

        client.CallAsyncRaw(message, outim_num, [&](int ret, const core::Msg& msg_resp) {
            std::cout << "**************Async[seq=" << msg_resp.seqID() << "]**************" << std::endl;
            if (ret < 0) {
                std::cout << "Async request failed. ret:" << ret << " out time !" << std::endl;
            }
            std::string body(msg_resp.data(), msg_resp.size());
            std::cout << "response: " << body << std::endl;
            std::cout << "*********************************" << std::endl << std::endl;
            });

        delay_ms(wait_num);
    }
}

// ======================================================================
//  Stop 专用快速下发封装
//
//  用法:
//    auto stop_future = send_stop_async(client);
//
//  说明:
//    普通同步 RPC 等待返回时，调用线程会被阻塞。Stop 属于打断类指令，
//    可从其他线程/控制入口调用本函数，由 std::async 单独线程发送 {Stop}。
// ======================================================================

inline std::future<bool> send_stop_async(cpp_rpc::CPPClient& client, int outim_num = 10000) {
    return std::async(std::launch::async, [&client, outim_num]() -> bool {
        if (!client.IsConnected()) {
            std::cerr << "Connection lost! Stop command not sent." << std::endl;
            return false;
        }

        int i = next_msg_seq_id();

        std::cout << std::endl;
        std::cout << "send stop[seq=" << i << "]: {Stop}" << std::endl;

        core::Msg message("{Stop}");
        message.setMsgID(10001);
        message.setMsgSeqID(i);

        bool sent = client.CallAsyncRaw(message, outim_num, [](int ret, const core::Msg& msg_resp) {
            std::cout << "**************Stop[seq=" << msg_resp.seqID() << "]**************" << std::endl;
            if (ret < 0) {
                std::cout << "Stop request failed. ret:" << ret << " out time !" << std::endl;
            }
            std::string body(msg_resp.data(), msg_resp.size());
            std::cout << "response: " << body << std::endl;
            std::cout << "********************************" << std::endl << std::endl;
            });

        if (!sent) {
            std::cerr << "Failed to send stop command." << std::endl;
        }
        return sent;
    });
}

//  新增响应类型：在 resp_dto.h 中添加结构体 + RespPrinter 特化，见文件底部模板。