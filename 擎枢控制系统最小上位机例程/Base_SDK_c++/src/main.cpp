#include "smal_sys_app.hpp"

#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// 把关节角数组转换成控制器指令需要的 {j1,j2,...} 字符串格式。
std::string formatTarget(const std::vector<double>& values) {
    std::ostringstream out;
    out << "{";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << std::setprecision(10) << values[i];
    }
    out << "}";
    return out.str();
}

int main() {
    try {
        // 初始化系统：启动 topic、连接 RPC、下发初始化指令。
        const std::string robot_ip = "192.168.11.11";
        auto client = smal_sys::initializeSystem(robot_ip);

        // 启动后台线程，一直打印当前末端位姿和关节角度。
        std::thread printer([] {
            while (true) {
                auto end_pose = getCurrentRobottarget(smal_sys::kModelIndex);
                auto joint_angles = getCurrentJointtarget(smal_sys::kModelIndex);

                std::cout << "current robottarget: ";
                print_vector(end_pose);
                std::cout << std::endl;

                std::cout << "current jointtarget: ";
                print_vector(joint_angles);
                std::cout << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        // 主线程读取一次当前末端位姿和当前关节角度，用于后续业务逻辑。
        auto end_pose = getCurrentRobottarget(smal_sys::kModelIndex);
        auto current_joint = getCurrentJointtarget(smal_sys::kModelIndex);

        // 业务逻辑示例：控制器需要 10 维 jointtarget，把读取到的关节角填进去。
        std::vector<double> joint_angles(10, 0.0);
        for (size_t i = 0; i < current_joint.size() && i < joint_angles.size(); ++i) {
            joint_angles[i] = current_joint[i];
        }
        joint_angles[5] = 0.0;

        const auto target_value = formatTarget(joint_angles);
        const std::vector<std::string> cmds = {
            "{MoveAbsJ --jointtarget_value=" + target_value + "}",
        };
        send_rpcsy<RespDemo>(*client, cmds, 100, smal_sys::kRpcTimeoutMs);

        // 主线程保持运行，后台打印线程不会停止。
        printer.join();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
