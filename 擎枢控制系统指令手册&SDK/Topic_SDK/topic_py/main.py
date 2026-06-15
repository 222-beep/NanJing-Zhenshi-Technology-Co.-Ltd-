# main.py — 直接获取方式 (Direct Mode) 示例
# 演示 jointtarget / robottarget 获取，以及子系统 data 的用户端解析
#
# 每个 get_*() 函数内部自动获取快照，无需手动管理快照对象。
# 完整示例见 topic_sub_direct.py（全部字段）和 topic_sub_snapshot.py（快照方式）。

import time
import struct
from platform_loader import get_topic_module

# 获取平台相关的 topic 模块
topic = get_topic_module()

# ---- 导入直接获取方式的函数 ----
from system_state_reader import (
    has_rt_data, has_nrt_data,
    get_model_count_rt, get_model_name, get_model_type,
    get_current_robottarget, get_current_jointtarget,
    get_subsystem_count, get_subsystem_name, get_subsystem_state,
    get_subsystem_data_size, parse_subsystem_data,
    get_interface_count, get_interface_name, get_interface_state,
)

# ============================================================================
# 用户自定义结构体格式 —— 用于解析子系统的 data 字段
# 对应 C++ 中的 TwoFingerGripperYSStatus
# ============================================================================
TWO_FINGER_GRIPPER_YS_FORMAT = '<d'    # TwoFingerGripperYSStatus: actual_pos (double, 8 bytes)


def print_two_finger_ys_status(unpacked):
    """打印解析后的 TwoFingerGripperYSStatus"""
    print("    Parsed as TwoFingerGripperYSStatus:")
    print(f"      actual_pos={unpacked[0]}")


# ============================================================================
# 主程序
# ============================================================================
if __name__ == "__main__":
    # 启动订阅（传入发布者的 IP 地址，端口固定为 19091）
    PUBLISHER_IP = "192.168.11.11"   # 请修改为实际发布者 IP
    topic.start_subscriber(PUBLISHER_IP)
    print(f"Subscriber started, listening to {PUBLISHER_IP}:19091")
    print("Direct mode — jointtarget / robottarget\n")

    try:
        last_rt_time = 0
        last_nrt_time = 0
        while True:
            now = time.time()

            # ==================== 实时数据 (RT) ====================
            if has_rt_data() and now - last_rt_time >= 1.0:
                print("\n==================== JointTarget & RobotTarget (Direct) ====================")
                for m in range(get_model_count_rt()):
                    print(f"\n--- Model {m} : {get_model_name(m)} ({get_model_type(m)}) ---")
                    print(f"  robottarget : {get_current_robottarget(m)}")
                    print(f"  jointtarget : {get_current_jointtarget(m)}")
                print("=============================================================================\n")
                last_rt_time = now

            # ==================== 非实时数据 (NRT) ====================
            if has_nrt_data() and now - last_nrt_time >= 5.0:
                print("\n--- Subsystems ---")
                for i in range(get_subsystem_count()):
                    print(f"  {get_subsystem_name(i)} state={get_subsystem_state(i)} "
                          f"data_size={get_subsystem_data_size(i)}")

                    # 用户根据 data 大小判断是否匹配自己的结构体，直接做解析
                    if get_subsystem_data_size(i) >= struct.calcsize(TWO_FINGER_GRIPPER_YS_FORMAT):
                        try:
                            unpacked = parse_subsystem_data(i, TWO_FINGER_GRIPPER_YS_FORMAT)
                            print_two_finger_ys_status(unpacked)
                        except Exception as e:
                            print(f"    Parse failed: {e}")

                print("\n--- Interfaces ---")
                for i in range(get_interface_count()):
                    print(f"  {get_interface_name(i)} state={get_interface_state(i)}")
                print()

                last_nrt_time = now

            time.sleep(0.5)

    except KeyboardInterrupt:
        print("\nExited.")
