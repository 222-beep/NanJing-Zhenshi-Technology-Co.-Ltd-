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

    // ReadSdo 指令参数（可自行修改）
    // slave_id  : 从站编号（从 0 开始，0 通常代表网络中第一个从站）
    // index     : 对象字典主索引（十六进制）
    // sub_index : 对象字典子索引（十六进制）
    // size      : 字节长度，指定要读取的数据大小
    // loop      : 循环次数，指令命令执行的总次数
    std::vector<std::string> readsdo_cmds = {
        "{ReadSdo --slave_id=5 --index=0x6064 --sub_index=0x00 --size=4 --loop=1}"
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
    //  ReadSdo 使用专用响应类型 RespSdo，额外返回 sdo_value
    // ==================================================================
    // 可选参数: send_rpcsy<RespXxx>(client, cmds, 间隔ms, 超时ms)
    send_rpcsy<RespDemo>(client, init_cmds, 100, 500);

    auto results = send_rpcsy<RespSdo>(client, readsdo_cmds, 500, 5000);
    for (const auto& r : results) {
        std::cout << "[ReadSdo] subcmd_index: " << r.subcmd_index << std::endl;
        std::cout << "[ReadSdo] return_code: " << r.return_code << std::endl;
        std::cout << "[ReadSdo] return_message: " << r.return_message << std::endl;
        if (r.has_sdo_value) {
            printf("[ReadSdo] sdo_value: %d (0x%X)\n", r.sdo_value, r.sdo_value);
        }
    }

    // ==================================================================
    //  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    // ==================================================================
    // 可选参数: send_rpcAsy(client, cmds, 等待ms, 超时ms)
    // send_rpcAsy(client, readsdo_cmds, 500, 5000);

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
