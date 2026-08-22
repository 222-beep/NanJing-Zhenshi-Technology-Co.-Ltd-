#include "rpc_client.h"
#include <cstdio>
#include <future>
#include <iostream>
#include <vector>
#include <string>

// ==================================================================
//  main  ——  使用示例
//  MoveS 主流程同步发送；Stop / SetEgmOffset 通过 send_rpc_thread
//  在独立线程中发送（见 rpc_client.h），不阻塞主流程
// ==================================================================

int main() {
    const std::string robot_ip = "192.168.11.11";

    // ---- 命令定义 --------------------------------------------------

    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Enable}",
        "{Var --clear}",
        // 定义轨迹目标点变量（笛卡尔位姿 x,y,z,q1,q2,q3,q4，x,y,z 单位：米）
        "{Var --type=robottarget --name=p1 --value={0.32,-0.32,0.48,0,1,0,0}}",
        "{Var --type=robottarget --name=p2 --value={0.38,-0.26,0.52,0,1,0,0}}",
        "{Var --type=robottarget --name=p3 --value={0.44,-0.32,0.48,0,1,0,0}}",
        "{Var --type=robottarget --name=p4 --value={0.38,-0.38,0.44,0,1,0,0}}",
        "{Var --type=robottarget --name=p5 --value={0.32,-0.32,0.48,0,1,0,0}}",
        // 开启最优求解器（需在 MoveS 发送之前）
        "{SetUsingSP --state=on}"
    };

    // MoveS 轨迹命令（同步发送）：first_insert 设置起点 -> insert 添加轨迹点 -> start 执行
    std::vector<std::string> moves_cmds = {
        "{MoveS --type=first_insert}",
        "{MoveS --type=insert --robottarget_var=p1}",
        "{MoveS --type=insert --robottarget_var=p2}",
        "{MoveS --type=insert --robottarget_var=p3}",
        "{MoveS --type=insert --robottarget_var=p4}",
        "{MoveS --type=insert --robottarget_var=p5}",
        "{MoveS --type=start}"
    };

    // 独立线程发送的新指令
    const std::string stop_cmd = "{Stop}";
    const std::string setegmoffset_cmd =
        "{SetEgmOffset --pos_offset={0.01,0,0} --max_vel=0.005 --max_acc=0.3 --max_dec=0.3 --coordinate=0}";

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
    // ==================================================================
    // 可选参数: send_rpcsy<RespDemo>(client, cmds, 间隔ms, 超时ms)
    send_rpcsy<RespDemo>(client, init_cmds, 100, 500);
    send_rpcsy<RespDemo>(client, moves_cmds, 500, 10000);

    // ==================================================================
    //  示例 2：独立线程发送（send_rpc_thread，见 rpc_client.h）
    //  使用 std::async 在独立线程中发送，不阻塞当前线程，
    //  返回 std::future<bool>，可在之后任意时机 .get() 获取发送结果
    // ==================================================================
    // 在独立线程中发送 SetEgmOffset 进行在线偏移
    auto setegmoffset_future = send_rpc_thread(client, setegmoffset_cmd, 10000, true);

    // 两个独立线程发送之间加延时，保证 SetEgmOffset 先于 Stop 到达
    delay_ms(500);

    // 在独立线程中发送 Stop 停止运动
    auto stop_future = send_rpc_thread(client, stop_cmd, 10000, true);

    // 等待两个独立线程的发送结果
    std::cout << "SetEgmOffset sent: " << (setegmoffset_future.get() ? "ok" : "failed") << std::endl;
    std::cout << "Stop sent: " << (stop_future.get() ? "ok" : "failed") << std::endl;

    return 0;
}
