// main.cpp — 精简示例：jointtarget / robottarget + 子系统消息
//
// 使用直接获取方式 (Direct Mode)：
//   每个 get*() 函数内部自动获取快照，无需手动管理快照对象。
//   完整示例见 topic_sub_direct.cpp（全部字段）和 topic_sub_snapshot.cpp（快照方式）。

#include "system_state_reader.hpp"
#include <chrono>
#include <thread>
#include <cstring>

// ============================================================================
// 用户自定义结构体 —— 用于解析子系统的 data 字段
// 对应 Python 中的 TWO_FINGER_GRIPPER_YS_FORMAT = '<d'
// ============================================================================
#pragma pack(1)
struct TwoFingerGripperYSStatus {
    double actual_pos;   // 位置是否到位，0-未到位，1-到位
};
#pragma pack()

int main() {
    std::string remote_ip = "192.168.2.145";  // 请修改为实际发布者 IP
    start_subscriber(remote_ip);
    std::cout << "Subscriber started, listening to " << remote_ip << ":19091" << std::endl;
    std::cout << "Direct mode — jointtarget / robottarget\n" << std::endl;

    auto last_rt_time  = std::chrono::steady_clock::now();
    auto last_nrt_time = std::chrono::steady_clock::now();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto now = std::chrono::steady_clock::now();

        // ==================== 实时数据 (RT) ====================
        if (hasRtData() &&
            std::chrono::duration_cast<std::chrono::seconds>(now - last_rt_time).count() >= 1)
        {
            std::cout << "\n==================== JointTarget & RobotTarget (Direct) ====================\n";

            for (size_t m = 0; m < getModelCountRt(); ++m) {
                std::cout << "\n--- Model " << m << " : "
                          << getModelName(m) << " (" << getModelType(m) << ") ---\n";
                std::cout << "  robottarget : ";
                print_vector(getCurrentRobottarget(m));
                std::cout << "\n";
                std::cout << "  jointtarget : ";
                print_vector(getCurrentJointtarget(m));
                std::cout << "\n";
            }

            std::cout << "=============================================================================\n" << std::endl;
            last_rt_time = now;
        }

        // ==================== 非实时数据 (NRT) ====================
        if (hasNrtData() &&
            std::chrono::duration_cast<std::chrono::seconds>(now - last_nrt_time).count() >= 5)
        {
            std::cout << "\n--- Subsystems ---" << std::endl;
            for (size_t i = 0; i < getSubsystemCount(); ++i) {
                std::cout << "  " << getSubsystemName(i)
                          << " state=" << getSubsystemState(i)
                          << " data_size=" << getSubsystemDataSize(i) << std::endl;

                // 用户根据 data 大小判断是否匹配自己的结构体，直接做解析
                if (getSubsystemDataSize(i) >= sizeof(TwoFingerGripperYSStatus)) {
                    try {
                        TwoFingerGripperYSStatus gripper = parseSubsystemData<TwoFingerGripperYSStatus>(i);
                        std::cout << "    Parsed as TwoFingerGripperYSStatus:" << std::endl;
                        std::cout << "      actual_pos=" << gripper.actual_pos << std::endl;
                    } catch (const std::exception& e) {
                        std::cout << "    Parse failed: " << e.what() << std::endl;
                    }
                }
            }
            std::cout << std::endl;

            last_nrt_time = now;
        }
    }

    return 0;
}
