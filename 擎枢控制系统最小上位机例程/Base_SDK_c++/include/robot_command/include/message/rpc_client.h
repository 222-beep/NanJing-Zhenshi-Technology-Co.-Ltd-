// rpc_client.h -- RPC 通信封装（同步 / 异步）
#pragma once

#include "message/resp_dto.h"
#include "util.hpp"
#include "cpp_rpc.hpp"
#include <atomic>
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
                std::function<void(int, const std::vector<RespType>&, int, const std::string&)> response_callback = nullptr)
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

        auto res = client.CallAwait<RespType>(sync_msg, outim_num);
        if (response_callback) {
            response_callback(res.first, res.second, seq, cmd);
        }

        if (res.first == 0) {
            if (debug) {
                std::cout << "*************Sync[seq=" << seq << "]***************" << std::endl;
                std::cout << "model size:" << res.second.size() << std::endl;
                for (const auto& r : res.second) {
                    std::cout << "subcmd_index:" << r.subcmd_index << std::endl;
                    std::cout << "return_code:" << r.return_code << std::endl;
                    std::cout << "return_message:" << r.return_message << std::endl;
                    RespPrinter<RespType>::print_extra(r);
                }
                std::cout << "*********over!!!**************" << std::endl;
                std::cout << std::endl;
            }
            all_results.insert(all_results.end(), res.second.begin(), res.second.end());
        } else {
            if (debug) {
                std::cout << "Synchronous request failed! "
                             "Ensure that the timeout is greater than the command execution time! "
                             "Error code: " << res.first << std::endl;
            }

            if (!client.IsConnected()) {
                std::cerr << "Connection lost after send failure! Aborting remaining commands." << std::endl;
                std::cerr << "Error: " << client.GetErrorInfo() << std::endl;
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
                        std::function<void(int, const core::Msg&, int, const std::string&)> response_callback = nullptr) {
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

        bool sent = client.CallAsyncRaw(message, outim_num,
            [debug, response_callback, seq, cmd](int ret, const core::Msg& msg_resp) {
                if (response_callback) {
                    response_callback(ret, msg_resp, seq, cmd);
                }
                if (debug) {
                    std::cout << "**************Async[seq=" << seq << "]**************" << std::endl;
                    if (ret < 0) {
                        std::cout << "Async request failed. ret:" << ret << " out time !" << std::endl;
                    }
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

        delay_ms(wait_num);
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
                                         std::function<void(int, const core::Msg&, int, const std::string&)> response_callback = nullptr) {
    return std::async(std::launch::async,
        [&client, cmd = std::move(cmd), outim_num, debug, response_callback]() -> bool {
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

            bool sent = client.CallAsyncRaw(message, outim_num,
                [debug, response_callback, seq, cmd](int ret, const core::Msg& msg_resp) {
                    if (response_callback) {
                        response_callback(ret, msg_resp, seq, cmd);
                    }
                    if (debug) {
                        std::cout << "**************Thread[seq=" << seq << "]**************" << std::endl;
                        if (ret < 0) {
                            std::cout << "Thread request failed. ret:" << ret << " out time !" << std::endl;
                        }
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

// 新增响应类型：在 resp_dto.h 中添加结构体 + RespPrinter 特化，见文件底部模板。
