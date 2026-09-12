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

    std::vector<std::string> joganyc_cmd = {
    //    "{JogAnyC --robottarget_value={ 笛卡尔位姿 x,y,z,q1,q2,q3,q4，x/y/z 单位：米 }
    //               --cartesian_vel={ 笛卡尔速度 }
    //               --cartesian_acc={ 笛卡尔加速度 }
    //               --cartesian_dec={ 笛卡尔减速度 }}"
        "{JogAnyC --robottarget_value={0.6,0.1,0.64,-0.5,0.5,-0.5,0.5} --cartesian_vel={1.0} --cartesian_acc={1.0} --cartesian_dec={1.0}}",
        "{JogAnyC --robottarget_value={0.5,0.2,0.74,-0.5,0.5,-0.5,0.5} --cartesian_vel={1.0} --cartesian_acc={1.0} --cartesian_dec={1.0}}",
        "{JogAnyC --robottarget_value={0.4,0.3,0.64,-0.5,0.5,-0.5,0.5} --cartesian_vel={1.0} --cartesian_acc={1.0} --cartesian_dec={1.0}}"
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
    //  for 循环持续发送 JogAnyC 保持点动
    // ==================================================================
    //  send_rpcAsy(client, joganyc_cmd, 间隔ms, 超时ms)

    //持续发送 10 条指令
    for(int i = 0; i < 10; ++i)
    {
        send_rpcAsy(client, joganyc_cmd, 0, 10000);
        delay_ms(200);
    }

    return 0;
}
