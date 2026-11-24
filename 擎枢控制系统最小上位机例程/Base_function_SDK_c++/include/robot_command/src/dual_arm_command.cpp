#include "dual_arm_command.hpp"
#include <iostream>

bool DualArmCommand::MoveAbsjDual(const DualArmMoveAbsjParams& params) {
    std::cout << "====== Sending MoveAbsjDual command ======" << std::endl;
    auto buildCmd = [](const ArmMoveAbsjParams& arm) -> std::string {
        if (arm.is_run) {
            return "MoveAbsJ --jointtarget_var=" + arm.target_name +
                " --speed=v" + std::to_string(arm.speed);
        }
        return "NotRunExecute";
        };
    std::string combined = "{" + buildCmd(params.arm1) + "||" +
        buildCmd(params.arm2) + "}";
    return sendSync({ combined }, 60000);
}

bool DualArmCommand::JogJointDual(const DualArmJogJointParams& params) {
    std::cout << "====== Sending JogJointDual command ======" << std::endl;
    auto buildCmd = [](const ArmJogJointParams& arm) -> std::string {
        if (arm.is_run) {
            return "JogJoint --speed=v" + std::to_string(arm.speed) +
                " --motion_id=" + arm.motion_id +
                " --increase_count=50"  +
                " --direction=" + std::to_string(arm.direction);
        }
        return "NotRunExecute";
        };
    std::string combined = "{" + buildCmd(params.arm1) + "||" +
        buildCmd(params.arm2) + "}";
    return sendAsync({ combined }, 5000, 90);
}

bool DualArmCommand::abc(const DualArmJogJointParams& params) {
    std::cout << "====== Sending JogJointDual command ======" << std::endl;
    auto buildCmd = [](const ArmJogJointParams& arm) -> std::string {
        if (arm.is_run) {
            return "JogJoint --speed=v" + std::to_string(arm.speed) +
                " --motion_id=" + arm.motion_id +
                " --increase_count=50" +
                " --direction=" + std::to_string(arm.direction);
        }
        return "NotRunExecute";
        };
    std::string combined = "{" + buildCmd(params.arm1) + "||" +
        buildCmd(params.arm2) + "}";
    return sendSync({ combined }, 5000);
}

bool DualArmCommand::JogCartesianDual(const DualArmJogCartesianParams& params) {
    std::cout << "====== Sending JogCartesianDual command ======" << std::endl;
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
    std::string combined = "{" + buildCmd(params.arm1) + "||" +
        buildCmd(params.arm2) + "}";
    return sendAsync({ combined }, 5000, 90);
}

bool DualArmCommand::MoveBlendLineDual(const DualArmBlendLineParams& params) {
    std::cout << "====== Executing MoveBlend sequence for two arms (parallel) ======" << std::endl;

    auto buildCommand = [](const ArmBlendLineParams& arm, const std::string& cmd) -> std::string {
        return arm.is_run ? cmd : "NotRunExecute";
        };

    // 1. first_insert
    std::string first_insert_cmd = "{" +
        buildCommand(params.arm1, "MoveBlend --type=first_insert") + "||" +
        buildCommand(params.arm2, "MoveBlend --type=first_insert") + "}";
    if (!sendSync({ first_insert_cmd }, 5000)) {
        std::cerr << "MoveBlendFirstInsert (dual) failed" << std::endl;
        return false;
    }

    // 2. insert_line
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
        buildInsertLine(params.arm1) + "||" +
        buildInsertLine(params.arm2) + "}";
    if (!sendSync({ insert_line_cmd }, 5000)) {
        std::cerr << "MoveBlendInsertLine (dual) failed" << std::endl;
        return false;
    }

    // 3. start
    std::string start_cmd = "{" +
        buildCommand(params.arm1, "MoveBlend --type=start") + "||" +
        buildCommand(params.arm2, "MoveBlend --type=start") + "}";
    if (!sendSync({ start_cmd }, 60000)) {
        std::cerr << "MoveBlendStart (dual) failed" << std::endl;
        return false;
    }

    std::cout << "Both arms MoveBlend sequences completed" << std::endl;
    return true;
}

//bool DualArmCommand::PointChooseIDMoveDual(const DualPointChooseIDMoveParams& params) {
//    std::cout << "====== Sending PointChooseIDMoveDual (dual arms) ======" << std::endl;
//
//    auto buildCmd = [](const ArmPointChooseIDMoveParams& arm) -> std::string {
//        if (!arm.is_run) return "NotRunExecute";
//        std::string cmd = "PointChooseIDMove";
//        cmd += " --mid_point_robottarget=" + arm.mid_point_robottarget;
//        cmd += " --point_id=" + std::to_string(arm.point_id);
//        cmd += " --target_x=" + std::to_string(arm.target_x);
//        cmd += " --target_y=" + std::to_string(arm.target_y);
//        cmd += " --len_end=" + std::to_string(arm.len_end);
//        cmd += " --len_point=" + std::to_string(arm.len_point);
//        cmd += " --cal_on=" + std::to_string(arm.cal_on);
//        return cmd;
//        };
//
    // 双臂格式：只有两个部分，以 || 分隔
//    std::string combined = "{" + buildCmd(params.arm1) + "||" + buildCmd(params.arm2) + "}";
//    return sendSync({ combined }, 10000);
//}

bool DualArmCommand::PointFixSaveDual(const DualPointFixSaveParams& params) {
    std::cout << "====== Sending PointFixSaveDual (dual arms) ======" << std::endl;

    auto buildCmd = [](const ArmPointFixSaveParams& arm) -> std::string {
        if (!arm.is_run) return "NotRunExecute";
        std::string cmd = "PointFixSave";
        cmd += " --point_id=" + std::to_string(arm.point_id);
        cmd += " --dx=" + std::to_string(arm.dx);
        cmd += " --dy=" + std::to_string(arm.dy);
        cmd += " --mode=" + arm.mode;
        return cmd;
        };

    std::string combined = "{" + buildCmd(params.arm1) + "||" + buildCmd(params.arm2) + "}";
    return sendSync({ combined }, 10000);
}

bool DualArmCommand::PointFitPlaneDual(const DualPointFitPlaneParams& params) {
    std::cout << "====== Sending PointFitPlaneDual (dual arms) ======" << std::endl;

    auto buildCmd = [](const ArmPointFitPlaneParams& arm) -> std::string {
        if (!arm.is_run) return "NotRunExecute";
        return "PointFitPlane --len_point=" + std::to_string(arm.len_point);
        };

    std::string combined = "{" + buildCmd(params.arm1) + "||" + buildCmd(params.arm2) + "}";
    return sendSync({ combined }, 10000);
}
