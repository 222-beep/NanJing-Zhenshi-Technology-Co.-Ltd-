#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <future>

// ==================================================================
//  main  ——  使用示例
// ==================================================================

int main() {
    const std::string robot_ip = "192.168.11.11";

    // ---- 命令定义 --------------------------------------------------

    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Recover}",
        "{Mode}",
        "{Enable}",
        "{Start}"
    };

    std::vector<std::string> readsdo_cmd = {
    //    "{ReadSdo --slave_id=（从站编号，从 0 开始，0 通常代表网络中第一个从站）
    //               --index=（对象字典主索引，十六进制）
    //               --sub_index=（对象字典子索引，十六进制）
    //               --size=（读取字节长度）
    //               --loop=（循环读取次数）}"
        "{ReadSdo --slave_id=5 --index=0x6064 --sub_index=0x00 --size=4 --loop=1}"
    };

    // ---- 连接机器人控制器 -------------------------------------------

    std::cout << "Connecting: " << std::endl;
    cpp_rpc::CPPClient client(robot_ip, 5868);
    if (!client.IsConnected()) {
        std::cerr << "Connection failed! Aborting all commands." << std::endl;
        return -1;
    }
    std::cout << "Connected: " << std::endl;

    // ==================================================================
    //  示例 1：通用同步 RPC（最常见用法）
    //  返回值只有 return_code / subcmd_index / return_message
    //  执行初始化
    // ==================================================================
    //  send_rpcsy<RespDemo>(client, init_cmds, 间隔ms)

    send_rpcsy<RespDemo>(client, init_cmds, 100);

    // ==================================================================
    //  示例 2：扩展返回值同步 RPC
    //  ReadSdo 使用专用响应类型 RespSdo，额外返回 sdo_value 字段
    //  send_rpcsy 返回 std::vector<RespSdo>，直接从返回值读取 sdo_value
    // ==================================================================
    //  send_rpcsy<RespSdo>(client, readsdo_cmd, 间隔ms)

    auto sdo_results = send_rpcsy<RespSdo>(client, readsdo_cmd, 100);
    for (const auto& r : sdo_results) {
        std::cout << "subcmd_index:" << r.subcmd_index << std::endl;
        std::cout << "return_code:" << r.return_code << std::endl;
        std::cout << "return_message:" << r.return_message << std::endl;
        if (r.has_sdo_value) {
            std::cout << "sdo_value:" << r.sdo_value
                      << " (0x" << std::hex << r.sdo_value << std::dec << ")" << std::endl;
        }
    }

    return 0;
}
