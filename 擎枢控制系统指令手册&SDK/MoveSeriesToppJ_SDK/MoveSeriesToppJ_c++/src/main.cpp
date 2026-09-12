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
        "{Mode}",
        "{Enable}",
        "{Start}"
    };

    // MoveSeriesToppJ 关节轨迹命令：first_insert 设置起点 -> insert 添加轨迹点 -> start 执行
    std::vector<std::string> trajectory_cmd = {
    //    "{MoveSeriesToppJ --type=first_insert}  设置轨迹起点
    //     {MoveSeriesToppJ --type=insert --jointtarget_value={ 关节目标，共 10 位，不足补 0，单位：弧度 }}
    //     {MoveSeriesToppJ --type=start --vel_coef=（速度系数 0~1） --acc_coef=（加速度系数 0~1）}"
        "{MoveSeriesToppJ --type=first_insert}",
        "{MoveSeriesToppJ --type=insert --jointtarget_value={0.1,-0.5,0.3,0,0,0,0,0,0,0}}",
        "{MoveSeriesToppJ --type=insert --jointtarget_value={0.2,0,0.5,-0.2,0,0,0,0,0,0}}",
        "{MoveSeriesToppJ --type=insert --jointtarget_value={0,0,0,0,0,0,0,0,0,0}}",
        "{MoveSeriesToppJ --type=start --vel_coef=0.95 --acc_coef=0.95}"
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
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    //  for 循环重复下发 MoveSeriesToppJ 轨迹序列
    // ==================================================================
    //  send_rpcAsy(client, trajectory_cmd, 间隔ms, 超时ms)

    //持续发送 10 组指令
    for(int i = 0; i < 10; ++i)
    {
        send_rpcAsy(client, trajectory_cmd, 0, 10000);
        delay_ms(5000);
    }

    return 0;
}
