#include "multi_arm_command.hpp"
#include "message/resp_dto.h"
#include <iostream>

// ====================== 多臂命令实现 ======================

bool MultiArmCommand::MoveAbsjTriple(const ArmMoveAbsjParams& arm1,
    const ArmMoveAbsjParams& arm2,
    const ArmMoveAbsjParams& arm3)
{
    std::cout << "====== Sending MoveAbsjTriple command ======" << std::endl;
    auto buildCmd = [](const ArmMoveAbsjParams& arm) -> std::string {
        if (arm.is_run) {
            return "MoveAbsJ --jointtarget_var=" + arm.target_name +
                " --speed=v" + std::to_string(arm.speed);
        }
        return "NotRunExecute";
        };
    std::string combined = "{" + buildCmd(arm1) + "||" +
        buildCmd(arm2) + "||" +
        buildCmd(arm3) + "}";
    return sendSync({ combined }, 60000);
}

bool MultiArmCommand::JogJointTriple(const ArmJogJointParams& arm1,
    const ArmJogJointParams& arm2,
    const ArmJogJointParams& arm3)
{
    std::cout << "====== Sending JogJointTriple command ======" << std::endl;
    auto buildCmd = [](const ArmJogJointParams& arm) -> std::string {
        if (arm.is_run) {
            return "JogJoint --speed=v" + std::to_string(arm.speed) +
                " --motion_id=" + arm.motion_id +
                " --increase_count=50" +
                " --direction=" + std::to_string(arm.direction);
        }
        return "NotRunExecute";
        };
    std::string combined = "{" + buildCmd(arm1) + "||" +
        buildCmd(arm2) + "||" +
        buildCmd(arm3) + "}";
    return sendSync({ combined }, 5000);
}

bool MultiArmCommand::PointChooseIDMove1Triple(const ArmPointChooseIDMove1Params& arm1,
    const ArmPointChooseIDMove1Params& arm2,
    const ArmPointChooseIDMove1Params& arm3,
    std::vector<double>& output_pq)
{
    std::cout << "====== Sending PointChooseIDMove1Triple command ======" << std::endl;

    // 检查连接状态，防止空指针崩溃
    if (!client_ || !connected_.load()) {
        std::cerr << "Not connected, attempting to reconnect..." << std::endl;
        if (!connect(server_ip_, server_port_)) {
            std::cerr << "Reconnection failed." << std::endl;
            return false;
        }
    }

    auto buildCmd = [](const ArmPointChooseIDMove1Params& arm) -> std::string {
        if (!arm.is_run) return "NotRunExecute";
        std::string cmd = "PointChooseIDMove";
        cmd += " --mid_point_robottarget=" + arm.mid_point_robottarget;
        cmd += " --point_id=" + std::to_string(arm.point_id);
        cmd += " --len_end=" + std::to_string(arm.len_end);
        cmd += " --len_point=" + std::to_string(arm.len_point);
        cmd += " --cal_on=" + std::to_string(arm.cal_on);
        return cmd;
        };

    std::string combined = "{" + buildCmd(arm1) + "||" + buildCmd(arm2) + "||" + buildCmd(arm3) + "}";
    auto results = send_rpcsy<PointChooseIDMoveResp>(getClient(), { combined }, 0, false, 100000);
    if (!results.empty() && !results[0].target_pq.empty()) {
        const auto& pq = results[0].target_pq;
        // 拷贝前7个元素到 output_pq
        output_pq.clear();
        for (size_t i = 0; i < pq.size() && i < 7; ++i) {
            output_pq.push_back(pq[i]);
        }
        return true;
    }
    return false;
}

bool MultiArmCommand::PointFixSaveTriple(const ArmPointFixSaveParams& arm1,
    const ArmPointFixSaveParams& arm2,
    const ArmPointFixSaveParams& arm3)
{
    std::cout << "====== Sending PointFixSaveTriple command ======" << std::endl;

    auto buildCmd = [](const ArmPointFixSaveParams& arm) -> std::string {
        if (!arm.is_run) return "NotRunExecute";
        std::string cmd = "PointFixSave";
        cmd += " --point_id=" + std::to_string(arm.point_id);
        cmd += " --dx=" + std::to_string(arm.dx);
        cmd += " --dy=" + std::to_string(arm.dy);
        cmd += " --mode=" + arm.mode;
        return cmd;
        };

    std::string combined = "{" + buildCmd(arm1) + "||" + buildCmd(arm2) + "||" + buildCmd(arm3) + "}";
    return sendSync({ combined }, 10000);
}

bool MultiArmCommand::PointFitPlaneTriple(const ArmPointFitPlaneParams& arm1,
    const ArmPointFitPlaneParams& arm2,
    const ArmPointFitPlaneParams& arm3)
{
    std::cout << "====== Sending PointFitPlaneTriple command ======" << std::endl;

    auto buildCmd = [](const ArmPointFitPlaneParams& arm) -> std::string {
        if (!arm.is_run) return "NotRunExecute";
        return "PointFitPlane --len_point=" + std::to_string(arm.len_point);
        };

    std::string combined = "{" + buildCmd(arm1) + "||" + buildCmd(arm2) + "||" + buildCmd(arm3) + "}";
    return sendSync({ combined }, 10000);
}

