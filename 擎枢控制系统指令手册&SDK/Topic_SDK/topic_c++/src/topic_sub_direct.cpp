#include "system_state_reader.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

// 辅助函数：打印 vector<double> 的内容，限制长度
void print_vector(const std::vector<double>& vec, size_t max_print = 6) {
    std::cout << "[";
    for (size_t i = 0; i < vec.size() && i < max_print; ++i) {
        std::cout << vec[i] << (i + 1 < vec.size() ? ", " : "");
    }
    if (vec.size() > max_print) std::cout << "...";
    std::cout << "]";
}


// ============================================================================
// 用户自定义结构体示例 —— 用于解析子系统的 data 字段
// 用户根据实际协议定义自己的 POD 结构体，然后调用 parseSubsystemData<T>(idx)
// ============================================================================
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
#pragma pack()

int main() {
    std::string remote_ip = "192.168.2.216";
    start_subscriber(remote_ip);

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // ==================== 实时数据 (RT) ====================
        if (hasRtData()) {
            std::cout << "\n==================== RT Data ====================\n";

            // ---- 顶层字段 ----
            std::cout << "header_timestamp      : " << getHeaderTimestamp() << std::endl;
            std::cout << "header_frame_id       : " << getHeaderFrameId() << std::endl;
            std::cout << "system_running_state  : " << (isSystemRunning() ? "true" : "false") << std::endl;
            std::cout << "system_info           : " << getSystemInfo() << std::endl;

            // ---- 控制器 ----
            std::cout << "controller.controller_name : " << getControllerName() << std::endl;
            std::cout << "controller.control_cycle   : " << getControlCycle() << std::endl;
            std::cout << "controller.global_count    : " << getGlobalCount() << std::endl;
            std::cout << "controller.master_info     : " << getMasterInfo() << std::endl;
            std::cout << "controller.is_link_up      : " << (isLinkUp() ? "true" : "false") << std::endl;
            std::cout << "controller.ftvalues        : " << getFtValuesCount() << " sensors" << std::endl;
            for (size_t i = 0; i < getFtValuesCount(); ++i) {
                std::cout << "  ftvalues[" << i << "]: ";
                print_vector(getFtValue(i));
                std::cout << std::endl;
            }

            // ---- 模型数据 ----
            for (size_t m = 0; m < getModelCountRt(); ++m) {
                std::cout << "\n--- Model " << m << " : " << getModelName(m)
                          << " (" << getModelType(m) << ") ---" << std::endl;
                std::cout << "  model_name           : " << getModelName(m) << std::endl;
                std::cout << "  model_type           : " << getModelType(m) << std::endl;
                std::cout << "  joint_count          : " << getJointCount(m) << std::endl;

                // 关节数据
                for (size_t j = 0; j < getJointCount(m); ++j) {
                    std::cout << "    Joint " << j << " :" << std::endl;
                    std::cout << "      joint_type      : " << getJointType(m, j) << std::endl;
                    std::cout << "      position        : " << getJointPosition(m, j) << std::endl;
                    std::cout << "      torque          : " << getJointTorque(m, j) << std::endl;
                    std::cout << "      is_enabled      : " << (getJointIsEnabled(m, j) ? "true" : "false") << std::endl;
                    std::cout << "      mode            : " << getJointMode(m, j) << std::endl;
                    std::cout << "      error_code      : " << getJointErrorCode(m, j) << std::endl;
                    std::cout << "      digit_output    : " << getJointDigitOutput(m, j) << std::endl;
                    std::cout << "      digit_input     : " << getJointDigitInput(m, j) << std::endl;
                    std::cout << "      sensor_torque   : " << getJointSensorTorque(m, j) << std::endl;
                    std::cout << "      velocity        : " << getJointVelocity(m, j) << std::endl;
                    std::cout << "      target_position : " << getJointTargetPosition(m, j) << std::endl;
                }

                // 当前点信息
                std::cout << "  Current Point :" << std::endl;
                std::cout << "    point_name        : " << getCurrentPointName(m) << std::endl;
                std::cout << "    tool_name         : " << getCurrentToolName(m) << std::endl;
                std::cout << "    wobj_name         : " << getCurrentWobjName(m) << std::endl;
                std::cout << "    robottarget       : "; print_vector(getCurrentRobottarget(m)); std::cout << std::endl;
                std::cout << "    jointtarget       : "; print_vector(getCurrentJointtarget(m)); std::cout << std::endl;

                // 模型运行状态信息
                std::cout << "  Model Info :" << std::endl;
                std::cout << "    error_code          : " << getModelErrorCode(m) << std::endl;
                std::cout << "    error_msg           : " << getModelErrorMsg(m) << std::endl;
                std::cout << "    model_state         : " << getModelState(m) << std::endl;
                std::cout << "    model_time_rate     : " << getModelTimeRate(m) << std::endl;
                std::cout << "    current_func_name   : " << getModelCurrentFuncName(m) << std::endl;
                std::cout << "    ee_pe321            : "; print_vector(getModelEePe321(m)); std::cout << std::endl;
            }
        } else {
            std::cout << "No RT data yet." << std::endl;
        }

        // ==================== 非实时数据 (NRT) ====================
        if (hasNrtData()) {
            std::cout << "\n==================== NRT Data ====================\n";

            // ---- 顶层字段 ----
            std::cout << "header_timestamp      : " << getHeaderTimestamp() << std::endl;
            std::cout << "header_frame_id       : " << getHeaderFrameId() << std::endl;
            std::cout << "system_running_state  : " << (isSystemRunning() ? "true" : "false") << std::endl;
            std::cout << "system_is_init        : " << (isSystemInit() ? "true" : "false") << std::endl;

            // ---- 从站 ----
            std::cout << "controller.slaves     : " << getSlaveCount() << " slaves" << std::endl;
            for (size_t i = 0; i < getSlaveCount(); ++i) {
                std::cout << "  slave[" << i << "] : " << getSlaveName(i)
                          << " state=" << getSlaveState(i)
                          << " online=" << getSlaveIsOnline(i) << std::endl;
            }

            // ---- 模型数据 ----
            for (size_t m = 0; m < getModelCountNrt(); ++m) {
                std::cout << "\n--- Model " << m << " : " << getModelName(m)
                          << " (" << getModelType(m) << ") ---" << std::endl;
                std::cout << "  model_name           : " << getModelName(m) << std::endl;
                std::cout << "  model_type           : " << getModelType(m) << std::endl;
                std::cout << "  is_using_sp          : " << (isModelUsingSP(m) ? "true" : "false") << std::endl;
                std::cout << "  is_collision_detection: " << (isModelCollisionDetection(m) ? "true" : "false") << std::endl;
                std::cout << "  take_photo           : " << getModelTakePhoto(m) << std::endl;
                std::cout << "  joint_count          : " << getJointCount(m) << std::endl;

                // 关节限制数据
                for (size_t j = 0; j < getJointCount(m); ++j) {
                    std::cout << "    Joint " << j << " limits:" << std::endl;
                    std::cout << "      max_position          : " << getJointMaxPosition(m, j) << std::endl;
                    std::cout << "      min_position          : " << getJointMinPosition(m, j) << std::endl;
                    std::cout << "      max_vel               : " << getJointMaxVel(m, j) << std::endl;
                    std::cout << "      min_vel               : " << getJointMinVel(m, j) << std::endl;
                    std::cout << "      max_acc               : " << getJointMaxAcc(m, j) << std::endl;
                    std::cout << "      min_acc               : " << getJointMinAcc(m, j) << std::endl;
                    std::cout << "      max_collision_torque  : " << getJointMaxCollisionTorque(m, j) << std::endl;
                }
            }

            // 子系统
            std::cout << "\n--- Subsystems (" << getSubsystemCount() << ") ---" << std::endl;
            for (size_t i = 0; i < getSubsystemCount(); ++i) {
                std::cout << "  " << getSubsystemName(i) << " id=" << getSubsystemState(i)
                          << " data size=" << getSubsystemDataSize(i) << std::endl;

                // 用户根据 data 大小判断是否匹配自己的结构体，直接传入结构体做解析
                if (getSubsystemDataSize(i) >= sizeof(TwoFingerGripperStatus)) {
                    auto nrt_snap = SystemStateReader::snapshotNrt();
                    TwoFingerGripperStatus gripper;
                    std::memcpy(&gripper, nrt_snap.subsystemRawData(i).data(), sizeof(TwoFingerGripperStatus));
                    std::cout << "    Parsed as TwoFingerGripperStatus:" << std::endl;
                    std::cout << "      Finger1: pos_real=" << gripper.f1_status.position_real
                              << " vol_duty=" << gripper.f1_status.vol_duty
                              << " speed_real=" << gripper.f1_status.speed_real
                              << " vol_real=" << gripper.f1_status.vol_real
                              << " current=" << gripper.f1_status.current
                              << " temperature=" << gripper.f1_status.temperature << std::endl;
                    std::cout << "      Finger2: pos_real=" << gripper.f2_status.position_real
                              << " vol_duty=" << gripper.f2_status.vol_duty
                              << " speed_real=" << gripper.f2_status.speed_real
                              << " vol_real=" << gripper.f2_status.vol_real
                              << " current=" << gripper.f2_status.current
                              << " temperature=" << gripper.f2_status.temperature << std::endl;
                    std::cout << "      sync_status=" << gripper.sync_status
                              << " running_status=" << gripper.running_status
                              << " pos_ok=" << gripper.pos_ok << std::endl;
                }
            }

            // 接口
            std::cout << "\n--- Interfaces (" << getInterfaceCount() << ") ---" << std::endl;
            for (size_t i = 0; i < getInterfaceCount(); ++i) {
                std::cout << "  " << getInterfaceName(i) << " state=" << getInterfaceState(i) << std::endl;
            }
        } else {
            std::cout << "No NRT data yet." << std::endl;
        }

        std::cout << "\n==================================================\n" << std::endl;
    }

    return 0;
}
