#include "robot.hpp"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

// 初始化命令列表
vector<string> init_cmds = {
    "{Clear}",
    "{Disable}",
    "{Mode}",
    "{SetMaxToq}",
    "{Recover}",
    "{SetRate}",
    "{Enable}",
    "{Var --clear}",
    "{Recover}",
};

int main() {
    const std::string robot_ip = "192.168.2.217";
    auto client = robot::create_client(robot_ip);

    // 发送初始化指令
    // robot::send_rpcsy(*client, init_cmds, 500, 1000);

    // 第一条SubLoop指令
    vector<string> first_subloop_cmds = {
        "{SubLoop --exec={Recover}||SubLoop --exec={Recover}}",
    };

    // 第一条指令必须是异步，且timeout必须设置得足够大，否则后续指令会被丢弃
    // robot::send_rpc_async(*client, first_subloop_cmds, INT32_MAX, 1000);

    vector<string> your_old_cmds = {
        "{MoveAbsJ||MoveAbsJ}",
    };

    // 示例指令列表，替换为你需要测试的指令（双模型）
    vector<string> your_cmds = {
        "{SubLoop --exec=Recover||SubLoop --exec=Recover}",
        "{SubLoop --exec=MoveAbsJ||SubLoop --exec=MoveAbsJ}",
        "{SubLoop --exec=exit||SubLoop --exec=exit}",
        "{SubLoop --exec||SubLoop --exec}",
    };

    // 普通异步RPC调用示例
    // robot::send_rpc_async(*client, your_old_cmds, 50000, 1000);

    // 主循环发送运动指令
    while (true) {
        cerr << "Input commands: " << endl;
        cin >> ws; // 清除输入缓冲区中的空白字符
        string input_cmd;
        getline(cin, input_cmd);
        your_cmds.clear();
        your_cmds.push_back(input_cmd);
        // robot::send_rpcsy(*client, your_cmds, 50000, 5000);
        robot::send_rpc_async(*client, your_cmds, 1000000, 3000);
    }

    return 0;
}