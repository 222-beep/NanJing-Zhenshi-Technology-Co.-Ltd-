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

    std::vector<std::string> readpdo_cmd = {
    //    "{ReadPdo --slave_id=（从站编号）
    //               --index=（PDO 对象索引，十六进制）
    //               --sub_index=（PDO 对象子索引，十六进制）
    //               --size=（读取数据位宽 bit）
    //               --interval=（读取间隔 s）
    //               --loop=（循环读取次数）}"
        "{ReadPdo --slave_id=6 --index=0x6041 --sub_index=0x00 --size=16 --interval=1 --loop=1}"
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
    //  send_rpcsy<RespDemo>(client, init_cmds, 间隔ms, 超时ms)

    send_rpcsy<RespDemo>(client, init_cmds, 100, 50000);

    // ==================================================================
    //  示例 2：扩展返回值同步 RPC
    //  ReadPdo 使用专用响应类型 RespPdo，额外返回 pdo_value 字段
    //  debug=true 时由头文件 RespPrinter<RespPdo> 自动打印 pdo_value
    // ==================================================================
    //  send_rpcsy<RespPdo>(client, readpdo_cmd, 间隔ms, 超时ms, debug=true)

    send_rpcsy<RespPdo>(client, readpdo_cmd, 100, 50000, true);

    return 0;
}
