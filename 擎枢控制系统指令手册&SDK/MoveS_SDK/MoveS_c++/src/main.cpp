#include "robot.hpp"
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
    string type;             // "moves"
    vector<double> values;   // x,y,z,q1,q2,q3,q4
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

static void print_vector(const vector<double>& values) {
    cout << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) cout << ", ";
        cout << values[i];
    }
    cout << "]";
}

int main() {
    const std::string robot_ip = "192.168.2.241";
    auto client = robot::create_client(robot_ip);

    // 发送初始化指令
    robot::send_rpcsy(*client, init_cmds, 500, 100);

    vector<TrajectoryPoint> trajectory_points;
    string user_input;

    while (true) {
        cout << "\n=== MoveS 轨迹控制 ===" << endl;
        cout << "start_moves    - 开始MoveS轨迹（设置起点）" << endl;
        cout << "add_moves      - 添加MoveS轨迹点" << endl;
        cout << "execute        - 执行轨迹" << endl;
        cout << "clear_points   - 清除所有轨迹点" << endl;
        cout << "show_points    - 显示当前轨迹点" << endl;
        cout << "exit           - 退出程序" << endl;

        cout << "请输入命令: ";
        getline(cin, user_input);

        transform(user_input.begin(), user_input.end(), user_input.begin(),
                  [](unsigned char c) { return static_cast<char>(tolower(c)); });

        if (user_input == "start_moves") {
            cout << "设置MoveS起点（当前位置）" << endl;
            robot::send_rpcsy(*client, {"{MoveS --type=first_insert}"}, 5000, 500);

            trajectory_points.clear();
            cout << "MoveS起点已设置" << endl;
        }

        else if (user_input == "add_moves") {
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

                string point_name = "p" + to_string(trajectory_points.size() + 1);
                string value_str = build_value_string(values);

                string var_cmd =
                    "{Var --type=robottarget --name=" + point_name +
                    " --value={" + value_str + "}}";

                string move_cmd =
                    "{MoveS --type=insert --robottarget_var=" + point_name + "}";

                robot::send_rpcsy(*client, {var_cmd}, 5000, 200);
                robot::send_rpcsy(*client, {move_cmd}, 5000, 500);

                trajectory_points.push_back({point_name, "moves", values});

                cout << "MoveS轨迹点 " << point_name << " 已添加" << endl;
            }
            catch (const exception& e) {
                cout << "输入格式错误! 请确保输入的是数字。" << endl;
                cout << "错误信息: " << e.what() << endl;
            }
        }

        else if (user_input == "execute") {
            if (trajectory_points.empty()) {
                cout << "错误: 没有轨迹点可执行!" << endl;
                continue;
            }

            cout << "开始执行MoveS轨迹，共 " << trajectory_points.size() << " 个轨迹点..." << endl;
            robot::send_rpcsy(*client, {"{MoveS --type=start}"}, 10000, 1000);
            cout << "MoveS轨迹执行完成!" << endl;
        }

        else if (user_input == "clear_points") {
            robot::send_rpcsy(*client, {"{Var --clear}"}, 5000, 500);
            trajectory_points.clear();
            cout << "所有MoveS轨迹点已清除" << endl;
        }

        else if (user_input == "show_points") {
            cout << "当前MoveS轨迹点数量: " << trajectory_points.size() << endl;

            for (size_t i = 0; i < trajectory_points.size(); ++i) {
                const auto& point = trajectory_points[i];
                cout << "  " << i + 1 << ". MoveS点 " << point.name << ": ";
                print_vector(point.values);
                cout << endl;
            }
        }

        else if (user_input == "exit") {
            cout << "退出程序..." << endl;
            robot::send_rpcsy(*client, {"{Stop --last_count=10}"}, 5000, 1000);
            break;
        }

        else {
            cout << "未知命令，请重新输入!" << endl;
        }
    }

    return 0;
}