bool MultiArmCommand::PointChooseIDMove2Triple(const ArmPointChooseIDMove2Params& arm1,
    const ArmPointChooseIDMove2Params& arm2,
    const ArmPointChooseIDMove2Params& arm3)
{
    std::cout << "====== Sending PointChooseIDMove2Triple command ======" << std::endl;

    auto buildCmd = [](const ArmPointChooseIDMove2Params& arm) -> std::string {
        if (!arm.is_run) return "NotRunExecute";
        std::string cmd = "PointChooseIDMove";
        cmd += " --mid_point_robottarget=" + arm.mid_point_robottarget;
        cmd += " --target_x=" + std::to_string(arm.target_x);
        cmd += " --target_y=" + std::to_string(arm.target_y);
        cmd += " --target_z=" + std::to_string(arm.target_z);
        cmd += " --len_end=" + std::to_string(arm.len_end);
        cmd += " --len_point=" + std::to_string(arm.len_point);
        cmd += " --cal_on=" + std::to_string(arm.cal_on);
        return cmd;
        };

    std::string combined = "{" + buildCmd(arm1) + "||" + buildCmd(arm2) + "||" + buildCmd(arm3) + "}";
    return sendSync({ combined }, 10000);
}

bool MultiArmCommand::JogCartesianTriple(const ArmJogCartesianParams& arm1,
    const ArmJogCartesianParams& arm2,
    const ArmJogCartesianParams& arm3)
{
    std::cout << "====== Sending JogCartesianTriple command ======" << std::endl;
    auto buildCmd = [](const ArmJogCartesianParams& arm) -> std::string {
        if (arm.is_run) {
            return "JogCartesian --moving_type=" + std::to_string(arm.moving_type) +
                " --direction=" + std::to_string(arm.direction) +
                " --speed=v" + std::to_string(arm.speed) +
                " --coordinate=" + std::to_string(arm.coordinate) +
                " --increase_count=50";
        }
        return "NotRunExecute";
        };
    std::string combined = "{" + buildCmd(arm1) + "||" +
        buildCmd(arm2) + "||" +
        buildCmd(arm3) + "}";
    return sendSync({ combined }, 5000);
}

bool MultiArmCommand::MoveBlendLineTriple(const ArmBlendLineParams& arm1,
    const ArmBlendLineParams& arm2,
    const ArmBlendLineParams& arm3)
{
    std::cout << "====== Executing MoveBlend sequence for three arms (parallel) ======" << std::endl;

    auto buildCommand = [](const ArmBlendLineParams& arm, const std::string& cmd) -> std::string {
        return arm.is_run ? cmd : "NotRunExecute";
        };

    // 1. 发送 first_insert
    std::string first_insert_cmd = "{" +
        buildCommand(arm1, "MoveBlend --type=first_insert") + "||" +
        buildCommand(arm2, "MoveBlend --type=first_insert") + "||" +
        buildCommand(arm3, "MoveBlend --type=first_insert") + "}";
    if (!sendSync({ first_insert_cmd }, 5000)) {
        std::cerr << "MoveBlendFirstInsert (parallel) failed" << std::endl;
        return false;
    }

    // 2. 发送 insert_line
    auto buildInsertLine = [](const ArmBlendLineParams& arm) -> std::string {
        if (!arm.is_run) return "NotRunExecute";
        std::string cmd = "MoveBlend --type=insert_line --robottarget_var=" + arm.target_name;
        if (arm.speed > 0) cmd += " --speed=v" + std::to_string(arm.speed);
        if (!arm.zone.empty()) {
            cmd += " --zone={" + std::to_string(arm.zone[0]) + "," + std::to_string(arm.zone[1]) + "}";
        }
        if (arm.ratio > 0) cmd += " --ratio=" + std::to_string(arm.ratio);
        return cmd;
        };

    std::string insert_line_cmd = "{" +
        buildInsertLine(arm1) + "||" +
        buildInsertLine(arm2) + "||" +
        buildInsertLine(arm3) + "}";
    if (!sendSync({ insert_line_cmd }, 5000)) {
        std::cerr << "MoveBlendInsertLine (parallel) failed" << std::endl;
        return false;
    }

    // 3. 发送 start
    std::string start_cmd = "{" +
        buildCommand(arm1, "MoveBlend --type=start") + "||" +
        buildCommand(arm2, "MoveBlend --type=start") + "||" +
        buildCommand(arm3, "MoveBlend --type=start") + "}";
    if (!sendSync({ start_cmd }, 60000)) {
        std::cerr << "MoveBlendStart (parallel) failed" << std::endl;
        return false;
    }

    std::cout << "All three arms MoveBlend sequences (parallel) completed" << std::endl;
    return true;
}

bool MultiArmCommand::EnableTriple(const ArmEnableParams& arm1,
    const ArmEnableParams& arm2,
    const ArmEnableParams& arm3)
{
    std::cout << "====== Sending EnableTriple command ======" << std::endl;
    auto buildCmd = [](const ArmEnableParams& arm) -> std::string {
        return arm.is_run ? "Enable" : "NotRunExecute";
        };
    std::string combined = "{" + buildCmd(arm1) + "||" +
        buildCmd(arm2) + "||" +
        buildCmd(arm3) + "}";
    return sendSync({ combined }, 5000);
}

bool MultiArmCommand::DisableTriple(const ArmEnableParams& arm1,
    const ArmEnableParams& arm2,
    const ArmEnableParams& arm3)
{
    std::cout << "====== Sending DisableTriple command ======" << std::endl;
    auto buildCmd = [](const ArmEnableParams& arm) -> std::string {
        return arm.is_run ? "Disable" : "NotRunExecute";
        };
    std::string combined = "{" + buildCmd(arm1) + "||" +
        buildCmd(arm2) + "||" +
        buildCmd(arm3) + "}";
    return sendSync({ combined }, 5000);
}

