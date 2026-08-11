#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "rpc_client.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <limits>

using namespace std;

// 笛卡尔目标点维数：x,y,z,q1,q2,q3,q4
const int NUM_CARTESIAN = 7;

// 字符串分割
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// 轨迹点位：笛卡尔位姿 + 速度
struct TrajectoryPoint {
    string robottarget_value;
    double speed;
};

// 拼接 JogAnyC 指令
string build_jog_cmd(const string& robottarget_value, double speed) {
    return "{JogAnyC --robottarget_value=" + robottarget_value +
        " --cartesian_vel={" + std::to_string(speed) + "}" +
        " --cartesian_acc={1.0} --cartesian_dec={1.0}}";
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    const std::string robot_ip = "192.168.11.11";

    string input;

    // 初始化命令
    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Enable}",
    };

    // JogAnyC 初始动作
    std::vector<std::string> jog_start_cmds = {
        "{JogAnyC --robottarget_value={0.6,0.1,0.64,-0.5,0.5,-0.5,0.5} --cartesian_vel={1.0} --cartesian_acc={1.0} --cartesian_dec={1.0}}"
    };

    // 停止动作
    std::vector<std::string> jog_stop_cmds = {
        "{Stop --last_count=10}"
    };

    // 轨迹点位列表（add 添加，run 按序执行）
    vector<TrajectoryPoint> trajectory_points;

    // 连接
    cpp_rpc::CPPClient client(robot_ip, 5868);

    // 初始化
    send_rpcsy<RespDemo>(client, init_cmds, 100, 5000);

    // 主循环
    while (true) {
        std::cout << "\n可用命令:\n";
        std::cout << "start  - 启动 JogAnyC 控制\n";
        std::cout << "stop   - 停止运动\n";
        std::cout << "custom - 输入自定义笛卡尔目标点(立即执行)\n";
        std::cout << "add    - 添加点位到轨迹列表\n";
        std::cout << "show   - 显示轨迹点位列表\n";
        std::cout << "clear  - 清空轨迹点位列表\n";
        std::cout << "run    - 按序执行轨迹点位列表\n";
        std::cout << "exit   - 退出程序\n";
        std::cout << "请输入命令: ";

        std::cin >> input;
        std::transform(input.begin(), input.end(), input.begin(), ::tolower);

        if (input == "start") {
            std::cout << "启动 JogAnyC 控制...\n";
            send_rpcsy<RespDemo>(client, jog_start_cmds, 1000, 5000);
            std::cout << "机器人已执行初始动作\n";
        }
        else if (input == "stop") {
            send_rpcsy<RespDemo>(client, jog_stop_cmds, 1000, 5000);
            std::cout << "运动已停止\n";
        }
        else if (input == "custom") {
            try {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                std::cout << "请输入" << NUM_CARTESIAN << "个笛卡尔位姿值(x,y,z,q1,q2,q3,q4，x,y,z单位:米)，用逗号分隔: ";
                std::string cartesian_input;
                std::getline(std::cin, cartesian_input);

                vector<string> cartesian_str = split(cartesian_input, ',');
                if (cartesian_str.size() != NUM_CARTESIAN) {
                    std::cout << "错误: 需要输入" << NUM_CARTESIAN << "个笛卡尔位姿值!\n";
                    continue;
                }

                std::string robottarget_value = "{";
                for (size_t i = 0; i < cartesian_str.size(); ++i) {
                    robottarget_value += cartesian_str[i];
                    if (i < cartesian_str.size() - 1) {
                        robottarget_value += ",";
                    }
                }
                robottarget_value += "}";

                std::cout << "请输入运动速度(默认1.0): ";
                std::string speed_input;
                std::getline(std::cin, speed_input);

                double speed = 1.0;
                if (!speed_input.empty()) {
                    speed = stod(speed_input);
                }

                std::string custom_cmd = build_jog_cmd(robottarget_value, speed);

                std::cout << "执行指令: " << custom_cmd << std::endl;

                std::vector<std::string> custom_cmds = { custom_cmd };
                send_rpcsy<RespDemo>(client, custom_cmds, 1000, 5000);
            }
            catch (const std::exception& e) {
                std::cout << "输入格式错误，请确保输入的是数字\n";
                std::cout << "错误信息: " << e.what() << std::endl;
            }
        }
        else if (input == "add") {
            try {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                std::cout << "请输入" << NUM_CARTESIAN << "个笛卡尔位姿值(x,y,z,q1,q2,q3,q4，x,y,z单位:米)，用逗号分隔: ";
                std::string cartesian_input;
                std::getline(std::cin, cartesian_input);

                vector<string> cartesian_str = split(cartesian_input, ',');
                if (cartesian_str.size() != NUM_CARTESIAN) {
                    std::cout << "错误: 需要输入" << NUM_CARTESIAN << "个笛卡尔位姿值!\n";
                    continue;
                }

                std::string robottarget_value = "{";
                for (size_t i = 0; i < cartesian_str.size(); ++i) {
                    robottarget_value += cartesian_str[i];
                    if (i < cartesian_str.size() - 1) {
                        robottarget_value += ",";
                    }
                }
                robottarget_value += "}";

                std::cout << "请输入运动速度(默认1.0): ";
                std::string speed_input;
                std::getline(std::cin, speed_input);

                double speed = 1.0;
                if (!speed_input.empty()) {
                    speed = stod(speed_input);
                }

                trajectory_points.push_back({ robottarget_value, speed });
                std::cout << "点位 " << trajectory_points.size() << " 已添加，当前共 "
                          << trajectory_points.size() << " 个点位\n";
            }
            catch (const std::exception& e) {
                std::cout << "输入格式错误，请确保输入的是数字\n";
                std::cout << "错误信息: " << e.what() << std::endl;
            }
        }
        else if (input == "show") {
            std::cout << "当前轨迹点位数量: " << trajectory_points.size() << "\n";
            for (size_t i = 0; i < trajectory_points.size(); ++i) {
                std::cout << "  " << i + 1 << ". " << trajectory_points[i].robottarget_value
                          << ", speed=" << trajectory_points[i].speed << "\n";
            }
        }
        else if (input == "clear") {
            trajectory_points.clear();
            std::cout << "轨迹点位列表已清空\n";
        }
        else if (input == "run") {
            if (trajectory_points.empty()) {
                std::cout << "错误: 没有轨迹点可执行! 请先用 add 添加点位\n";
                continue;
            }

            std::cout << "开始执行轨迹，共 " << trajectory_points.size() << " 个点位...\n";
            for (size_t i = 0; i < trajectory_points.size(); ++i) {
                const auto& point = trajectory_points[i];
                std::string cmd = build_jog_cmd(point.robottarget_value, point.speed);
                std::cout << "[" << i + 1 << "/" << trajectory_points.size() << "] " << cmd << std::endl;
                std::vector<std::string> cmds = { cmd };
                // 同步发送，等待到达当前点后再发下一个点
                send_rpcsy<RespDemo>(client, cmds, 500, 30000);
            }
            std::cout << "轨迹执行完成!\n";
        }
        else if (input == "exit") {
            std::cout << "退出程序...\n";
            send_rpcsy<RespDemo>(client, jog_stop_cmds, 1000, 5000);
            break;
        }
        else {
            std::cout << "未知命令，请重新输入!\n";
        }
    }

    return 0;
}
