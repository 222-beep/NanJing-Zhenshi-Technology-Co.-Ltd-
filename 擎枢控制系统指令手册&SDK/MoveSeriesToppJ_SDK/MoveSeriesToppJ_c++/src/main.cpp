// 注意：robot.hpp 必须先于 <windows.h> 包含，避免 Windows 头文件中的宏
// （如 min/max、close/open 等）污染 robot 库的内部实现。
#include "robot.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <iomanip>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
// 阻止 windows.h 定义 min/max 宏，避免与 std::min/std::max 冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#pragma comment(lib, "ws2_32.lib")
#define PATH_DEVNULL "NUL"
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#define PATH_DEVNULL "/dev/null"
#endif

using namespace std;

// 常量定义
const string DEFAULT_ROBOT_IP = "192.168.11.11";
const double PI = acos(-1.0);
const int REQUIRED_DIM = 10;
const int USER_INPUT_AXIS = 6;
const double MIN_SPEED_ACCEL = 0.1;
const double MAX_SPEED_ACCEL = 1.0;

// 初始化指令列表
vector<string> init_commands = {
    "{Clear}",
    "{Disable}",
    "{Enable}"
};

// 关节点结构体
struct JointPoint {
    string point_name;
    vector<double> angles; // 存储弧度值
};

// 校验IP地址格式是否合法
bool is_valid_ip(const string& ip) {
#ifdef _WIN32
    sockaddr_in sa;
    return InetPton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
#else
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
#endif
}

// 解析关节角度字符串（度->弧度）
vector<double> parse_joint_angles(const string& input_str) {
    vector<double> angles_deg;
    stringstream ss(input_str);
    string item;

    // 解析逗号分隔的角度值
    while (getline(ss, item, ',')) {
        // 去除前后空格
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);

        if (item.empty()) continue;

        try {
            double deg = stod(item);
            // 角度范围合理性检查（可根据实际机器人关节范围调整）
            if (deg < -360.0 || deg > 360.0) {
                throw invalid_argument("角度值超出合理范围");
            }
            angles_deg.push_back(deg);
        }
        catch (...) {
            throw invalid_argument("角度值格式错误: " + item);
        }
    }

    // 补全或截断到指定轴数
    while (angles_deg.size() < USER_INPUT_AXIS) {
        angles_deg.push_back(0.0);
    }
    if (angles_deg.size() > USER_INPUT_AXIS) {
        angles_deg.resize(USER_INPUT_AXIS);
    }

    // 转换为弧度
    vector<double> angles_rad;
    for (double deg : angles_deg) {
        angles_rad.push_back(deg * PI / 180.0);
    }

    // 补全到要求的维度
    while (angles_rad.size() < REQUIRED_DIM) {
        angles_rad.push_back(0.0);
    }

    return angles_rad;
}

// 构建关节角度指令字符串
string build_joint_string(const vector<double>& angles) {
    ostringstream oss;
    oss << "{";
    for (int i = 0; i < REQUIRED_DIM; ++i) {
        if (i > 0) oss << ",";
        oss << fixed << setprecision(5) << angles[i];
    }
    oss << "}";
    return oss.str();
}

// 显示关节角度（弧度->度）
void show_joint_angles(const vector<double>& angles) {
    cout << "[";
    for (int i = 0; i < USER_INPUT_AXIS; ++i) {
        if (i > 0) cout << ", ";
        cout << fixed << setprecision(1) << angles[i] * 180.0 / PI;
    }
    cout << "]";
}

// 测试机器人连接
bool connect_test(const string& ip) {
    cout << "\n当前机器人IP：" << ip << endl;
    cout << "正在连接中..." << endl;

#ifdef _WIN32
    // Windows平台ping命令
    string cmd = "ping -n 1 -w 1000 " + ip + " > " PATH_DEVNULL " 2>&1";
#else
    // Linux平台ping命令
    string cmd = "ping -c 1 -W 1 " + ip + " > " PATH_DEVNULL " 2>&1";
#endif

    int ret = system(cmd.c_str());
#ifdef _WIN32
    ret = (ret == 0) ? 0 : 1; // Windows system返回值处理
#else
    ret = WEXITSTATUS(ret);   // Linux获取子进程退出码
#endif

    if (ret == 0) {
        cout << "\033[32m机器人连接成功！\033[0m" << endl;
        return true;
    }
    else {
        cout << "\033[31m机器人连接失败！\033[0m" << endl;
        return false;
    }
}

// 全局变量 - 标准输出文件描述符
int stdout_fd = -1;

