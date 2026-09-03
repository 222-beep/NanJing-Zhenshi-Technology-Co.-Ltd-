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
        "{SetUsingSP --state=on}",
        "{Enable}",
        "{Start}"
    };

    // 切换到 CST 拖拽模式（拖拽前单独同步下发一次）
    std::vector<std::string> switch_cst_cmd = {
        "{SwitchToCST}"
    };

    std::vector<std::string> dragincst_cmd = {
    //    "{DragInCST --cf_coef={ 力控系数，共 7 位 }
    //                 --vf_coef={ 速度反馈系数，共 7 位 }
    //                 --vel_limit={ 各方向速度限制，共 7 位，单位 m/s 或 rad/s }
    //                 --ping_pong_amp=（乒乓幅度）
    //                 --zero_check=（零漂检测阈值）}"
        "{DragInCST --cf_coef={0,0,0,0,0,0,0} --vf_coef={0,0,0,0,0,0,0} --vel_limit={0.3,0.3,0.3,0.3,0.3,0.3,0.3} --ping_pong_amp=0 --zero_check=0.004}"
    };

    // 退出 CST 拖拽，恢复到 CSP 位置模式
    std::vector<std::string> exit_cmds = {
        "{Stop --last_count=10}",
        "{SwitchToCSP}",
        "{Recover}",
        "{Start}"
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

    // 切换到 CST 拖拽模式（同步下发一次）
    send_rpcsy<RespDemo>(client, switch_cst_cmd, 100, 50000);

    // ==================================================================
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    //  for 循环持续发送 DragInCST 保持拖拽状态
    // ==================================================================
    //  send_rpcAsy(client, dragincst_cmd, 间隔ms, 超时ms)

    //持续发送 10 组指令
    for(int i = 0; i < 10; ++i)
    {
        send_rpcAsy(client, dragincst_cmd, 0, 10000);
        delay_ms(200);
    }

    // ==================================================================
    //  退出 CST 拖拽模式，恢复到 CSP 位置模式
    // ==================================================================
    send_rpcsy<RespDemo>(client, exit_cmds, 100, 50000);

    return 0;
}
