#include "robot.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

const std::string robot_ip = "192.168.2.199";

// 可选模式：
// "free_drag"      ：6 个方向都可拖动，零力拖动
// "z_force"        ：只开启 Z 方向恒力
// "hybrid_move_z"  ：直线运动 + Z 方向力控，力位混合
const std::string DragMode = "free_drag";

std::atomic<bool> g_run{true};

void on_sigint(int) {
    g_run = false;
    std::cerr << "\nCtrl+C detected. Send Stop...\n";
}

} // namespace

int main() {
    std::signal(SIGINT, on_sigint);

    const std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Mode}",
        "{SetMaxToq}",
        "{Recover}",
        "{SetRate}",
        "{Enable}",
        "{Var --clear}",
        "{Recover}",
    };

    const std::vector<std::string> stop_cmds = {
        "{Stop}",
    };

    const std::vector<std::string> free_drag_cmds = {
        "{ForcePositionHybridControl "
        "--force_direction={1,1,1,1,1,1} "
        "--force_target={0,0,0,0,0,0} "
        "--damping_gain={0.001,0.001,0.001,0.1,0.1,0.1}}",
    };

    const std::vector<std::string> z_force_cmds = {
        "{ForcePositionHybridControl "
        "--force_direction={0,0,1,0,0,0} "
        "--force_target={0,0,3,0,0,0} "
        "--damping_gain={0.001,0.001,0.001,0.1,0.1,0.1} "
        "--coordinate=0}",
    };

    // 注意：p1 要改成你现场安全、可达的位置
    const std::vector<std::string> hybrid_move_z_cmds = {
        "{Var --type=robottarget --name=p1 --value={0.490,0.100,0.680,0,0.70710678,0,0.70710678} }",
        "{MoveBlend --type=first_insert}",
        "{MoveBlend --type=insert_line --robottarget_var=p1}",
        "{ForcePositionHybridControl "
        "--force_direction={0,0,1,0,0,0} "
        "--force_target={0,0,3,0,0,0} "
        "--damping_gain={0.001,0.001,0.001,0.1,0.1,0.1} "
        "--open_tg=1}",
    };

    const std::unordered_map<std::string, const std::vector<std::string>*> drag_cmd_map = {
        {"free_drag", &free_drag_cmds},
        {"z_force", &z_force_cmds},
        {"hybrid_move_z", &hybrid_move_z_cmds},
    };

    auto it = drag_cmd_map.find(DragMode);
    if (it == drag_cmd_map.end()) {
        throw std::runtime_error("未知 DRAG_MODE: " + DragMode);
    }
    const std::vector<std::string>& drag_cmds = *it->second;

    auto client = robot::create_client(robot_ip);

    // Python: send_rpcsy(..., 500, 0.1) -> 此处 sleep 与 py 一致用 100ms
    robot::send_rpcsy(*client, init_cmds, 500, 100);

    try {
        // ForcePositionHybridControl 是持续型控制，不要在循环里一直重复发
        // Python: send_rpc_async(..., 86400000, 0.1)
        robot::send_rpc_async(*client, drag_cmds, 86400000, 100);

        while (g_run) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    } catch (const std::exception& exc) {
        std::cerr << "Error: " << exc.what() << "\n";
    }

    try {
        robot::send_rpcsy(*client, stop_cmds, 1000, 100);
        std::cout << "Stop command sent.\n";
    } catch (const std::exception& exc) {
        std::cerr << "Failed to send Stop: " << exc.what() << "\n";
    }

    return 0;
}
