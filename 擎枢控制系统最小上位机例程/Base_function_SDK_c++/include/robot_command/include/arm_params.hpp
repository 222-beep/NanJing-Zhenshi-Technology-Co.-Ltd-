#ifndef ARM_PARAMS_HPP
#define ARM_PARAMS_HPP

#include <string>
#include <vector>

/**
 * @brief 绝对关节运动参数
 */
struct ArmMoveAbsjParams {
    std::string target_name;   // 目标点名称
    int speed = 0;             // 运动速度档位
    bool is_run = false;       // true-运动，false-不运动
};

/**
 * @brief 关节空间点动参数
 */
struct ArmJogJointParams {
    std::string motion_id;     // 电机ID
    int speed = 0;             // 运动速度档位
    int direction = 0;         // 运动方向：1-正，-1-负
    bool is_run = false;       // true-运动，false-不运动
};

/**
 * @brief 笛卡尔点动参数
 */
struct ArmJogCartesianParams {
    int moving_type = 0;       // 运动方式 0~5（0~2 平动，3~5 旋转）
    int speed = 0;             // 运动速度档位
    int direction = 0;         // 运动方向：1-正，-1-负
    int coordinate = 0;        // 参考坐标系：0-世界，1-工具
    bool is_run = false;       // true-运动，false-不运动
};

/**
 * @brief MoveBlend 直线序列参数
 */
struct ArmBlendLineParams {
    std::string target_name;            // 目标点名称
    int speed = 0;                      // 运动速度档位
    std::vector<double> zone;           // 平滑参数 [笛卡尔转弯区大小(mm), 百分比]
    double ratio = 0.0;                 // 加速度/速度比率，范围 1~10
    bool is_run = false;                // true-运动，false-不运动
};

/**
 * @brief 点位偏移保存参数
 */
struct ArmPointFixSaveParams {
    int point_id = 0;                   // 点号ID
    double dx = 0.0;                    // X方向偏移量
    double dy = 0.0;                    // Y方向偏移量
    std::string mode = "inc";           // 补偿更新模式：inc(增量) / set(覆盖)
    bool is_run = false;                // 是否执行
};

/**
 * @brief 平面校准参数
 */
struct ArmPointFitPlaneParams {
    double len_point = 10.0;            // 点阵间距
    bool is_run = false;                // 是否执行
};

/**
 * @brief 使能/下使能参数
 */
struct ArmEnableParams {
    bool is_run = false;    // true-执行，false-不执行
};

/**
 * @brief 点阵标定参数 (PointChooseIDMove1)
 */
struct ArmPointChooseIDMove1Params {
    std::string mid_point_robottarget;  // 中心点示教变量名
    int point_id = -1;                  // 目标点ID (1~25)
    double len_end = 89.0;              // 工具末端到点阵的距离
    double len_point = 10.0;            // 点阵间隔
    int cal_on = 1;                     // 补偿模式: 0无补偿,1硬补偿,2平面补偿
    bool is_run = false;                // 是否执行
};

/**
 * @brief 点阵选择参数 (PointChooseIDMove2)
 */
struct ArmPointChooseIDMove2Params {
    std::string mid_point_robottarget;  // 中心点示教变量名
    double target_x = 0.0;              // 目标点X坐标
    double target_y = 0.0;              // 目标点Y坐标
    double target_z = 0.0;              // 目标点Z坐标
    double len_end = 89.0;              // 工具末端到点阵的距离
    double len_point = 10.0;            // 点阵间隔
    int cal_on = 2;                     // 补偿模式: 0无补偿,1硬补偿,2平面补偿
    bool is_run = false;                // 是否执行
};

#endif // ARM_PARAMS_HPP
