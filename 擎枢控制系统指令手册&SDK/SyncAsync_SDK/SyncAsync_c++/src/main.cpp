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
        "{SetUsingSP --state=on}",
        "{Start}"
    };

    // 同步示例指令：MoveAbsJ 依次到 3 个关节目标点
    std::vector<std::string> sync_cmd = {
    //    "{MoveAbsJ --jointtarget_value={ 关节目标，共 10 位，不足补 0，单位：弧度 }}"
        "{MoveAbsJ --jointtarget_value={0,0,0,0,0,0,0,0,0,0}}",
        "{MoveAbsJ --jointtarget_value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
        "{MoveAbsJ --jointtarget_value={0.2,0,0,0,0,0,0,0,0,0}}"
    };

    // 异步示例指令：SpeedL 在线规划往返
    std::vector<std::string> async_cmd = {
    //    "{SpeedL --vel={ 笛卡尔速度 vx,vy,vz,wx,wy,wz }
    //              --last_count=（末尾保持周期数）}"
        "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=100}",
        "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=100}",
        "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=100}",
        "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=100}"
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
    //  执行初始化 + MoveAbsJ 同步运动序列
    // ==================================================================
    //  send_rpcsy<RespDemo>(client, cmds, 间隔ms, 超时ms)

    send_rpcsy<RespDemo>(client, init_cmds, 100, 50000);
    send_rpcsy<RespDemo>(client, sync_cmd, 100, 50000);

    // ==================================================================
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    //  SpeedL 在线规划往返
    // ==================================================================
    //  send_rpcAsy(client, cmds, 间隔ms, 超时ms)

    send_rpcAsy(client, async_cmd, 0, 10000);

    return 0;
}
