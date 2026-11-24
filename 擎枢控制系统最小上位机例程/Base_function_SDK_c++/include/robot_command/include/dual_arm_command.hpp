#ifndef DUAL_ARM_COMMAND_HPP
#define DUAL_ARM_COMMAND_HPP

#include "command.hpp"

// 双臂绝对关节运动参数
struct DualArmMoveAbsjParams {
    ArmMoveAbsjParams arm1;
    ArmMoveAbsjParams arm2;
};

// 双臂关节点动参数
struct DualArmJogJointParams {
    ArmJogJointParams arm1;
    ArmJogJointParams arm2;
};

// 双臂笛卡尔点动参数
struct DualArmJogCartesianParams {
    ArmJogCartesianParams arm1;
    ArmJogCartesianParams arm2;
};

// 双臂 MoveBlend 直线序列参数
struct DualArmBlendLineParams {
    ArmBlendLineParams arm1;
    ArmBlendLineParams arm2;
};

//// 双臂获取目标点参数
//struct DualPointChooseIDMoveParams {
//    ArmPointChooseIDMoveParams arm1;
//    ArmPointChooseIDMoveParams arm2;
//};

// 双臂点位偏移保存参数
struct DualPointFixSaveParams {
    ArmPointFixSaveParams arm1;
    ArmPointFixSaveParams arm2;
};

// 双臂平面校准参数
struct DualPointFitPlaneParams {
    ArmPointFitPlaneParams arm1;
    ArmPointFitPlaneParams arm2;
};

class DualArmCommand : public Command {
public:
    using Command::Command;   // 继承构造函数

    // 双臂绝对关节运动
    bool MoveAbsjDual(const DualArmMoveAbsjParams& params);
    // 或分开传参，视喜好
    // bool MoveAbsjDual(const ArmMoveAbsjParams& arm1, const ArmMoveAbsjParams& arm2);

    // 双臂关节点动
    bool JogJointDual(const DualArmJogJointParams& params); //异步
    bool abc(const DualArmJogJointParams& params);          //同步

    // 双臂笛卡尔点动
    bool JogCartesianDual(const DualArmJogCartesianParams& params);

    // 双臂 MoveBlend 
    bool MoveBlendLineDual(const DualArmBlendLineParams& params);

    //// 精度测试寻点
    //bool PointChooseIDMoveDual(const DualPointChooseIDMoveParams& params);

    // 点位偏移
    bool PointFixSaveDual(const DualPointFixSaveParams& params);

    // 平面校准
    bool PointFitPlaneDual(const DualPointFitPlaneParams& params);
};

#endif