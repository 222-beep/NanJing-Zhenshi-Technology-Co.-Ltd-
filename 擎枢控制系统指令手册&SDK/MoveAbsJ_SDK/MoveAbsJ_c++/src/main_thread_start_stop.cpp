#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <future>
#include <thread>
#include <chrono>

// ==================================================================
//  main  ——  使用示例（运动指令同步发送，Start / Stop 用独立线程）
//  流程结构：
//    1. 定义初始化指令（不含 Start）
//    2. 连接机器人控制器
//    3. 同步发送初始化指令
//    4. 在独立线程中发送 Start（send_rpc_thread，见 rpc_client.h）
//    5. 同步发送运动指令（send_rpcsy，在主线程，不使用独立线程）
//    6. 在独立线程中发送 Stop 停止运动
//
//  说明：Start 与 Stop 都通过独立线程发送；Start 发完先 .get() 等其返回，
//  保证 Start 先于运动指令到达。主线程用 send_rpcsy 同步发送运动指令时会
//  被阻塞（直到运动执行完或被打断），因此打断用的 Stop 必须放到独立线程里发。
// ==================================================================

int main() {
    const std::string robot_ip = "192.168.11.11";

    // ---- 命令定义 --------------------------------------------------

    // 初始化指令（同步发送）：不含 Start，Start 移到独立线程发送
    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Recover}",
        "{Mode}",
        "{Enable}",
        "{Var --clear}",
    //    "{Var --type=jointtarget --name=（变量名）
    //                  --value={ jointtarget 共 10 位，不足补 0，单位：弧度 }}"
        "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}"
    };

    // 运动指令（主线程同步发送，不用独立线程）：MoveAbsJ j1 -> j2 -> j0
    std::vector<std::string> motion_cmds = {
    //    "{MoveAbsJ --jointtarget_var=（关节目标变量名，需先在 init_cmds 中通过 Var 预定义）}"
        "{MoveAbsJ --jointtarget_var=j1}",
        "{MoveAbsJ --jointtarget_var=j2}",
        "{MoveAbsJ --jointtarget_var=j0}"
    };

    // 独立线程发送的启动指令
    const std::string start_cmd = "{Start}";

    // 独立线程发送的停止指令
    const std::string stop_cmd = "{Stop}";

    // ---- 连接机器人控制器 -------------------------------------------

    std::cout << "Connecting: " << std::endl;
    cpp_rpc::CPPClient client(robot_ip, 5868);
    if (!client.IsConnected()) {
        std::cerr << "Connection failed! Aborting all commands." << std::endl;
        return -1;
    }
    std::cout << "Connected: " << std::endl;

    // ==================================================================
    //  示例 1：通用同步 RPC 发送初始化指令
    //  返回值只有 return_code / subcmd_index / return_message
    // ==================================================================
    //  send_rpcsy<RespDemo>(client, cmds, 间隔ms, 超时ms)

    send_rpcsy<RespDemo>(client, init_cmds, 100, 50000);

    // ==================================================================
    //  示例 2：Start / Stop 独立线程发送，运动指令主线程同步发送
    //
    //  Start 在独立线程中发送，发完先 .get() 等其返回，保证 Start 先于
    //  运动指令到达。主线程 send_rpcsy(motion_cmds) 会阻塞直到运动跑完/被
    //  打断，所以把 Stop 安排到独立线程（send_rpc_thread，见 rpc_client.h）：
    //  线程内先延时 1s 等机器人动起来，再从独立线程发 Stop 打断运动。
    // ==================================================================

    // 在独立线程中发送 Start，并等待其返回，保证 Start 先于运动指令到达
    auto start_future = send_rpc_thread(client, start_cmd, 10000, true);
    std::cout << "Start sent: " << (start_future.get() ? "ok" : "failed") << std::endl;

    // 独立线程：延时 1s 后发送 Stop，用于打断下面阻塞中的运动
    auto stop_future = std::async(std::launch::async, [&client, stop_cmd]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return send_rpc_thread(client, stop_cmd, 10000, true).get();
    });

    // 主线程同步发送运动指令（阻塞，期间会被上面的 Stop 打断）
    //  send_rpcsy<RespDemo>(client, cmds, 间隔ms, 超时ms)
    send_rpcsy<RespDemo>(client, motion_cmds, 200, 50000);

    // 等待独立线程的 Stop 发送结果
    std::cout << "Stop sent: " << (stop_future.get() ? "ok" : "failed") << std::endl;

    return 0;
}
