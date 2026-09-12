#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <future>

// ==================================================================
//  main  ——  使用示例
// ==================================================================

int main() {
    const std::string robot_ip = "192.168.11.11";

    // ---- 命令定义 --------------------------------------------------

    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Recover}",
        "{Mode}",
        "{Enable}",
        "{SetUsingSP --state=on}",
        "{Start}"
    };

    // MoveBlend 轨迹命令：first_insert 设置起点 -> insert_line/insert_circle 添加轨迹点 -> start 执行
    std::vector<std::string> moveblend_cmd = {
    //    "{MoveBlend --type=first_insert}  设置轨迹起点
    //     {MoveBlend --type=insert_line   --robottarget_value={ x,y,z,q1,q2,q3,q4 } --zone={ 过渡区 } --speed=v50}  设置轨迹直线点
    //     {MoveBlend --type=insert_circle --robottarget_value={...} --zone={...} --speed=v50}  设置轨迹圆弧点
    //     {MoveBlend --type=start}  启动轨迹执行"
        "{MoveBlend --type=first_insert}",
        "{MoveBlend --type=insert_line --robottarget_value={0.036,0,0.9,0,0,0,1} --zone={0.1,0.1} --speed=v50}",
        "{MoveBlend --type=insert_line --robottarget_value={0.036,-0.068,0.9,0,0,0,1} --zone={0.1,0.1} --speed=v50}",
        "{MoveBlend --type=start}"
    };

    // ---- 连接机器人控制器 -------------------------------------------

    std::cout << "Connecting: " << std::endl;
    cpp_rpc::CPPClient client(robot_ip, 5868);
    if (!client.IsConnected()) {
        std::cerr << "Connection failed! Aborting all commands." << std::endl;
        return -1;
    }
    std::cout << "Connected: " << std::endl;

    // ==================================================================
    //  示例 1：通用同步 RPC（最常见用法）
    //  返回值只有 return_code / subcmd_index / return_message
    //  执行初始化
    // ==================================================================
    //  send_rpcsy<RespDemo>(client, init_cmds, 间隔ms, 超时ms)

    send_rpcsy<RespDemo>(client, init_cmds, 100, 50000);

    // ==================================================================
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    //  for 循环重复下发 MoveBlend 轨迹序列
    // ==================================================================
    //  send_rpcAsy(client, moveblend_cmd, 间隔ms, 超时ms)

    //持续发送 10 组指令
    for(int i = 0; i < 10; ++i)
    {
        send_rpcAsy(client, moveblend_cmd, 0, 50000);
        delay_ms(5000);
    }

    return 0;
}
