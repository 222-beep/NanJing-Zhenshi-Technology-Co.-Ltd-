// ==================================================================
//  main1.cpp  ——  单臂 JogAnyJ 正弦运动控制
//
//  功能：
//    1. 连接机器人控制器，执行 Clear → Disable → Enable 初始化
//    2. 支持 start / stop / custom / exit 交互式命令
//    3. start：以 100Hz 异步下发正弦运动指令，同步导出 CSV 指令数据
//    4. custom：手动输入关节角度，同步下发单次指令
//    5. 退出或停止时自动发送 Stop 指令
// ==================================================================

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include "rpc_client.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <limits>
#include <cmath>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>
#include <iomanip>

// 获取壁钟时间戳（毫秒级）
inline int64_t wall_clock_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

using namespace std;

// ==================================================================
//  常量配置
// ==================================================================

const int NUM_JOINTS = 7;
const double PI = 3.14159265358979323846;

// 初始关节角度（度）
const double XX[NUM_JOINTS] = { 0, 0, 0, 0, 0, 0, 0 };
// 各关节振幅（度）
const double AMP_DEG[NUM_JOINTS] = { 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0 };

// 正弦运动参数
const double SINE_FREQ = 2.0;    // 频率 2Hz（周期 0.5s）
const double SINE_DT   = 0.01;   // 控制步长 20ms（100Hz 下发）

// JogAnyJ 运动指令参数（正弦运动时使用）
const string MOTION_ACC = "{12.0}";   // 加速度
const string MOTION_DEC = "{12.0}";   // 减速度
const string MOTION_VEL = "{3.0}";    // 速度
const int    MOTION_LAST_COUNT = 20;

// JogAnyJ 启动/自定义指令参数（归零、custom 时使用）
const string START_ACC = "{1.0}";     // 加速度（低速）
const string START_DEC = "{1.0}";     // 减速度（低速）
const string START_VEL = "{1.0}";     // 速度（低速）
const int    START_LAST_COUNT = 300;

// 机器人 IP 与端口
const string ROBOT_IP  = "192.168.11.11";
const int    ROBOT_PORT = 5868;

// 运行标志
atomic<bool> running(false);

// ==================================================================
//  工具函数
// ==================================================================

// 生成 {0,0,...,0} 格式字符串（num_joints 个关节 + 3 个填充 0）
string make_zero_joint_pos(int num_joints) {
    string result = "{";
    int total = num_joints + 3;
    for (int i = 0; i < total; ++i) {
        result += "0";
        if (i < total - 1) result += ",";
    }
    result += "}";
    return result;
}

// 根据弧度数组生成 {val,val,...,0,0,0} 格式的关节位置字符串
string make_joint_pos(const double rad[], int n) {
    string result = "{";
    for (int i = 0; i < n; ++i) {
        char buf[32];
        sprintf(buf, "%.6f", rad[i]);
        result += buf;
        if (i < n) result += ",";
    }
    result += "0,0,0}";
    return result;
}

// 构造 JogAnyJ 指令字符串
string make_jog_cmd(const string& joint_pos,
                    const string& acc, const string& dec,
                    const string& vel, int last_count) {
    return "{JogAnyJ --jointtarget_value=" + joint_pos
         + " --joint_acc=" + acc
         + " --joint_dec=" + dec
         + " --joint_vel=" + vel
         + " --last_count=" + to_string(last_count)
         + "}";
}

// 逗号分隔字符串
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// ==================================================================
//  正弦运动循环（异步发送，100Hz 下发指令，同步导出 CSV 指令数据）
// ==================================================================

