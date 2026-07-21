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

// 左右臂轴数，可按实际机型修改
const int NUM_JOINTS_ARM1 = 7;
const int NUM_JOINTS_ARM2 = 7;

// 生成 {0,0,0,0,0,0,0} 这种字符串
std::string make_zero_joint_pos(int num_joints) {
    std::string result = "{";
    for (int i = 0; i < num_joints; ++i) {
        result += "0";
        if (i < num_joints - 1) {
            result += ",";
        }
    }
    result += "}";
    return result;
}

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

// 去掉字符串首尾空格
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

// 把输入的关节数组拼成 {a,b,c,d}
string make_joint_pos_string(const vector<string>& joints_str) {
    string joint_pos = "{";
    for (size_t i = 0; i < joints_str.size(); ++i) {
        joint_pos += trim(joints_str[i]);
        if (i < joints_str.size() - 1) {
            joint_pos += ",";
        }
    }
    joint_pos += "}";
    return joint_pos;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    const std::string robot_ip = "192.168.2.199";

    string input;

    // 初始化命令
    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Enable}",
    };

    // 左右臂初始 joint_pos
    std::string zero_joint_pos_arm1 = make_zero_joint_pos(NUM_JOINTS_ARM1);
    std::string zero_joint_pos_arm2 = make_zero_joint_pos(NUM_JOINTS_ARM2);

    // 双臂 JogAnyJ 初始动作
    std::vector<std::string> jog_start_cmds = {
        "{JogAnyJ --joint_pos=" + zero_joint_pos_arm1 + " --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5"
        "||JogAnyJ --joint_pos=" + zero_joint_pos_arm2 + " --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5}"
    };

    // 双臂停止动作
    std::vector<std::string> jog_stop_cmds = {
        "{Stop --last_count=10||Stop --last_count=10}"
    };

    // 连接
    cpp_rpc::CPPClient client(robot_ip, 5868);

    // 初始化
    send_rpcsy<RespDemo>(client, init_cmds, 100, 5000);

    // 主循环
    while (true) {
        std::cout << "\n可用命令:\n";
        std::cout << "start   - 启动双臂 JogAnyJ 控制\n";
        std::cout << "stop    - 停止双臂运动\n";
        std::cout << "custom  - 输入双臂自定义关节位置\n";
        std::cout << "exit    - 退出程序\n";
        std::cout << "请输入命令: ";

        std::cin >> input;
        std::transform(input.begin(), input.end(), input.begin(), ::tolower);

        if (input == "start") {
            std::cout << "启动双臂 JogAnyJ 控制...\n";
            send_rpcsy<RespDemo>(client, jog_start_cmds, 1000, 5000);
            std::cout << "双臂已执行初始动作\n";
        }
        else if (input == "stop") {
            send_rpcsy<RespDemo>(client, jog_stop_cmds, 1000, 5000);
            std::cout << "双臂运动已停止\n";
        }
        else if (input == "custom") {
            try {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                // 左臂输入
                std::cout << "请输入左臂 " << NUM_JOINTS_ARM1
                          << " 个关节角度(弧度)，用逗号分隔: ";
                std::string joint_input_arm1;
                std::getline(std::cin, joint_input_arm1);

                vector<string> joints_str_arm1 = split(joint_input_arm1, ',');
                if (joints_str_arm1.size() != NUM_JOINTS_ARM1) {
                    std::cout << "错误: 左臂需要输入 " << NUM_JOINTS_ARM1 << " 个关节角度!\n";
                    continue;
                }

                // 右臂输入
                std::cout << "请输入右臂 " << NUM_JOINTS_ARM2
                          << " 个关节角度(弧度)，用逗号分隔: ";
                std::string joint_input_arm2;
                std::getline(std::cin, joint_input_arm2);

                vector<string> joints_str_arm2 = split(joint_input_arm2, ',');
                if (joints_str_arm2.size() != NUM_JOINTS_ARM2) {
                    std::cout << "错误: 右臂需要输入 " << NUM_JOINTS_ARM2 << " 个关节角度!\n";
                    continue;
                }

                std::string joint_pos_arm1 = make_joint_pos_string(joints_str_arm1);
                std::string joint_pos_arm2 = make_joint_pos_string(joints_str_arm2);

                std::cout << "请输入运动速度(默认0.1): ";
                std::string speed_input;
                std::getline(std::cin, speed_input);

                double speed = 0.1;
                if (!speed_input.empty()) {
                    speed = stod(speed_input);
                }

                std::string custom_cmd =
                    "{JogAnyJ --joint_pos=" + joint_pos_arm1 +
                    " --joint_vel=" + std::to_string(speed) +
                    " --joint_acc=0.5 --joint_dec=0.5"
                    "||JogAnyJ --joint_pos=" + joint_pos_arm2 +
                    " --joint_vel=" + std::to_string(speed) +
                    " --joint_acc=0.5 --joint_dec=0.5}";

                std::cout << "执行指令: " << custom_cmd << std::endl;

                std::vector<std::string> custom_cmds = { custom_cmd };
                send_rpcsy<RespDemo>(client, custom_cmds, 1000, 5000);
            }
            catch (const std::exception& e) {
                std::cout << "输入格式错误，请确保输入的是数字\n";
                std::cout << "错误信息: " << e.what() << std::endl;
            }
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