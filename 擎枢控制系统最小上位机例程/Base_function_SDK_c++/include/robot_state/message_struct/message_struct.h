#pragma pack(1)
struct FingerGripperStatus {
    int position_real;   // 实际位置
    int vol_duty;        // 控制输出驱动电机的电压占空比
    int speed_real;      // 实际速度
    int vol_real;        // 实际电压
    int current;         // 实际电流
    int temperature;     // 实际温度
};

struct TwoFingerGripperStatus {
    FingerGripperStatus f1_status;   // 手指1状态
    FingerGripperStatus f2_status;   // 手指2状态
    int sync_status;                 // 同步状态，0-非同步，1-同步
    int running_status;              // 运行状态，0-停止，1-运行
    int pos_ok;                      // 位置是否到位，0-未到位，1-到位
};

struct TwoFingerGripperYSStatus {    //usb-485因时夹爪已测试
    double actual_pos;
};