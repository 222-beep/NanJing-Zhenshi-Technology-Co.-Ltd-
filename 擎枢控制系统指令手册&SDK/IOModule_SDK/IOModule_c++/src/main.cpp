#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
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
        "{Enable}",
        "{Start}"
    };

    std::vector<std::string> getdi_cmd = {
    //    "{GetDI --di_name=（数字输入端口名，如 DI0、DI1 ...）}"
        "{GetDI --di_name=DI0}"
    };

    std::vector<std::string> setdo_cmd = {
    //    "{SetDO --do_name=（数字输出端口名，如 DO0、DO1 ...）
    //             --do_value=（输出值：0-低电平，1-高电平）}"
        "{SetDO --do_name=DO2 --do_value=1}"
    };

    std::vector<std::string> dopulse_cmd = {
    //    "{DOPulse --do_name=（数字输出端口名）
    //               --pulse_active=（有效电平：0-低，1-高）
    //               --high_cycles=（高电平持续周期数）
    //               --low_cycles=（低电平持续周期数）}"
        "{DOPulse --do_name=DO1 --pulse_active=1 --high_cycles=10 --low_cycles=10}"
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
    //  示例 2：通用同步 RPC 依次执行 IO 操作
    //  GetDI 读取数字输入 -> SetDO 设置数字输出 -> DOPulse 输出脉冲
    // ==================================================================
    //  send_rpcsy<RespDemo>(client, cmds, 间隔ms, 超时ms)

    send_rpcsy<RespDemo>(client, getdi_cmd, 100, 50000);
    send_rpcsy<RespDemo>(client, setdo_cmd, 100, 50000);
    send_rpcsy<RespDemo>(client, dopulse_cmd, 100, 50000);

    return 0;
}
