#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <future>

// ==================================================================
//  main  ——  JogAnyJFast 使用示例
//
//  JogAnyJFast 参数：
//    --joint_vel   关节速度上限
//    --joint_acc   关节加速度上限
//    --joint_dec   关节减速度上限
//    --last_count  末尾保持周期数
//    --index       0:127.0.0.1的5900端口发送的指令，1：192.168.11.11的5900端口发送的指令
// ==================================================================

int main() {

    //机器人IP地址
    const std::string robot_ip = "192.168.11.11";
    std::cout << "Robot IP: " << robot_ip << std::endl;

    // ---- 命令定义 --------------------------------------------------

    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Recover}",
        "{Mode}",
        "{Enable}",
        "{Start}"
    };

    std::vector<std::string> joganyjfast_cmd = {
            "{JogAnyJFast --joint_vel={1.0} --joint_acc={1.0} --joint_dec={1.0} --last_count=10000 --index=0}"
    };

    std::vector<std::string> stop_cmds = {
        "{Stop --last_count=10}"
    };

    // ---- 连接机器人控制器（5868：控制指令通道）----------------------

    std::cout << "Connecting 5868: " << std::endl;
    cpp_rpc::CPPClient rpc_client(robot_ip, 5868);
    if (!rpc_client.IsConnected()) {
        std::cerr << "5868 connection failed: "
            << rpc_client.GetErrorInfo() << std::endl;
        return 1;
    }
    std::cout << "Connected 5868: " << std::endl;

    // ==================================================================
    //  示例 1：通用同步 RPC
    //  经 5868 执行初始化
    // ==================================================================
    //  send_rpcsy<RespDemo>(client, cmds, 间隔ms)

    send_rpcsy<RespDemo>(rpc_client, init_cmds, 0);

    cpp_rpc::RealtimeChannel realtime(robot_ip); // 默认端口 5900
    if (!realtime.Connect()) {
        std::cerr << "5900 connection failed: "
            << realtime.LastError() << std::endl;
        send_rpcsy<RespDemo>(rpc_client, stop_cmds, 0, true);
        return 2;
    }
    send_rpcAsy(rpc_client, joganyjfast_cmd, 0);

    delay_ms(5000); // 等待 5 秒，确保机器人进入快速运动模式
    // ==================================================================
    //  示例 3：5900 端口下发关节目标
    //  FastJointTargetParam 为 5 组臂 × 10 关节的绝对关节位置（单位 rad），
    //  每次 send_realtime_joint_target 发送一帧，发送周期由调用方控制；
    //  本示例以 1ms 周期持续发送 2000 帧。
    //  arm_target_deg 为目标姿态（单位：度）
    // ==================================================================
    cpp_rpc::FastJointTargetParam target{};
    const double arm_target_deg[cpp_rpc::REALTIME_JOINT_COUNT] = {
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
    };
    const double DEG2RAD = 3.1415926 / 180.0; // 度→弧度 换算系数
    for (std::size_t joint = 0; joint < cpp_rpc::REALTIME_JOINT_COUNT; ++joint) {
        target.joint_target_pos[0][joint] = arm_target_deg[joint] * DEG2RAD; // 第 1 组机械臂
    }

    constexpr int send_count = 2000;        // 发送总帧数
    constexpr int send_interval_ms = 1;     // 发送周期（ms）
    int sent_count = 0;
    auto result = cpp_rpc::RealtimeSendResult::Ok;

    for (int i = 0; i < send_count; ++i) {
        target.count++;  // count = 当前 JogAnyJFast 运行次数,JogAnyJFast 运行次数：每运行一次+1

        result = send_realtime_joint_target(realtime, target);
        if (result != cpp_rpc::RealtimeSendResult::Ok) {
            std::cerr << "5900 send failed at frame " << i << ": "
                << realtime.LastError() << std::endl;
            break;
        }
        ++sent_count;
        delay_ms(send_interval_ms);
    }
    std::cout << "5900 sent " << sent_count << "/" << send_count
        << " frames." << std::endl;

    realtime.Close();

    // ---- 停止运动（5868）--------------------------------------------

    send_rpcsy<RespDemo>(rpc_client, stop_cmds, 0, true);
    std::cout << "Motion stopped." << std::endl;

    return result == cpp_rpc::RealtimeSendResult::Ok ? 0 : 3;

}