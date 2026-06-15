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
// 用户根据实际协议定义自己的 POD 结构体，然后通过 memcpy 解析 data
// ============================================================================
#pragma pack(push, 1)
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
#pragma pack(pop)

int main() {
    std::string remote_ip = "192.168.2.145";
    start_subscriber(remote_ip);

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // ==================== 实时数据 (RT) ====================
        auto rt = SystemStateReader::snapshotRt();
        if (rt.valid()) {
            std::cout << "\n==================== RT Data aa====================\n";

            // ---- 顶层字段 ----
            std::cout << "header_timestamp      : " << rt.headerTimestamp() << std::endl;
            std::cout << "header_frame_id       : " << rt.headerFrameId() << std::endl;
            std::cout << "system_running_state  : " << (rt.isSystemRunning() ? "true" : "false") << std::endl;
            std::cout << "system_info           : " << rt.systemInfo() << std::endl;

            // ---- 控制器 ----
            std::cout << "controller.controller_name : " << rt.controllerName() << std::endl;
            std::cout << "controller.control_cycle   : " << rt.controlCycle() << std::endl;
            std::cout << "controller.global_count    : " << rt.globalCount() << std::endl;
            std::cout << "controller.master_info     : " << rt.masterInfo() << std::endl;
            std::cout << "controller.is_link_up      : " << (rt.isLinkUp() ? "true" : "false") << std::endl;
            std::cout << "controller.ftvalues        : " << rt.ftValuesCount() << " sensors" << std::endl;
            for (size_t i = 0; i < rt.ftValuesCount(); ++i) {
                std::cout << "  ftvalues[" << i << "]: ";
                print_vector(rt.ftValue(i));
                std::cout << std::endl;
            }

            // ---- 模型数据 ----
            for (size_t m = 0; m < rt.modelCount(); ++m) {
                std::cout << "\n--- Model " << m << " : " << rt.modelName(m)
                          << " (" << rt.modelType(m) << ") ---" << std::endl;
                std::cout << "  model_name           : " << rt.modelName(m) << std::endl;
                std::cout << "  model_type           : " << rt.modelType(m) << std::endl;
                std::cout << "  joint_count          : " << rt.jointCount(m) << std::endl;

                // 关节数据
                for (size_t j = 0; j < rt.jointCount(m); ++j) {
                    std::cout << "    Joint " << j << " :" << std::endl;
                    std::cout << "      joint_type      : " << rt.jointType(m, j) << std::endl;
                    std::cout << "      position        : " << rt.jointPosition(m, j) << std::endl;
                    std::cout << "      torque          : " << rt.jointTorque(m, j) << std::endl;
                    std::cout << "      is_enabled      : " << (rt.jointIsEnabled(m, j) ? "true" : "false") << std::endl;
                    std::cout << "      mode            : " << rt.jointMode(m, j) << std::endl;
                    std::cout << "      error_code      : " << rt.jointErrorCode(m, j) << std::endl;
                    std::cout << "      digit_output    : " << rt.jointDigitOutput(m, j) << std::endl;
                    std::cout << "      digit_input     : " << rt.jointDigitInput(m, j) << std::endl;
                    std::cout << "      sensor_torque   : " << rt.jointSensorTorque(m, j) << std::endl;
                    std::cout << "      velocity        : " << rt.jointVelocity(m, j) << std::endl;
                    std::cout << "      target_position : " << rt.jointTargetPosition(m, j) << std::endl;
                }

                // 当前点信息
                if (rt.hasCurrentPoint(m)) {
                    std::cout << "  Current Point :" << std::endl;
                    std::cout << "    point_name        : " << rt.currentPointName(m) << std::endl;
                    std::cout << "    tool_name         : " << rt.currentToolName(m) << std::endl;
                    std::cout << "    wobj_name         : " << rt.currentWobjName(m) << std::endl;
                    std::cout << "    tool_data         : "; print_vector(rt.currentToolData(m)); std::cout << std::endl;
                    std::cout << "    wobj_data         : "; print_vector(rt.currentWobjData(m)); std::cout << std::endl;
                    std::cout << "    robottarget       : "; print_vector(rt.currentRobottarget(m)); std::cout << std::endl;
                    std::cout << "    jointtarget       : "; print_vector(rt.currentJointtarget(m)); std::cout << std::endl;
                }

                // 模型运行状态信息
                std::cout << "  Model Info :" << std::endl;
                std::cout << "    error_code          : " << rt.modelErrorCode(m) << std::endl;
                std::cout << "    error_msg           : " << rt.modelErrorMsg(m) << std::endl;
                std::cout << "    model_state         : " << rt.modelState(m) << std::endl;
                std::cout << "    model_time_rate     : " << rt.modelTimeRate(m) << std::endl;
                std::cout << "    current_func_name   : " << rt.modelCurrentFuncName(m) << std::endl;
                std::cout << "    current_func_info   : " << rt.modelCurrentFuncInfo(m) << std::endl;
                std::cout << "    func_count          : " << rt.modelFuncCount(m) << std::endl;
                std::cout << "    info_msg            : " << rt.modelInfoMsg(m) << std::endl;
                std::cout << "    ee_pe321            : "; print_vector(rt.modelEePe321(m)); std::cout << std::endl;
            }
        } else {
            std::cout << "No RT data yet." << std::endl;
        }

        // ==================== 非实时数据 (NRT) ====================
        auto nrt = SystemStateReader::snapshotNrt();
        if (nrt.valid()) {
            std::cout << "\n==================== NRT Data ====================\n";

            // ---- 顶层字段 ----
            std::cout << "header_timestamp      : " << nrt.headerTimestamp() << std::endl;
            std::cout << "header_frame_id       : " << nrt.headerFrameId() << std::endl;
            std::cout << "system_running_state  : " << (nrt.isSystemRunning() ? "true" : "false") << std::endl;
            std::cout << "system_is_init        : " << (nrt.isSystemInit() ? "true" : "false") << std::endl;

            // ---- 从站 ----
            std::cout << "controller.slaves     : " << nrt.slaveCount() << " slaves" << std::endl;
            for (size_t i = 0; i < nrt.slaveCount(); ++i) {
                std::cout << "  slave[" << i << "] : " << nrt.slaveName(i)
                          << " phy_id=" << nrt.slavePhyId(i)
                          << " alias=" << nrt.slaveAlias(i)
                          << " state=" << nrt.slaveState(i)
                          << " online=" << nrt.slaveIsOnline(i)
                          << " virtual=" << nrt.slaveIsVirtual(i)
                          << " error=" << nrt.slaveIsError(i) << std::endl;
            }

            // ---- 模型数据 ----
            for (size_t m = 0; m < nrt.modelCount(); ++m) {
                std::cout << "\n--- Model " << m << " : " << nrt.modelName(m)
                          << " (" << nrt.modelType(m) << ") ---" << std::endl;
                std::cout << "  model_name           : " << nrt.modelName(m) << std::endl;
                std::cout << "  model_type           : " << nrt.modelType(m) << std::endl;
                std::cout << "  is_using_sp          : " << (nrt.isModelUsingSP(m) ? "true" : "false") << std::endl;
                std::cout << "  is_collision_detection: " << (nrt.isModelCollisionDetection(m) ? "true" : "false") << std::endl;
                std::cout << "  take_photo           : " << nrt.modelTakePhoto(m) << std::endl;
                std::cout << "  joint_count          : " << nrt.jointCount(m) << std::endl;

                // 关节限制数据
                for (size_t j = 0; j < nrt.jointCount(m); ++j) {
                    std::cout << "    Joint " << j << " limits:" << std::endl;
                    std::cout << "      max_position          : " << nrt.jointMaxPosition(m, j) << std::endl;
                    std::cout << "      min_position          : " << nrt.jointMinPosition(m, j) << std::endl;
                    std::cout << "      max_vel               : " << nrt.jointMaxVel(m, j) << std::endl;
                    std::cout << "      min_vel               : " << nrt.jointMinVel(m, j) << std::endl;
                    std::cout << "      max_acc               : " << nrt.jointMaxAcc(m, j) << std::endl;
                    std::cout << "      min_acc               : " << nrt.jointMinAcc(m, j) << std::endl;
                    std::cout << "      max_collision_torque  : " << nrt.jointMaxCollisionTorque(m, j) << std::endl;
                }

                // 工具
                std::cout << "  Tools (" << nrt.toolCount(m) << "):" << std::endl;
                for (size_t i = 0; i < nrt.toolCount(m); ++i) {
                    std::cout << "    tool[" << i << "] : " << nrt.toolName(m, i) << " data=";
                    print_vector(nrt.toolData(m, i));
                    std::cout << std::endl;
                }

                // 工件
                std::cout << "  Wobjs (" << nrt.wobjCount(m) << "):" << std::endl;
                for (size_t i = 0; i < nrt.wobjCount(m); ++i) {
                    std::cout << "    wobj[" << i << "] : " << nrt.wobjName(m, i) << " data=";
                    print_vector(nrt.wobjData(m, i));
                    std::cout << std::endl;
                }

                // 负载
                std::cout << "  Loads (" << nrt.loadCount(m) << "):" << std::endl;
                for (size_t i = 0; i < nrt.loadCount(m); ++i) {
                    std::cout << "    load[" << i << "] : " << nrt.loadName(m, i) << " data=";
                    print_vector(nrt.loadData(m, i));
                    std::cout << std::endl;
                }

                // 示教点
                std::cout << "  Teach points (" << nrt.teachPointCount(m) << "):" << std::endl;
                for (size_t i = 0; i < nrt.teachPointCount(m); ++i) {
                    std::cout << "    point[" << i << "] : " << nrt.teachPointName(m, i)
                              << " tool=" << nrt.teachPointToolName(m, i)
                              << " wobj=" << nrt.teachPointWobjName(m, i) << std::endl;
                    std::cout << "      tool_data="; print_vector(nrt.teachPointToolData(m, i)); std::cout << std::endl;
                    std::cout << "      wobj_data="; print_vector(nrt.teachPointWobjData(m, i)); std::cout << std::endl;
                    std::cout << "      robottarget="; print_vector(nrt.teachPointRobottarget(m, i)); std::cout << std::endl;
                    std::cout << "      jointtarget="; print_vector(nrt.teachPointJointtarget(m, i)); std::cout << std::endl;
                }

                // IO（按模型索引）
                std::cout << "  IO (" << nrt.ioCount(m) << "):" << std::endl;
                for (size_t i = 0; i < nrt.ioCount(m); ++i) {
                    std::cout << "    io[" << i << "] : " << nrt.ioName(m, i)
                              << " data=" << nrt.ioData(m, i) << std::endl;
                }
            }

            // 子系统
            std::cout << "\n--- Subsystems (" << nrt.subsystemCount() << ") ---" << std::endl;
            for (size_t i = 0; i < nrt.subsystemCount(); ++i) {
                std::cout << "  " << nrt.subsystemName(i) << " id=" << nrt.subsystemId(i)
                          << " state=" << nrt.subsystemState(i)
                          << " data size=" << nrt.subsystemDataSize(i) << std::endl;

                // 用户根据 data 大小判断是否匹配自己的结构体，直接传入结构体做解析
                if (nrt.subsystemDataSize(i) >= sizeof(TwoFingerGripperStatus)) {
                    TwoFingerGripperStatus gripper;
                    std::memcpy(&gripper, nrt.subsystemRawData(i).data(), sizeof(TwoFingerGripperStatus));
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
            std::cout << "\n--- Interfaces (" << nrt.interfaceCount() << ") ---" << std::endl;
            for (size_t i = 0; i < nrt.interfaceCount(); ++i) {
                std::cout << "  " << nrt.interfaceName(i) << " id=" << nrt.interfaceId(i)
                          << " state=" << nrt.interfaceState(i) << std::endl;
            }
        } else {
            std::cout << "No NRT data yet." << std::endl;
        }

        std::cout << "\n==================================================\n" << std::endl;
    }

    return 0;
}
