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
        "{Var --clear}",
    //    "{Var --type=jointtarget --name=（变量名）
    //                  --value={ jointtarget 共 10 位，不足补 0，单位：弧度 }}"
        "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
        "{Start}"
    };

    std::vector<std::string> moveabsj_cmd = {
    //    "{MoveAbsJ --jointtarget_var=（关节目标变量名，需先在 init_cmds 中通过 Var 预定义）}"
        "{MoveAbsJ --jointtarget_var=j1}",
        "{MoveAbsJ --jointtarget_var=j2}",
        "{MoveAbsJ --jointtarget_var=j0}"
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
    //  for 循环持续发送 MoveAbsJ
    //  依次运动到 j1 -> j2 -> j0
    // ==================================================================
    //  send_rpcAsy(client, moveabsj_cmd, 间隔ms, 超时ms)

    //持续发送 10 组指令
    for(int i = 0; i < 10; ++i)
    {
        send_rpcAsy(client, moveabsj_cmd, 0, 10000);
        delay_ms(200);
    }

    return 0;
}
