#include "rpc_client.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

// 初始化命令列表
vector<string> init_cmds = {
    "{Clear}",
    "{Disable}",
    "{Enable}",
};

struct TrajectoryPoint {
    string name;
    string type;                 // "line" or "circle"
    vector<double> values;       // line point
    vector<double> mid_point;    // circle mid point
    vector<double> target_point; // circle target point
    string zone;
    string speed;
};

static vector<double> parse_values(const string& input) { 
    vector<double> values;
    stringstream ss(input);
    string item;

    while (getline(ss, item, ',')) {
        values.push_back(stod(item));
    }
    return values;
}

static string build_value_string(const vector<double>& values) {
    ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ",";
        oss << values[i];
    }
    return oss.str();
}

static string parse_zone(const string& input) {
    if (input.empty()) {
        return "{0.1,0.1}";
    }

    vector<double> zone_values = parse_values(input);
    if (zone_values.size() != 2) {
        return "{0.1,0.1}";
    }

    ostringstream oss;
    oss << "{" << zone_values[0] << "," << zone_values[1] << "}";
    return oss.str();
}

static void print_vector(const vector<double>& values) {
    cout << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) cout << ", ";
        cout << values[i];
    }
    cout << "]";
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    const std::string robot_ip = "192.168.2.199";
    cpp_rpc::CPPClient client(robot_ip, 5868);

    send_rpcsy<RespDemo>(client, init_cmds, 100, 500);

    vector<TrajectoryPoint> trajectory_points;
    string user_input;

    while (true) {
        cout << "\n=== MoveBlend 轨迹控制 ===" << endl;
        cout << "first_insert  - 添加起点（当前位置）" << endl;
        cout << "add_line      - 添加直线轨迹点" << endl;
        cout << "add_circle    - 添加圆弧轨迹点（需要中间点）" << endl;
        cout << "start         - 执行轨迹" << endl;
        cout << "clear_points  - 清除所有轨迹点" << endl;
        cout << "show_points   - 显示当前轨迹点" << endl;
        cout << "exit          - 退出程序" << endl;

        cout << "请输入命令: ";
        getline(cin, user_input);

        transform(user_input.begin(), user_input.end(), user_input.begin(),
                  [](unsigned char c) { return static_cast<char>(tolower(c)); });

        if (user_input == "first_insert") {
            cout << "设置起点（当前位置）" << endl;
            send_rpcsy<RespDemo>(client, {"{MoveBlend --type=first_insert}"}, 500, 5000);

            trajectory_points.clear();
            cout << "起点已设置" << endl;
        }

        else if (user_input == "add_line") {
            try {
                cout << "请输入目标点坐标和姿态（x,y,z,q1,q2,q3,q4）：" << endl;
                cout << "示例：0.32,-0.32,0.48,0,1,0,0" << endl;

                string target_input;
                cout << "目标点: ";
                getline(cin, target_input);

                vector<double> values = parse_values(target_input);
                if (values.size() != 7) {
                    cout << "错误: 需要7个值（x,y,z,q1,q2,q3,q4）!" << endl;
                    continue;
                }

                string zone_input;
                cout << "请输入zone值（如 0.1,0.1，回车默认）: ";
                getline(cin, zone_input);
                string zone = parse_zone(zone_input);

                string speed_input;
                cout << "请输入speed值（如 v50，回车默认v50）: ";
                getline(cin, speed_input);
                string speed = speed_input.empty() ? "v50" : speed_input;

                string point_name = "p" + to_string(trajectory_points.size() + 1);
                string value_str = build_value_string(values);

                string var_cmd =
                    "{Var --type=robottarget --name=" + point_name +
                    " --value={" + value_str + "}}";

                string line_cmd =
                    "{MoveBlend --type=insert_line --robottarget_var=" + point_name +
                    " --zone=" + zone +
                    " --speed=" + speed + "}";

                send_rpcsy<RespDemo>(client, {var_cmd}, 200, 5000);
                send_rpcsy<RespDemo>(client, {line_cmd}, 500, 5000);

                trajectory_points.push_back({
                    point_name, "line", values, {}, {}, zone, speed
                });

                cout << "直线轨迹点 " << point_name << " 已添加" << endl;
            }
            catch (const exception& e) {
                cout << "输入格式错误! 请确保输入的是数字。" << endl;
                cout << "错误信息: " << e.what() << endl;
            }
        }

        else if (user_input == "add_circle") {
            try {
                cout << "请输入中间点坐标和姿态（x,y,z,q1,q2,q3,q4）：" << endl;
                string mid_input;
                cout << "中间点: ";
                getline(cin, mid_input);

                vector<double> mid_values = parse_values(mid_input);

                cout << "请输入目标点坐标和姿态（x,y,z,q1,q2,q3,q4）：" << endl;
                string target_input;
                cout << "目标点: ";
                getline(cin, target_input);

                vector<double> target_values = parse_values(target_input);

                if (mid_values.size() != 7 || target_values.size() != 7) {
                    cout << "错误: 需要7个值（x,y,z,q1,q2,q3,q4）!" << endl;
                    continue;
                }

                string zone_input;
                cout << "请输入zone值（如 0.1,0.1，回车默认）: ";
                getline(cin, zone_input);
                string zone = parse_zone(zone_input);

                string speed_input;
                cout << "请输入speed值（如 v50，回车默认v50）: ";
                getline(cin, speed_input);
                string speed = speed_input.empty() ? "v50" : speed_input;

                string mid_name = "mid" + to_string(trajectory_points.size() + 1);
                string target_name = "target" + to_string(trajectory_points.size() + 1);

                string mid_value_str = build_value_string(mid_values);
                string target_value_str = build_value_string(target_values);

                string var_mid_cmd =
                    "{Var --type=robottarget --name=" + mid_name +
                    " --value={" + mid_value_str + "}}";

                string var_target_cmd =
                    "{Var --type=robottarget --name=" + target_name +
                    " --value={" + target_value_str + "}}";

                string circle_cmd =
                    "{MoveBlend --type=insert_circle --mid_robottarget_var=" + mid_name +
                    " --robottarget_var=" + target_name +
                    " --zone=" + zone +
                    " --speed=" + speed + "}";

                send_rpcsy<RespDemo>(client, {var_mid_cmd, var_target_cmd}, 200, 5000);
                send_rpcsy<RespDemo>(client, {circle_cmd}, 500, 5000);

                trajectory_points.push_back({
                    target_name, "circle", {}, mid_values, target_values, zone, speed
                });

                cout << "圆弧轨迹点 " << target_name << " 已添加" << endl;
            }
            catch (const exception& e) {
                cout << "输入格式错误! 请确保输入的是数字。" << endl;
                cout << "错误信息: " << e.what() << endl;
            }
        }

        else if (user_input == "start") {
            if (trajectory_points.empty()) {
                cout << "错误: 没有轨迹点可执行!" << endl;
                continue;
            }

            cout << "开始执行轨迹，共 " << trajectory_points.size() << " 个轨迹点..." << endl;
            send_rpcsy<RespDemo>(client, {"{MoveBlend --type=start}"}, 1000, 10000);
            cout << "轨迹执行完成!" << endl;
        }

        else if (user_input == "clear_points") {
            send_rpcsy<RespDemo>(client, {"{Var --clear}"}, 500, 5000);
            trajectory_points.clear();
            cout << "所有轨迹点已清除" << endl;
        }

        else if (user_input == "show_points") {
            cout << "当前轨迹点数量: " << trajectory_points.size() << endl;

            for (size_t i = 0; i < trajectory_points.size(); ++i) {
                const auto& point = trajectory_points[i];

                if (point.type == "line") {
                    cout << "  " << i + 1 << ". 直线点 " << point.name << ": ";
                    print_vector(point.values);
                    cout << ", zone=" << point.zone << ", speed=" << point.speed << endl;
                }
                else if (point.type == "circle") {
                    cout << "  " << i + 1 << ". 圆弧点 " << point.name << ": 中间点=";
                    print_vector(point.mid_point);
                    cout << ", 目标点=";
                    print_vector(point.target_point);
                    cout << ", zone=" << point.zone << ", speed=" << point.speed << endl;
                }
            }
        }

        else if (user_input == "exit") {
            cout << "退出程序..." << endl;
            send_rpcsy<RespDemo>(client, {"{Stop --last_count=10}"}, 1000, 5000);
            break;
        }

        else {
            cout << "未知命令，请重新输入!" << endl;
        }
    }

    return 0;
}