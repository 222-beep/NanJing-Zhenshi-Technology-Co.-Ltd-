// rpc_client.h
#pragma once


#include "resp_dto.h"
#include "util.hpp"

// 异步版本
void send_rpcAsy(cpp_rpc::CPPClient& client, const std::vector<std::string>& cmd_cmd, int outim_num, int wait_num);

// 同步版本，带额外等待参数（你可按需要重命名）
void send_rpcsy(cpp_rpc::CPPClient& client, const std::vector<std::string>& cmd_cmd, int outim_num, int sleep_num);