void sine_motion_loop(cpp_rpc::CPPClient& client) {
    ofstream csv_file("sine_motion_data.csv");
    if (!csv_file.is_open()) {
        cerr << "无法创建数据文件！" << endl;
        return;
    }

    // CSV 表头：增加 wall_clock_ms 列，用于与 Topic CSV 对齐时间轴
    csv_file << "time,wall_clock_ms";
    for (int i = 0; i < NUM_JOINTS; ++i)
        csv_file << ",joint" << (i + 1);
    csv_file << endl;

    int64_t start_wall = wall_clock_ms();
    // 记录启动时间戳到独立文件，方便后续与 Topic CSV 对比
    ofstream log_file("jog_start_time.txt");
    if (log_file.is_open()) {
        log_file << start_wall << endl;
        log_file.close();
        cout << "[时间同步] 运动启动壁钟时间已写入 jog_start_time.txt" << endl;
        cout << "[时间同步] start_wall_clock = " << start_wall << " ms" << endl;
    }

    double t = 0.0;
    double omega = 2.0 * PI * SINE_FREQ;  // 角频率 ω = 2πf

    while (running) {
        t += SINE_DT;

        // 计算各关节角度
        double x_deg[NUM_JOINTS];
        double x_rad[NUM_JOINTS];
        for (int i = 0; i < NUM_JOINTS; ++i) {
            x_deg[i] = XX[i] + AMP_DEG[i] * sin(omega * t);
            x_rad[i] = x_deg[i] / 180.0 * PI;
        }

        // 写入 CSV（度数 + 壁钟时间戳）
        csv_file << fixed << setprecision(6) << t
                 << "," << wall_clock_ms();
        for (int i = 0; i < NUM_JOINTS; ++i)
            csv_file << "," << x_deg[i];
        csv_file << endl;

        // 构造 JogAnyJ 指令并异步发送
        string joint_pos = make_joint_pos(x_rad, NUM_JOINTS);
        string cmd = make_jog_cmd(joint_pos, MOTION_ACC, MOTION_DEC, MOTION_VEL, MOTION_LAST_COUNT);

        vector<string> cmds = { cmd };
        send_rpcAsy(client, cmds, 20, 600000);
    }

    csv_file.close();
    cout << "数据已保存到 sine_motion_data.csv" << endl;
}

// ==================================================================
//  主函数
// ==================================================================

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    string input;

    // 初始化命令：清除错误 → 去使能 → 使能
    vector<string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Enable}",
    };

    // 全零初始关节位置
    string zero_joint_pos = make_zero_joint_pos(NUM_JOINTS);

    // JogAnyJ 初始动作：归零，低速到达初始位置
    vector<string> jog_start_cmds = {
        make_jog_cmd(zero_joint_pos, START_ACC, START_DEC, START_VEL, START_LAST_COUNT)
    };

    // 停止动作
    vector<string> jog_stop_cmds = {
        "{Stop --last_count=10}"
    };

    // 创建 RPC 客户端连接
    cpp_rpc::CPPClient client(ROBOT_IP, ROBOT_PORT);

    // 执行初始化命令序列（Clear → Disable → Enable）
    send_rpcsy<RespDemo>(client, init_cmds, 100, 5000);

    // 主循环
    while (true) {
        cout << "\n可用命令:\n";
        cout << "start  - 启动 JogAnyJ 正弦运动\n";
        cout << "stop   - 停止运动\n";
        cout << "custom - 输入自定义关节位置\n";
        cout << "exit   - 退出程序\n";
        cout << "请输入命令: ";

        cin >> input;
        transform(input.begin(), input.end(), input.begin(), ::tolower);

        if (input == "start") {
            if (!running) {
                running = true;
                cout << "启动正弦运动控制...\n";
                thread motion_thread(sine_motion_loop, ref(client));
                motion_thread.detach();
                cout << "正弦运动已启动，输入 'stop' 停止\n";
            } else {
                cout << "运动已在运行中\n";
            }
        }
        else if (input == "stop") {
            running = false;
            delay_ms(100);
            send_rpcsy<RespDemo>(client, jog_stop_cmds, 100, 5000);
            cout << "运动已停止\n";
        }
        else if (input == "custom") {
            try {
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');

                cout << "请输入" << NUM_JOINTS << "个关节角度(弧度)，用逗号分隔: ";
                string joint_input;
                getline(cin, joint_input);

                vector<string> joints_str = split(joint_input, ',');
                if ((int)joints_str.size() != NUM_JOINTS) {
                    cout << "错误: 需要输入" << NUM_JOINTS << "个关节角度!\n";
                    continue;
                }

                string joint_pos = "{";
                for (int i = 0; i < NUM_JOINTS; ++i) {
                    joint_pos += joints_str[i];
                    joint_pos += ",";
                }
                joint_pos += "0,0,0}";

                string custom_cmd = make_jog_cmd(joint_pos, START_ACC, START_DEC, START_VEL, START_LAST_COUNT);
                cout << "执行指令: " << custom_cmd << endl;

                vector<string> custom_cmds = { custom_cmd };
                send_rpcsy<RespDemo>(client, custom_cmds, 100, 5000);
            }
            catch (const exception& e) {
                cout << "输入格式错误，请确保输入的是数字\n";
                cout << "错误信息: " << e.what() << endl;
            }
        }
        else if (input == "exit") {
            cout << "退出程序...\n";
            running = false;
            delay_ms(100);
            send_rpcsy<RespDemo>(client, jog_stop_cmds, 100, 5000);
            break;
        }
        else {
            cout << "未知命令，请重新输入!\n";
        }
    }

    return 0;
}
