#include "rpc_client.h"
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
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    const std::string robot_ip = "192.168.2.217";
    cpp_rpc::CPPClient client(robot_ip, 5868);

    // 发送初始化指令
    // send_rpcsy<RespDemo>(client, init_cmds, 1000, 500);

    // 第一条SubLoop指令
    vector<string> first_subloop_cmds = {
        "{SubLoop --exec={Recover}||SubLoop --exec={Recover}}",
    };

    // 第一条指令必须是异步，且timeout必须设置得足够大，否则后续指令会被丢弃
    // send_rpcAsy(client, first_subloop_cmds, 1000, INT32_MAX);

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
    // send_rpcAsy(client, your_old_cmds, 1000, 50000);

    // 主循环发送运动指令
    while (true) {
        cerr << "Input commands: " << endl;
        cin >> ws; // 清除输入缓冲区中的空白字符
        string input_cmd;
        getline(cin, input_cmd);
        your_cmds.clear();
        your_cmds.push_back(input_cmd);
        // send_rpcsy<RespDemo>(client, your_cmds, 5000, 50000);
        send_rpcAsy(client, your_cmds, 3000, 1000000);
    }

    return 0;
}