#include "rpc_client.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

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
    "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j3 --value={-0.1,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j4 --value={-0.2,0,0,0,0,0,0,0,0,0}}",
};

// 同步示例指令（MoveAbsJ 依次到 j0~j4）
vector<string> sync_cmds = {
    "{MoveAbsJ --jointtarget_var=j0}",
    "{MoveAbsJ --jointtarget_var=j1}",
    "{MoveAbsJ --jointtarget_var=j2}",
    "{MoveAbsJ --jointtarget_var=j3}",
    "{MoveAbsJ --jointtarget_var=j4}",
};

// 异步示例指令（SpeedL 往返 + Stop/Start）
vector<string> async_cmds = {
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
    "{Stop}",
    "{Start}",
};

// 计时辅助函数
static double elapsed_sec(chrono::steady_clock::time_point start) {
    auto end = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0;
}

static void demo_sync(cpp_rpc::CPPClient& client) {
    cout << "\n[同步发送] 开始，共 " << sync_cmds.size() << " 条指令" << endl;
    auto t0 = chrono::steady_clock::now();
    send_rpcsy<RespDemo>(client, sync_cmds, 500, 10000);
    cout << "[同步发送] 完成，耗时 " << elapsed_sec(t0) << " 秒" << endl;
}

static void demo_async(cpp_rpc::CPPClient& client) {
    cout << "\n[异步发送] 开始，共 " << async_cmds.size() << " 条指令" << endl;
    auto t0 = chrono::steady_clock::now();
    send_rpcAsy(client, async_cmds, 500, 10000);
    cout << "[异步发送] 完成，耗时 " << elapsed_sec(t0) << " 秒" << endl;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    const std::string robot_ip = "192.168.2.241";
    cpp_rpc::CPPClient client(robot_ip, 5868);

    // 发送初始化指令
    send_rpcsy<RespDemo>(client, init_cmds, 100, 500);
    cout << "初始化完成" << endl;

    string user_input;

    while (true) {
        cout << "\n=== 同步 / 异步 发送 ===" << endl;
        cout << "sync    - 运行同步发送" << endl;
        cout << "async   - 运行异步发送" << endl;
        cout << "compare - 同步 vs 异步 对比" << endl;
        cout << "clear   - 清除所有变量" << endl;
        cout << "exit    - 退出程序" << endl;

        cout << "请输入命令: ";
        getline(cin, user_input);

        // 转为小写
        for (auto& c : user_input) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

        if (user_input == "sync") {
            demo_sync(client);
        }
        else if (user_input == "async") {
            demo_async(client);
        }
        else if (user_input == "compare") {
            demo_sync(client);
            demo_async(client);
            cout << "\n对比完成" << endl;
        }
        else if (user_input == "clear") {
            send_rpcsy<RespDemo>(client, {"{Var --clear}"}, 500, 5000);
            cout << "所有变量已清除" << endl;
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
