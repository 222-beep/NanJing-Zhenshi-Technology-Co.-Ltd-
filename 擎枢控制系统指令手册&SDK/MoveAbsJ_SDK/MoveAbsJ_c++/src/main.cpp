#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>

// ==================================================================
//  main  ——  使用示例
// ==================================================================

int main() {
    const std::string robot_ip = "192.168.2.199";

    // ---- 命令定义 --------------------------------------------------

    std::vector<std::string> init_cmds = {
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
        "{Var --type=jointtarget --name=j4 --value={-0.2,0,0,0,0,0,0,0,0,0}}"
    };

    std::vector<std::string> motion_cmds = {
        "{MoveAbsJ --jointtarget_var=j0}",
        "{MoveAbsJ --jointtarget_var=j1}",
        "{MoveAbsJ --jointtarget_var=j2}"
    };

    std::vector<std::string> motion_speedL_cmds = {
        "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
        "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
        "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
        "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
        "{Stop}",
        "{Start}",
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

    // ==================================================================
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    // ==================================================================
    // 可选参数: send_rpcAsy(client, cmds, 等待ms, 超时ms)
    // send_rpcAsy(client, motion_speedL_cmds, 500, 10000);

    // // ==================================================================
    // //  示例 3：扩展返回值（PointChooseIDMove 返回 target_pq）
    // //  当某个指令返回了额外的字段时，使用专用的响应类型
    // // ==================================================================
    // // 通过 CallAwait 直接拿到带扩展字段的返回结果
    // core::Msg req(your_cmds[0]);
    // req.setMsgID(10001);
    // auto results = client->CallAwait<PointChooseIDMoveResp>(req, 5000);
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
