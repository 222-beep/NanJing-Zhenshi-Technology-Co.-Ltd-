#include "rpc_client.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

// Ctrl+C 退出标志：信号回调只置标志，主线程循环检测到后退出
std::atomic<bool> g_run{true};

// SIGINT 回调：不在信号上下文里发送 RPC，退出指令统一由主线程发送
void on_sigint(int) {
    g_run = false;
    std::cerr << "\nCtrl+C detected. Exit drag...\n";
}

} // namespace

// ==================================================================
//  main  ——  使用示例
// ==================================================================

int main() {
    // 注册 Ctrl+C 处理；与力控例程一致，由主线程负责发送退出指令
    std::signal(SIGINT, on_sigint);

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

    // 切换到 CST 拖拽模式（先清除运动记录，再设置全部轴及最大力矩）
    std::vector<std::string> switch_cst_cmd = {
        "{GetDefaultVfCf}",
        "{DeleteMotionRecord}",
        "{SwitchToCST --all --max_torque=1000}"
    };

    std::vector<std::string> dragincst_cmd = {
        "{DragInCST --cbfSwitch={0} --cutoff_freq=20.0 --gravity_cof={1.0} --ping_pong=100 --ping_pong_amp=0.5 --vel_limit={0.1} --zero_check={0.0001}}"
    };

    // 退出 CST 拖拽，恢复到 CSP 位置模式
    // 顺序：Stop 停止拖拽 → Sleep 等待停止生效 → SwitchToCSP 切回位置模式
    //       → Recover 同步模型与实际状态 → Start 恢复运动控制
    std::vector<std::string> exit_cmds = {
        "{Stop --last_count=10}",
        "{Sleep --count=20}",
        "{SwitchToCSP --all --max_torque=1000}",
        "{Recover}",
        "{Start --last_count=50}"
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
    //  send_rpcsy<RespDemo>(client, init_cmds, 间隔ms)

    send_rpcsy<RespDemo>(client, init_cmds, 100);

    // 切换到 CST 拖拽模式（同步下发一次）
    send_rpcsy<RespDemo>(client, switch_cst_cmd, 100);

    // ==================================================================
    //  示例 2：通用同步 RPC
    //  DragInCST 只需同步发送一次，随后等待 Ctrl+C 退出拖拽
    // ==================================================================
    //  send_rpcsy<RespDemo>(client, dragincst_cmd, 间隔ms)

    try {
        // DragInCST 只需发送一次，控制器会持续保持拖拽状态
        send_rpcsy<RespDemo>(client, dragincst_cmd, 100);
        std::cout << "DragInCST started. Press Ctrl+C to exit.\n";

        // 保持客户端存活；Ctrl+C 将 g_run 置 false 后退出循环
        // 不要在循环里一直重复发 DragInCST
        while (g_run) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    } catch (const std::exception& exc) {
        // 发送或等待异常时也继续走下面的退出流程，避免拖拽模式残留
        std::cerr << "Error: " << exc.what() << "\n";
    }

    // ==================================================================
    //  Ctrl+C 后退出 CST 拖拽模式，恢复到 CSP 位置模式
    // ==================================================================
    try {
        // 正常 Ctrl+C 或上面异常后，都尝试发送退出序列
        send_rpcsy<RespDemo>(client, exit_cmds, 100);
        std::cout << "Exit commands sent.\n";
    } catch (const std::exception& exc) {
        std::cerr << "Failed to send exit commands: " << exc.what() << "\n";
    }

    return 0;
}
