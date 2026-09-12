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
        "{Var --clear}",
        "{Start}"
    };

    std::vector<std::string> jogc_cmd = {
    //    "{JogC --motion_type=（运动方式：0-x平动、1-y平动、2-z平动、3-x旋转、4-y旋转、5-z旋转）
    //            --direction=（方向：1-正方向，-1-负方向）
    //            --step=（步长）
    //            --coordinate=（坐标系：0-绝对世界坐标系，1-工具坐标系）
    //            --speed=（速度档位：v1、v5、v10、v25、v50、v100 ...）}"
        "{JogC --motion_type=0 --direction=1 --step=0.1 --coordinate=0 --speed=v100}",
        "{JogC --motion_type=0 --direction=-1 --step=0.1 --coordinate=0 --speed=v100}"
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
    // send_rpcsy<RespDemo>(client, jogc_cmd, 100, 50000);
    
    // ==================================================================
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    //  for 循环持续发送 JogC 保持点动
    // ==================================================================
    //  send_rpcAsy(client, jogc_cmd, 间隔ms, 超时ms)

    //持续发送 10 条指令
    for(int i = 0; i < 10; ++i)
    {
        send_rpcAsy(client, jogc_cmd, 0, 10000);
        delay_ms(5000);
    }

    return 0;
}
