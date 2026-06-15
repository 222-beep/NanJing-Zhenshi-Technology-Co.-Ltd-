#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "robot.hpp"
#include "shared_data.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>

const std::string robot_ip = "192.168.2.216";

std::atomic<bool> running(true);
std::mutex cout_mtx;


// ==================== 打印 vector<double> ====================
void print_vector(const std::vector<double>& vec) {
    std::cout << "[";

    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << std::fixed << std::setprecision(6) << vec[i];

        if (i + 1 < vec.size()) {
            std::cout << ", ";
        }
    }

    std::cout << "]";
}


// ==================== 线程 1：Topic 打印线程 ====================
void topic_thread_func() {
    start_subscriber(robot_ip);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    while (running) {
        auto rt = getSystemState().getRt();

        {
            std::lock_guard<std::mutex> lock(cout_mtx);

            if (!rt) {
                std::cout << "[Topic] No RT data yet." << std::endl;
            }
            else {
                std::cout << "\n========== Topic State ==========\n";

                // 1. 当前关节角度，单位 rad
                // 不再使用 model_info.joint_count / joint_start_idx
                // 直接打印 models_joints 里面所有关节的 position
                std::vector<double> joint_rad;

                for (size_t idx = 0; idx < rt->models_joints.size(); ++idx) {
                    const auto& joint = rt->models_joints[idx];
                    joint_rad.push_back(joint.position);
                }

                std::cout << "joint_rad  : ";
                print_vector(joint_rad);
                std::cout << std::endl;


                // 2. 当前末端位姿
                // 默认读取第 0 个模型的当前点
                if (!rt->models_current_points.empty()) {
                    const auto& cur = rt->models_current_points[0];

                    std::cout << "tcp_pose   : ";
                    print_vector(cur.robottarget);
                    std::cout << std::endl;
                }
                else {
                    std::cout << "tcp_pose   : no data" << std::endl;
                }


                // 3. 错误码和错误信息
                // 默认读取第 0 个模型的运行状态
                if (!rt->models_info.empty()) {
                    const auto& info = rt->models_info[0];

                    std::cout << "error_code : " << info.error_code << std::endl;
                    std::cout << "error_msg  : " << info.error_msg << std::endl;
                }
                else {
                    std::cout << "error_code : no data" << std::endl;
                    std::cout << "error_msg  : no data" << std::endl;
                }

                std::cout << "=================================\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


// ==================== 线程 2：MoveAbsJ 运动线程 ====================
void moveabsj_thread_func() {
    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Mode}",
        "{SetMaxToq}",
        "{Recover}",
        "{SetRate}",
        "{Enable}",
        "{Var --clear}",
        "{Recover}",
        "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}"
    };

    std::vector<std::string> motion_cmds = {
        "{MoveAbsJ --jointtarget_var=j0}",
        "{MoveAbsJ --jointtarget_var=j1}",
        "{MoveAbsJ --jointtarget_var=j2}"
    };

    auto client = robot::create_client(robot_ip);

    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "[RPC] Start initialization..." << std::endl;
    }

    robot::send_rpcsy(*client, init_cmds, 500, 100);

    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "[RPC] Initialization finished." << std::endl;
        std::cout << "[RPC] Start MoveAbsJ loop..." << std::endl;
    }

    while (running) {
        robot::send_rpcsy(*client, motion_cmds, 50000, 1000);
    }
}


// ==================== 主函数 ====================
int main() {
    std::cout << "Minimal SDK Demo Start" << std::endl;
    std::cout << "Robot IP: " << robot_ip << std::endl;

    std::thread topic_thread(topic_thread_func);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::thread moveabsj_thread(moveabsj_thread_func);

    topic_thread.detach();
    moveabsj_thread.detach();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}