// rpc_client.h -- RPC 通信封装（同步 / 异步）
#pragma once

#include "message/resp_dto.h"
#include "util.hpp"
#include "cpp_rpc.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

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

struct RpcTimingInfo {
    int64_t seq_id = 0;
    int64_t born_time_ms = 0;
    int64_t return_time_ms = 0;
    int64_t header_delta_ms = 0;
    double client_rtt_ms = 0.0;
    bool server_time_valid = false;
};

inline RpcTimingInfo make_rpc_timing_info(
        int64_t seq_id,
        const core::Msg& response,
        std::chrono::steady_clock::time_point request_started,
        bool response_received,
        int64_t request_born_time) {
    RpcTimingInfo info;
    info.seq_id = seq_id;
    if (response_received) {
        info.born_time_ms = response.msgBornTime();
        info.return_time_ms = response.msgReturnTime();
    } else {
        // 超时或发送失败时，底层回调的默认Msg不代表原请求。
        info.born_time_ms = request_born_time;
        info.return_time_ms = 0;
    }
    info.client_rtt_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - request_started).count();
    info.server_time_valid = info.born_time_ms > 0 &&
                             info.return_time_ms > 0 &&
                             info.return_time_ms != 202401;
    if (info.server_time_valid) {
        info.header_delta_ms = info.return_time_ms - info.born_time_ms;
    }
    return info;
}

inline void print_rpc_timing(const RpcTimingInfo& info) {
    std::cout << "seq_id:" << info.seq_id << std::endl;
    std::cout << "born_time_ms:" << info.born_time_ms << std::endl;
    std::cout << "return_time_ms:" << info.return_time_ms << std::endl;
    if (info.server_time_valid) {
        std::cout << "header_delta_ms:" << info.header_delta_ms << std::endl;
    } else {
        std::cout << "header_delta_ms:unavailable" << std::endl;
    }
    std::cout << "client_rtt_ms:" << info.client_rtt_ms << std::endl;
}

// ======================================================================
//  通用同步 RPC（模板，支持扩展响应类型）
//
//  用法：
//    send_rpcsy<RespDemo>(client, cmds, interval_ms, timeout_ms);
//    send_rpcsy<PointChooseIDMoveResp>(client, cmds, interval_ms, timeout_ms);
//
//  debug 默认 false，不打印发送和返回信息。
//  response_callback 可选，签名：
//    void callback(int status, const std::vector<RespType>& resp, int seq, const std::string& cmd)
// ======================================================================

template<typename RespType>
auto send_rpcsy(cpp_rpc::CPPClient& client,
                const std::vector<std::string>& cmd_cmd,
                int sleep_num = 0,
                int outim_num = 864000000,
                bool debug = false,
                std::function<void(int, const std::vector<RespType>&, int, const std::string&)> response_callback = nullptr,
                std::function<void(const RpcTimingInfo&)> timing_callback = nullptr)
    -> std::vector<RespType> {
    std::vector<RespType> all_results;

    for (const auto& cmd : cmd_cmd) {
        if (!client.IsConnected()) {
            std::cerr << "Connection lost! Aborting remaining commands." << std::endl;
            std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
            break;
        }

        int seq = next_msg_seq_id();
        if (debug) {
            std::cout << std::endl;
            std::cout << "send[seq=" << seq << "]: " << cmd << std::endl;
        }

        core::Msg sync_msg(cmd);
        sync_msg.setMsgID(10001);
        sync_msg.setMsgSeqID(seq);
        const auto request_born_time = sync_msg.msgBornTime();

        const auto request_started = std::chrono::steady_clock::now();
        auto raw_res = client.CallAwaitRaw(sync_msg, outim_num);
        const int rpc_status = raw_res.first;
        int status = rpc_status;
        std::vector<RespType> responses;
        if (status == 0) {
            try {
                responses = nlohmann::json::parse(raw_res.second.toString())
                                .get<std::vector<RespType>>();
            } catch (const std::exception& e) {
                status = -1;
                if (debug) {
                    std::cerr << "JSON parse error: " << e.what() << std::endl;
                }
            }
        }
        const auto timing = make_rpc_timing_info(
            seq, raw_res.second, request_started,
            rpc_status == 0, request_born_time);
        if (response_callback) {
            response_callback(status, responses, seq, cmd);
        }
        if (timing_callback) {
            timing_callback(timing);
        }

        if (status == 0) {
            if (debug) {
                std::cout << "*************Sync[seq=" << seq << "]***************" << std::endl;
                print_rpc_timing(timing);
                std::cout << "model size:" << responses.size() << std::endl;
                for (const auto& r : responses) {
                    std::cout << "subcmd_index:" << r.subcmd_index << std::endl;
                    std::cout << "return_code:" << r.return_code << std::endl;
                    std::cout << "return_message:" << r.return_message << std::endl;
                    RespPrinter<RespType>::print_extra(r);
                }
                std::cout << "*********over!!!**************" << std::endl;
                std::cout << std::endl;
            }
            all_results.insert(all_results.end(), responses.begin(), responses.end());
        } else {
            if (debug) {
                std::cout << "Synchronous request failed! "
                             "Ensure that the timeout is greater than the command execution time! "
                             "Error code: " << status << std::endl;
                print_rpc_timing(timing);
            }

            if (!client.IsConnected()) {
                std::cerr << "Connection lost after send failure! Aborting remaining commands." << std::endl;
                std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
                break;
            }
        }

        if (sleep_num > 0) {
            delay_ms(static_cast<unsigned int>(sleep_num));
        }
    }

    return all_results;
}

