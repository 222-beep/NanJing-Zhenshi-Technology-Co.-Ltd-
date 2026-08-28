#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "message/rpc_client.h"
#include "system_state_reader.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace smal_sys {

inline constexpr int kRpcPort = 5868;
inline constexpr int kModelIndex = 0;
inline constexpr int kRpcTimeoutMs = 5000;

inline const std::vector<std::string> kInitCmds = {
    "{Clear}",
    "{Disable}",
    "{Mode}",
    "{SetMaxToq}",
    "{Recover}",
    "{SetRate}",
    "{Enable}",
};

// 连接机器人指令客户端。端口号封装在这里，main 里只需要传入 IP。
inline std::unique_ptr<cpp_rpc::CPPClient> connectRpc(const std::string& ip) {
    auto client = std::make_unique<cpp_rpc::CPPClient>(ip, kRpcPort);
    if (!client->IsConnected()) {
        throw std::runtime_error(std::string("Robot command connection failed: ") + client->GetErrorInfo());
    }
    return client;
}

// 启动机器人状态订阅。内部固定端口封装在 SDK 里，main 不需要关心。
inline void startTopic(const std::string& ip) {
    start_subscriber(ip);
}

// 下发控制器初始化指令，例如 Clear、Enable 等。
inline void initializeController(cpp_rpc::CPPClient& client) {
    send_rpcsy<RespDemo>(client, kInitCmds, 100, kRpcTimeoutMs);
}

// 完成状态订阅、指令连接和控制器初始化。
// IP 从 main 传入，这样客户只需要在 main 里改一个 IP 变量。
inline std::unique_ptr<cpp_rpc::CPPClient> initializeSystem(const std::string& ip) {
    startTopic(ip);
    auto client = connectRpc(ip);
    initializeController(*client);
    return client;
}

}  // namespace smal_sys
