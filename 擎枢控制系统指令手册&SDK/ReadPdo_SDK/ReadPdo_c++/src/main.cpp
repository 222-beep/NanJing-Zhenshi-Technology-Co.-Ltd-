#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

// ==================================================================
//  main  ——  使用示例
// ==================================================================

int main() {
    const std::string robot_ip = "192.168.11.11";

    // ---- 命令定义 --------------------------------------------------

    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Mode}",
        "{SetMaxToq}",
        "{Recover}",
        "{SetRate}",
        "{Enable}",
    };

    // ReadPdo 指令参数（可自行修改）
    // slave_id  : 从站编号
    // index     : PDO 对象索引（十六进制）
    // sub_index : PDO 对象子索引（十六进制）
    // size      : 读取数据位宽（bit）
    // interval  : 读取间隔（s）
    // loop      : 循环读取次数
    std::vector<std::string> readpdo_cmds = {
        "{ReadPdo --slave_id=6 --index=0x6041 --sub_index=0x00 --size=16 --interval=1 --loop=1}"
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
    //  ReadPdo 使用专用响应类型 RespPdo，额外返回 pdo_value
    // ==================================================================
    // 可选参数: send_rpcsy<RespXxx>(client, cmds, 间隔ms, 超时ms)
    send_rpcsy<RespDemo>(client, init_cmds, 100, 500);

    auto results = send_rpcsy<RespPdo>(client, readpdo_cmds, 500, 5000);
    for (const auto& r : results) {
        std::cout << "[ReadPdo] subcmd_index: " << r.subcmd_index << std::endl;
        std::cout << "[ReadPdo] return_code: " << r.return_code << std::endl;
        std::cout << "[ReadPdo] return_message: " << r.return_message << std::endl;
        if (r.has_pdo_value) {
            printf("[ReadPdo] pdo_value: %d (0x%X)\n", r.pdo_value, r.pdo_value);
        }
    }

    // ==================================================================
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    // ==================================================================
    // 可选参数: send_rpcAsy(client, cmds, 等待ms, 超时ms)
    // send_rpcAsy(client, readpdo_cmds, 500, 5000);

    // // ==================================================================
    // //  示例 3：扩展返回值（PointChooseIDMove 返回 target_pq）
    // //  当某个指令返回了额外的字段时，使用专用的响应类型
    // // ==================================================================
    // // 通过 CallAwait 直接拿到带扩展字段的返回结果
    // core::Msg req(your_cmds[0]);
    // req.setMsgID(10001);
    // auto results2 = client.CallAwait<PointChooseIDMoveResp>(req, 5000);
    //
    // // ---- 拿到 target_pq，拼成 MoveBlend 指令序列再发送 --------------
    // if (results2.first == 0 && !results2.second.empty()) {
    //     std::vector<double>& pq = results2.second[0].target_pq;
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
