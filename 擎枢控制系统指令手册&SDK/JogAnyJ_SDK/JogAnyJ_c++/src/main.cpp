#include "robot.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <limits>

using namespace std;

// 机器人关节轴数，按实际机型修改，例如 6、7
const int NUM_JOINTS = 7;

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

int main() {
    const std::string robot_ip = "192.168.2.199";

    string input;

    // 初始化命令
    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Enable}",
    };

    // 根据轴数自动生成初始 joint_pos
    std::string zero_joint_pos = make_zero_joint_pos(NUM_JOINTS);

    // JogAnyJ 初始动作
    std::vector<std::string> jog_start_cmds = {
        "{JogAnyJ --joint_pos=" + zero_joint_pos + " --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5}"
    };

    // 停止动作
    std::vector<std::string> jog_stop_cmds = {
        "{Stop --last_count=10}"
    };

    // 连接
    auto client = robot::create_client(robot_ip);

    // 初始化
    robot::send_rpcsy(*client, init_cmds, 5000, 100);

    // 主循环
    while (true) {
        std::cout << "\n可用命令:\n";
        std::cout << "start  - 启动 JogAnyJ 控制\n";
        std::cout << "stop   - 停止运动\n";
        std::cout << "custom - 输入自定义关节位置\n";
        std::cout << "exit   - 退出程序\n";
        std::cout << "请输入命令: ";

        std::cin >> input;
        std::transform(input.begin(), input.end(), input.begin(), ::tolower);

        if (input == "start") {
            std::cout << "启动 JogAnyJ 控制...\n";
            robot::send_rpcsy(*client, jog_start_cmds, 5000, 1000);
            std::cout << "机器人已执行初始动作\n";
        }
        else if (input == "stop") {
            robot::send_rpcsy(*client, jog_stop_cmds, 5000, 1000);
            std::cout << "运动已停止\n";
        }
        else if (input == "custom") {
            try {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                std::cout << "请输入" << NUM_JOINTS << "个关节角度(弧度)，用逗号分隔: ";
                std::string joint_input;
                std::getline(std::cin, joint_input);

                vector<string> joints_str = split(joint_input, ',');
                if (joints_str.size() != NUM_JOINTS) {
                    std::cout << "错误: 需要输入" << NUM_JOINTS << "个关节角度!\n";
                    continue;
                }

                std::string joint_pos = "{";
                for (size_t i = 0; i < joints_str.size(); ++i) {
                    joint_pos += joints_str[i];
                    if (i < joints_str.size() - 1) {
                        joint_pos += ",";
                    }
                }
                joint_pos += "}";

                std::cout << "请输入运动速度(默认0.1): ";
                std::string speed_input;
                std::getline(std::cin, speed_input);

                double speed = 0.1;
                if (!speed_input.empty()) {
                    speed = stod(speed_input);
                }

                std::string custom_cmd =
                    "{JogAnyJ --joint_pos=" + joint_pos +
                    " --joint_vel=" + std::to_string(speed) +
                    " --joint_acc=0.5 --joint_dec=0.5}";

                std::cout << "执行指令: " << custom_cmd << std::endl;

                std::vector<std::string> custom_cmds = { custom_cmd };
                robot::send_rpcsy(*client, custom_cmds, 5000, 1000);
            }
            catch (const std::exception& e) {
                std::cout << "输入格式错误，请确保输入的是数字\n";
                std::cout << "错误信息: " << e.what() << std::endl;
            }
        }
        else if (input == "exit") {
            std::cout << "退出程序...\n";
            robot::send_rpcsy(*client, jog_stop_cmds, 5000, 1000);
            break;
        }
        else {
            std::cout << "未知命令，请重新输入!\n";
        }
    }

    return 0;
}