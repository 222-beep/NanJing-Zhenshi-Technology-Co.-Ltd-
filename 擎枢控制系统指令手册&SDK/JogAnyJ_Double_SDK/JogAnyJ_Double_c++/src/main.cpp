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
        "{Enable}",
        "{Start}"
    };

    // 双臂 JogAnyJ 指令使用 || 分隔左右臂
    std::vector<std::string> joganyj_cmd = {
    //    "{JogAnyJ --jointtarget_value={ 关节目标，共 10 位，不足补 0，单位：弧度 }
    //               --joint_vel=（关节速度） --joint_acc=（关节加速度） --joint_dec=（关节减速度）
    //               --last_count=（末尾保持周期数）
    //     ||JogAnyJ --...  （右臂指令，参数格式与左臂一致）}"
        "{JogAnyJ --jointtarget_value={0.1,-0.5,0.3,0,0,0,0,0,0,0} --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5 --last_count=100"
        "||JogAnyJ --jointtarget_value={-0.1,0.5,-0.3,0,0,0,0,0,0,0} --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5 --last_count=100}"
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
    //  for 循环持续发送双臂 JogAnyJ 保持点动
    // ==================================================================
    //  send_rpcAsy(client, joganyj_cmd, 间隔ms, 超时ms)

    //持续发送 10 条指令
    for(int i = 0; i < 10; ++i)
    {
        send_rpcAsy(client, joganyj_cmd, 0, 10000);
        delay_ms(200);
    }

    return 0;
}
