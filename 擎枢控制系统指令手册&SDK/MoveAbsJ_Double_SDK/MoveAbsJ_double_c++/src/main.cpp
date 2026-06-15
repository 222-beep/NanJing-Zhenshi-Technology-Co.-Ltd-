#include "robot.hpp"
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
        // 定义机械臂1的关节目标变量
        "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j3 --value={-0.1,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j4 --value={-0.2,0,0,0,0,0,0,0,0,0}}",
        // 定义机械臂2的关节目标变量
        "{Var --type=jointtarget --name=j11 --value={0,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j21 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j22 --value={0.2,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j23 --value={-0.1,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j24 --value={-0.2,0,0,0,0,0,0,0,0,0}}"
    };

    // 双臂运动指令列表 - 使用||分隔两个机械臂的指令
    std::vector<std::string> motion_cmds = {
        // 双臂同时运动到初始位置
        "{MoveAbsJ --jointtarget_var=j0||MoveAbsJ --jointtarget_var=j11}",
        // 双臂同时运动到不同位置
        "{MoveAbsJ --jointtarget_var=j1||MoveAbsJ --jointtarget_var=j21}",
        "{MoveAbsJ --jointtarget_var=j2||MoveAbsJ --jointtarget_var=j22}",
        "{MoveAbsJ --jointtarget_var=j3||MoveAbsJ --jointtarget_var=j23}",
        "{MoveAbsJ --jointtarget_var=j4||MoveAbsJ --jointtarget_var=j24}"
    };

    // 双臂在线规划指令
    std::vector<std::string> motion_speedL_cmds = {
        // 双臂同时进行在线规划运动
        "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000||SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
        "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000||SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
        "{Stop||Stop}",
        "{Start||Start}",
    };

    std::vector<std::string> your_cmds = {
        "{PointChooseIDMove --mid_point_robottarget=ppp --point_id=13 --len_end=89 --len_point=10 --cal_on=0}"

        //add your cmds

    };

    // ---- 连接机器人控制器 -------------------------------------------

    std::cout << "Connecting: " << std::endl;
    auto client = robot::create_client(robot_ip);
    std::cout << "Connected: " << std::endl;

    // ==================================================================
    //  示例 1：通用同步 RPC（最常见用法）
    //  返回值只有 return_code / subcmd_index / return_message
    // ==================================================================
    // 可选参数: robot::send_rpcsy(*client, cmds, 超时ms, 间隔ms)
    robot::send_rpcsy(*client, init_cmds, 500, 100);

    // ==================================================================
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    // ==================================================================
    // 可选参数: robot::send_rpc_async(*client, cmds, 超时ms, 等待ms)
    // robot::send_rpc_async(*client, motion_speedL_cmds, 10000, 500);

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
    //     robot::send_rpcsy(*client, blend_cmds, 5000, 500);
    // }

    return 0;
}
