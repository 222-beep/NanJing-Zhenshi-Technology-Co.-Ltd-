#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>

// ==================================================================
//  main  ——  使用示例
// ==================================================================

int main() {
    const std::string robot_ip = "192.168.11.11";

    // ---- 命令定义 --------------------------------------------------

    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Enable}",
    };

    // MoveBlend 轨迹命令：first_insert 设置起点 -> insert_line/insert_circle 添加轨迹点 -> start 执行
    // robottarget 为笛卡尔位姿 x,y,z,q1,q2,q3,q4，x,y,z 单位：米
    std::vector<std::string> moveblend_cmds = {
        "{MoveBlend --type=first_insert}",
        "{MoveBlend --type=insert_line --robottarget_value={0.32,-0.32,0.48,0,1,0,0} --zone={0.1,0.1} --speed=v50}",
        "{MoveBlend --type=start}"
    };

    // 停止命令
    std::vector<std::string> stop_cmds = {
        "{Stop --last_count=10}"
    };

    std::vector<std::string> your_cmds = {
        "{PointChooseIDMove --mid_point_robottarget=ppp --point_id=13 --len_end=89 --len_point=10 --cal_on=0}"

        //add your cmds

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
    // ==================================================================
    // 可选参数: send_rpcsy<RespDemo>(client, cmds, 间隔ms, 超时ms)
    send_rpcsy<RespDemo>(client, init_cmds, 100, 500);
    send_rpcsy<RespDemo>(client, moveblend_cmds, 500, 10000);
    send_rpcsy<RespDemo>(client, stop_cmds, 1000, 5000);

    // ==================================================================
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    // ==================================================================
    // 可选参数: send_rpcAsy(client, cmds, 等待ms, 超时ms)
    // send_rpcAsy(client, moveblend_cmds, 500, 10000);

    // // ==================================================================
    // //  示例 3：扩展返回值（PointChooseIDMove 返回 target_pq）
    // //  当某个指令返回了额外的字段时，使用专用的响应类型
    // // ==================================================================
    // // 通过 CallAwait 直接拿到带扩展字段的返回结果
    // core::Msg req(your_cmds[0]);
    // req.setMsgID(10001);
    // auto results = client.CallAwait<PointChooseIDMoveResp>(req, 5000);
    //
    // // ---- 拿到 target_pq，拼成 MoveBlend 指令序列再发送 --------------
    // if (results.first == 0 && !results.second.empty()) {
    //     std::vector<double>& pq = results.second[0].target_pq;
    //     char buf[512];
    //     snprintf(buf, sizeof(buf),
    //         "{MoveBlend --type=insert_line --robottarget_value={%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f} --speed=v50}",
    //         pq[0], pq[1], pq[2], pq[3], pq[4], pq[5], pq[6]);
    //     std::vector<std::string> blend_cmds = {
    //         "{MoveBlend --type=first_insert}",
    //         buf,
    //         "{MoveBlend --type=start}"
    //     };
    //     send_rpcsy<RespDemo>(client, blend_cmds, 500, 5000);
    // }

    return 0;
}
