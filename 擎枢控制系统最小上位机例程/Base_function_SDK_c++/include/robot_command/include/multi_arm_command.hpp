#ifndef COMMAND_MULTI_ARM_HPP
#define COMMAND_MULTI_ARM_HPP

#include "command.hpp"

// 三臂命令专用类（继承自 Command，复用连接和发送能力）
class MultiArmCommand : public Command {
public:
    using Command::Command;   // 继承构造函数

    // 三臂绝对关节运动
    bool MoveAbsjTriple(const ArmMoveAbsjParams& arm1,
        const ArmMoveAbsjParams& arm2,
        const ArmMoveAbsjParams& arm3);

    // 三臂关节点动
    bool JogJointTriple(const ArmJogJointParams& arm1,
        const ArmJogJointParams& arm2,
        const ArmJogJointParams& arm3);

    // 三臂笛卡尔点动
    bool JogCartesianTriple(const ArmJogCartesianParams& arm1,
        const ArmJogCartesianParams& arm2,
        const ArmJogCartesianParams& arm3);

    // 三臂 MoveBlend 直线序列
    bool MoveBlendLineTriple(const ArmBlendLineParams& arm1,
        const ArmBlendLineParams& arm2,
        const ArmBlendLineParams& arm3);

    // 三臂使能
    bool EnableTriple(const ArmEnableParams& arm1,
        const ArmEnableParams& arm2,
        const ArmEnableParams& arm3);

    // 三臂下使能
    bool DisableTriple(const ArmEnableParams& arm1,
        const ArmEnableParams& arm2,
        const ArmEnableParams& arm3);

    // 点阵标定
    bool PointChooseIDMove1Triple(const ArmPointChooseIDMove1Params& arm1,
        const ArmPointChooseIDMove1Params& arm2,
        const ArmPointChooseIDMove1Params& arm3,
        std::vector<double>& output_pq);

    // 补偿保存
    bool PointFixSaveTriple(const ArmPointFixSaveParams& arm1,
        const ArmPointFixSaveParams& arm2,
        const ArmPointFixSaveParams& arm3);

    // 平面拟合
    bool PointFitPlaneTriple(const ArmPointFitPlaneParams& arm1,
        const ArmPointFitPlaneParams& arm2,
        const ArmPointFitPlaneParams& arm3);

    // 点阵选择
    bool PointChooseIDMove2Triple(const ArmPointChooseIDMove2Params& arm1,
        const ArmPointChooseIDMove2Params& arm2,
        const ArmPointChooseIDMove2Params& arm3);
};

#endif