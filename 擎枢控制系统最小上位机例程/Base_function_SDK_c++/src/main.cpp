#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif

#include "command.hpp"
#include "system_state_reader.hpp"

#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// 把数组转换成控制器指令需要的 {v1,v2,...} 字符串格式。
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

// 打印当前末端位姿和当前关节角度。
void printStateLoop() {
    while (true) {
        auto end_pose = getCurrentRobottarget(0);
        auto joint_angles = getCurrentJointtarget(0);

        std::cout << "current robottarget: ";
        print_vector(end_pose);
        std::cout << std::endl;

        std::cout << "current jointtarget: ";
        print_vector(joint_angles);
        std::cout << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    try {
        // 修改 IP 
        const std::string robot_ip = "192.168.2.199";

        // 启动机器人状态订阅，用于读取当前末端位姿和关节角度。
        start_subscriber(robot_ip);

        // 创建机器人指令客户端，并下发初始化指令。
        Command cmd(robot_ip);
        cmd.Clear();
        cmd.Recover();
        cmd.Enable();

        // 启动后台线程，一直打印当前末端位姿和关节角度。
        std::thread printer(printStateLoop);

        // 等到机器人状态数据更新后，再读取当前关节角度。
        while (!hasRtData()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 读取当前末端位姿和当前关节角度。
        auto end_pose = getCurrentRobottarget(0);
        auto current_joint = getCurrentJointtarget(0);

        // 控制器需要 10 维 jointtarget，把当前关节角填进去，再把第六轴改为 0。
        std::vector<double> joint_angles(10, 0.0);
        for (size_t i = 0; i < current_joint.size() && i < joint_angles.size(); ++i) {
            joint_angles[i] = current_joint[i];
        }
        joint_angles[5] = 0.0;

        // MoveAbsJ 业务示例：直接使用 jointtarget_value，不定义命名点位。
        const auto target_value = formatTarget(joint_angles);
        cmd.sendSync({"{MoveAbsJ --jointtarget_value=" + target_value + "}"}, 60000);

        // 主线程保持运行，后台打印线程不会停止。
        printer.join();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