// ======================================================================
//  通用异步 RPC
//
//  用法：
//    send_rpcAsy(client, cmds, wait_ms, timeout_ms);
//
//  debug 默认 false，不打印发送和返回信息。
//  response_callback 可选，签名：
//    void callback(int status, const core::Msg& resp, int seq, const std::string& cmd)
// ======================================================================

inline void send_rpcAsy(cpp_rpc::CPPClient& client,
                        const std::vector<std::string>& cmd_cmd,
                        int wait_num = 0,
                        int outim_num = 864000000,
                        bool debug = false,
                        std::function<void(int, const core::Msg&, int, const std::string&)> response_callback = nullptr,
                        std::function<void(const RpcTimingInfo&)> timing_callback = nullptr) {
    for (const auto& cmd : cmd_cmd) {
        if (!client.IsConnected()) {
            std::cerr << "Connection lost! Aborting remaining commands." << std::endl;
            std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
            break;
        }

        int seq = next_msg_seq_id();
        if (debug) {
            std::cout << std::endl;
            std::cout << "send[seq=" << seq << "]: " << cmd << std::endl;
        }

        core::Msg message(cmd);
        message.setMsgID(10001);
        message.setMsgSeqID(seq);
        const auto request_born_time = message.msgBornTime();

        const auto request_started = std::chrono::steady_clock::now();
        bool sent = client.CallAsyncRaw(message, outim_num,
            [debug, response_callback, timing_callback, seq, cmd,
             request_started, request_born_time]
            (int ret, const core::Msg& msg_resp) {
                const auto timing = make_rpc_timing_info(
                    seq, msg_resp, request_started,
                    ret == 0, request_born_time);
                if (response_callback) {
                    response_callback(ret, msg_resp, seq, cmd);
                }
                if (timing_callback) {
                    timing_callback(timing);
                }
                if (debug) {
                    std::cout << "**************Async[seq=" << seq << "]**************" << std::endl;
                    if (ret < 0) {
                        std::cout << "Async request failed. ret:" << ret << " out time !" << std::endl;
                    }
                    print_rpc_timing(timing);
                    std::string body(msg_resp.data(), msg_resp.size());
                    std::cout << "response: " << body << std::endl;
                    std::cout << "*********************************" << std::endl << std::endl;
                }
            });

        if (!sent) {
            if (!client.IsConnected()) {
                std::cerr << "Connection lost! Command not sent: " << cmd << std::endl;
                std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
            } else if (debug) {
                std::cerr << "Failed to send command: " << cmd << std::endl;
                std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
            }
        }

        if (wait_num > 0) {
            delay_ms(static_cast<unsigned int>(wait_num));
        }
    }
}