// 禁用标准输出
void disable_stdout() {
    if (stdout_fd != -1) return; // 防止重复调用

#ifdef _WIN32
    stdout_fd = _dup(_fileno(stdout));
    int devnull = _open(PATH_DEVNULL, _O_WRONLY);
    if (devnull != -1) {
        _dup2(devnull, _fileno(stdout));
        _close(devnull);
    }
#else
    stdout_fd = dup(STDOUT_FILENO);
    int devnull = open(PATH_DEVNULL, O_WRONLY);
    if (devnull != -1) {
        dup2(devnull, STDOUT_FILENO);
        close(devnull);
    }
#endif
}

// 启用标准输出
void enable_stdout() {
    if (stdout_fd == -1) return; // 防止重复调用

#ifdef _WIN32
    _dup2(stdout_fd, _fileno(stdout));
    _close(stdout_fd);
#else
    dup2(stdout_fd, STDOUT_FILENO);
    close(stdout_fd);
#endif
    stdout_fd = -1; // 重置标记
}

// 限制数值在指定范围内
double clamp(double value, double min_val, double max_val) {
    return max(min_val, min(value, max_val));
}

int main() {
#ifdef _WIN32
    // Windows初始化Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    cout << "=====================================================" << endl;
    cout << "          MoveSeriesToppJ 关节轨迹控制程序         " << endl;
    cout << "=====================================================" << endl;

    string robot_ip = DEFAULT_ROBOT_IP;
    bool connect_ok = false;

    // 机器人IP连接流程
    while (true) {
        if (is_valid_ip(robot_ip)) {
            connect_ok = connect_test(robot_ip);
            if (connect_ok) break;
        }
        else {
            cout << "\033[31m无效的IP地址格式！\033[0m" << endl;
        }

        cout << "请输入新的机器人IP地址：";
        while (true) {
            getline(cin, robot_ip);
            if (!robot_ip.empty() && is_valid_ip(robot_ip)) {
                break;
            }
            cout << "\033[31m请输入有效的IP地址！\033[0m" << endl;
        }
    }

    // 创建机器人客户端（SDK 返回 shared_ptr<cpp_rpc::CPPClient>）
    std::shared_ptr<cpp_rpc::CPPClient> robot_client;
    disable_stdout();
    try {
        robot_client = robot::create_client(robot_ip);
    }
    catch (...) {
        enable_stdout();
        cout << "\033[31m机器人客户端创建失败！\033[0m" << endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    enable_stdout();

    // 初始化机器人
    cout << "\n程序初始化中..." << endl;
    int total_cmds = init_commands.size();
    int success_count = 0;
    int fail_count = 0;

    for (const string& cmd : init_commands) {
        bool cmd_success = true;
        disable_stdout();
        try {
            robot::send_rpcsy(*robot_client, { cmd }, 500, 100);
        }
        catch (...) {
            cmd_success = false;
        }
        enable_stdout();

        if (cmd_success) {
            success_count++;
        }
        else {
            fail_count++;
        }
    }

    cout << "\n初始化完成：" << success_count << " 个成功，" << fail_count << " 个失败" << endl;
    if (fail_count > 0) {
        cout << "\033[33m警告：部分初始化指令执行失败，可能影响机器人正常运行！\033[0m" << endl;
    }
    else {
        cout << "\033[32m机器人初始化完成！\033[0m" << endl;
    }

    // 轨迹控制主流程
    vector<JointPoint> trajectory_list;
    string user_input;
    double speed = 0.95;
    double accel = 0.95;

    while (true) {
        // 显示菜单
        cout << "\n=====================================================" << endl;
        cout << "          MoveSeriesToppJ 关节轨迹控制" << endl;
        cout << "=====================================================" << endl;
        cout << "1）设置轨迹起点（当前位置）" << endl;
        cout << "2）添加关节轨迹点" << endl;
        cout << "3）设置速度系数 [" << MIN_SPEED_ACCEL << "~" << MAX_SPEED_ACCEL << "]" << endl;
        cout << "4）设置加速度系数 [" << MIN_SPEED_ACCEL << "~" << MAX_SPEED_ACCEL << "]" << endl;
        cout << "5）执行轨迹运动" << endl;
        cout << "6）清空所有轨迹点" << endl;
        cout << "7）查看已添加轨迹点" << endl;
        cout << "8）安全停止并退出" << endl;
        cout << "=====================================================" << endl;
        cout << "当前：速度：" << fixed << setprecision(2) << speed
            << "   加速度：" << fixed << setprecision(2) << accel << endl;
        cout << "选择（1-8）：";

        // 获取用户输入
        getline(cin, user_input);
        if (user_input.empty()) {
            cout << "\033[31m错误：请输入有效选项！\033[0m" << endl;
            continue;
        }

        // 处理菜单选项
        try {
            if (user_input == "1") {
                cout << "[操作] 设置轨迹起点..." << endl;
                disable_stdout();
                robot::send_rpcsy(*robot_client,
                    { "{MoveSeriesToppJ --type=first_insert}" }, 5000, 500);
                enable_stdout();
                trajectory_list.clear();
                cout << "[成功] 轨迹起点已设置！" << endl;
            }
            else if (user_input == "2") {
                cout << "[操作] 输入6个关节角（逗号分隔，单位：度）：";
                string angle_input;
                getline(cin, angle_input);

                vector<double> angles_rad = parse_joint_angles(angle_input);
                string joint_str = build_joint_string(angles_rad);

                // 发送添加轨迹点指令
                disable_stdout();
                string cmd = "{MoveSeriesToppJ --type=insert --jointtarget_value=" + joint_str + "}";
                robot::send_rpcsy(*robot_client, { cmd }, 5000, 500);
                enable_stdout();

                // 记录轨迹点
                string point_name = "j" + to_string(trajectory_list.size() + 1);
                trajectory_list.push_back({ point_name, angles_rad });

                cout << "[成功] 已添加：" << point_name << " ";
                show_joint_angles(angles_rad);
                cout << endl;
            }
            else if (user_input == "3") {
                cout << "[操作] 输入速度系数(" << MIN_SPEED_ACCEL << "~" << MAX_SPEED_ACCEL << ")：";
                string s;
                getline(cin, s);
                double new_speed = stod(s);

                new_speed = clamp(new_speed, MIN_SPEED_ACCEL, MAX_SPEED_ACCEL);
                speed = new_speed;

                cout << "[成功] 速度已设为：" << fixed << setprecision(2) << speed << endl;
            }
            else if (user_input == "4") {
                cout << "[操作] 输入加速度系数(" << MIN_SPEED_ACCEL << "~" << MAX_SPEED_ACCEL << ")：";
                string s;
                getline(cin, s);
                double new_accel = stod(s);

                new_accel = clamp(new_accel, MIN_SPEED_ACCEL, MAX_SPEED_ACCEL);
                accel = new_accel;

                cout << "[成功] 加速度已设为：" << fixed << setprecision(2) << accel << endl;
            }
            else if (user_input == "5") {
                if (trajectory_list.empty()) {
                    cout << "\033[31m[错误] 请先添加轨迹点！\033[0m" << endl;
                    continue;
                }

                cout << "[操作] 开始执行轨迹..." << endl;
                ostringstream cmd_ss;
                cmd_ss << "{MoveSeriesToppJ --type=start --vel_coef="
                    << fixed << setprecision(5) << speed
                    << " --acc_coef=" << fixed << setprecision(5) << accel << "}";

                disable_stdout();
                robot::send_rpcsy(*robot_client,
                    { cmd_ss.str() }, 30000, 1000);
                enable_stdout();

                cout << "[成功] 轨迹执行完成！" << endl;
            }
            else if (user_input == "6") {
                cout << "[操作] 清空轨迹点..." << endl;
                disable_stdout();
                robot::send_rpcsy(*robot_client,
                    { "{MoveSeriesToppJ --type=clear}" }, 5000, 500);
                enable_stdout();

                trajectory_list.clear();
                cout << "[成功] 已清空所有轨迹点！" << endl;
            }
            else if (user_input == "7") {
                cout << "[信息] 轨迹点总数：" << trajectory_list.size() << endl;
                if (trajectory_list.empty()) {
                    cout << "  暂无轨迹点" << endl;
                }
                else {
                    for (size_t i = 0; i < trajectory_list.size(); i++) {
                        cout << i + 1 << ". " << trajectory_list[i].point_name << ": ";
                        show_joint_angles(trajectory_list[i].angles);
                        cout << endl;
                    }
                }
            }
            else if (user_input == "8") {
                cout << "[操作] 安全停止机器人..." << endl;
                disable_stdout();
                // 停止机器人运动
                robot::send_rpcsy(*robot_client,
                    { "{Stop --last_count=10}" }, 5000, 1000);
                // 恢复伺服使能
                robot::send_rpcsy(*robot_client,
                    { "{Start}" }, 5000, 1000);
                enable_stdout();

                cout << "[成功] 程序安全退出！" << endl;
                break;
            }
            else {
                cout << "\033[31m[错误] 请输入 1~8 的数字！\033[0m" << endl;
            }
        }
        catch (const exception& e) {
            cout << "\033[31m[错误] " << e.what() << "\033[0m" << endl;
        }
        catch (...) {
            cout << "\033[31m[错误] 未知错误，请检查输入！\033[0m" << endl;
        }
    }

    // 资源清理
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}