// ======================================================================
//  独立线程通用 RPC
//
//  用法：
//    auto send_future = send_rpc_thread(client, "{Stop}");
//
//  说明：
//    普通同步 RPC 等待返回时，调用线程会被阻塞。本接口使用 std::async
//    在独立线程中发送传入的任意指令，可从其他线程或控制入口调用。
//
//  debug 默认 false，不打印发送和返回信息。
//  response_callback 可选，签名：
//    void callback(int status, const core::Msg& resp, int seq, const std::string& cmd)
// ======================================================================

inline std::future<bool> send_rpc_thread(cpp_rpc::CPPClient& client,
                                         std::string cmd,
                                         int outim_num = 10000,
                                         bool debug = false,
                                         std::function<void(int, const core::Msg&, int, const std::string&)> response_callback = nullptr,
                                         std::function<void(const RpcTimingInfo&)> timing_callback = nullptr) {
    return std::async(std::launch::async,
        [&client, cmd = std::move(cmd), outim_num, debug, response_callback, timing_callback]() -> bool {
            if (!client.IsConnected()) {
                std::cerr << "Connection lost! Command not sent: " << cmd << std::endl;
                std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
                return false;
            }

            int seq = next_msg_seq_id();
            if (debug) {
                std::cout << std::endl;
                std::cout << "send thread[seq=" << seq << "]: " << cmd << std::endl;
            }

            core::Msg message(cmd);
            message.setMsgID(10001);
            message.setMsgSeqID(seq);
            const auto request_born_time = message.msgBornTime();

            const auto request_started = std::chrono::steady_clock::now();
            bool sent = client.CallAsyncRaw(message, outim_num,
                [debug, response_callback, timing_callback, seq, cmd,
                 request_started, request_born_time]
                (int ret, const core::Msg& msg_resp) {
                    const auto timing = make_rpc_timing_info(
                        seq, msg_resp, request_started,
                        ret == 0, request_born_time);
                    if (response_callback) {
                        response_callback(ret, msg_resp, seq, cmd);
                    }
                    if (timing_callback) {
                        timing_callback(timing);
                    }
                    if (debug) {
                        std::cout << "**************Thread[seq=" << seq << "]**************" << std::endl;
                        if (ret < 0) {
                            std::cout << "Thread request failed. ret:" << ret << " out time !" << std::endl;
                        }
                        print_rpc_timing(timing);
                        std::string body(msg_resp.data(), msg_resp.size());
                        std::cout << "response: " << body << std::endl;
                        std::cout << "********************************" << std::endl << std::endl;
                    }
                });

            if (!sent) {
                if (!client.IsConnected()) {
                    std::cerr << "Connection lost! Command not sent: " << cmd << std::endl;
                    std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
                } else if (debug) {
                    std::cerr << "Failed to send command: " << cmd << std::endl;
                    std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
                }
            }
            return sent;
        });
}

// ======================================================================
//  5900实时关节目标发送
//
//  说明：
//    5900端口不是普通字符串指令通道，只接受FastJointTargetParam固定二进制帧：
//    5组机械臂 × 每组10个关节 × double，共400字节。
//    每次调用只发送一帧，发送周期由调用方控制。
//    使用前需通过5868普通RPC启动JogAnyJFast，并设置--index=0。
//    目标必须持续刷新，停止发送后不应认为机械臂会继续运动到目标位置。
//
//  返回值：
//    Ok(0) / NotConnected(-1) / SendFailed(-2) / TimedOut(-3)
// ======================================================================

inline cpp_rpc::RealtimeSendResult send_realtime_joint_target(
    cpp_rpc::RealtimeChannel& channel,
    const cpp_rpc::FastJointTargetParam& target) {
    return channel.Send(target);
}

// 新增响应类型：在 resp_dto.h 中添加结构体 + RespPrinter 特化，见文件底部模板。
