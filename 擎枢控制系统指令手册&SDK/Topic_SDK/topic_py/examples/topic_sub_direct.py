# topic_sub_direct.py — 直接获取方式 (Direct Mode) 示例
# 对应 C++ 的 topic_sub_direct.cpp
#
# 使用方式：每个 get_*() 函数内部自动获取快照，无需手动管理快照对象。
# 适合单字段快速访问；批量读取推荐使用 topic_sub_snapshot.py 的快照方式。
#
# 另见 topic_sub_snapshot.py — 快照方式 (Snapshot Mode) 示例。

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import struct

from system_state_reader import (
    has_rt_data, has_nrt_data,
    get_header_timestamp, get_header_frame_id, is_system_running, get_system_info, is_system_init,
    get_controller_name, get_control_cycle, get_global_count, get_master_info, is_link_up,
    get_ftvalues_count, get_ftvalue,
    get_model_count_rt, get_model_count_nrt, get_model_name, get_model_type, get_joint_count,
    get_joint_type, get_joint_position, get_joint_torque, get_joint_is_enabled,
    get_joint_mode, get_joint_error_code, get_joint_digit_output, get_joint_digit_input,
    get_joint_sensor_torque, get_joint_velocity, get_joint_target_position,
    get_joint_max_position, get_joint_min_position, get_joint_max_vel, get_joint_min_vel,
    get_joint_max_acc, get_joint_min_acc, get_joint_max_collision_torque,
    is_model_using_sp, is_model_collision_detection, get_model_take_photo,
    get_current_point_name, get_current_tool_name, get_current_wobj_name,
    get_current_robottarget, get_current_jointtarget,
    get_model_error_code, get_model_error_msg, get_model_state, get_model_time_rate,
    get_model_current_func_name, get_model_ee_pe321,
    get_slave_count, get_slave_name, get_slave_state, get_slave_is_online,
    get_subsystem_count, get_subsystem_name, get_subsystem_state,
    get_subsystem_data_size, parse_subsystem_data,
    get_interface_count, get_interface_name, get_interface_state,
)

# ============================================================================
# 用户自定义结构体格式 —— 用于解析子系统的 data 字段
# 对应 C++ 中的 TwoFingerGripperYSStatus
# ============================================================================
TWO_FINGER_GRIPPER_YS_FORMAT = '<d'  # TwoFingerGripperYSStatus: actual_pos (double, 8 bytes)


def print_rt():
    """直接获取方式打印 RT 数据 —— 每个 get_*() 内部自动获取快照"""
    if not has_rt_data():
        print("No RT data yet.")
        return

    print("\n==================== RT Data (Direct) ====================")
    print(f"header_timestamp      : {get_header_timestamp()}")
    print(f"header_frame_id       : {get_header_frame_id()}")
    print(f"system_running_state  : {is_system_running()}")
    print(f"system_info           : {get_system_info()}")

    # 控制器
    print(f"controller.controller_name : {get_controller_name()}")
    print(f"controller.control_cycle   : {get_control_cycle()}")
    print(f"controller.global_count    : {get_global_count()}")
    print(f"controller.master_info     : {get_master_info()}")
    print(f"controller.is_link_up      : {is_link_up()}")
    print(f"controller.ftvalues        : {get_ftvalues_count()} sensors")
    for i in range(get_ftvalues_count()):
        print(f"  ftvalues[{i}]: {get_ftvalue(i)}")

    # 模型数据
    for m in range(get_model_count_rt()):
        print(f"\n--- Model {m} : {get_model_name(m)} ({get_model_type(m)}) ---")
        print(f"  model_name           : {get_model_name(m)}")
        print(f"  model_type           : {get_model_type(m)}")
        print(f"  joint_count          : {get_joint_count(m)}")

        # 关节数据
        for j in range(get_joint_count(m)):
            print(f"    Joint {j} :")
            print(f"      joint_type      : {get_joint_type(m, j)}")
            print(f"      position        : {get_joint_position(m, j)}")
            print(f"      torque          : {get_joint_torque(m, j)}")
            print(f"      is_enabled      : {get_joint_is_enabled(m, j)}")
            print(f"      mode            : {get_joint_mode(m, j)}")
            print(f"      error_code      : {get_joint_error_code(m, j)}")
            print(f"      digit_output    : {get_joint_digit_output(m, j)}")
            print(f"      digit_input     : {get_joint_digit_input(m, j)}")
            print(f"      sensor_torque   : {get_joint_sensor_torque(m, j)}")
            print(f"      velocity        : {get_joint_velocity(m, j)}")
            print(f"      target_position : {get_joint_target_position(m, j)}")

        # 当前点信息
        print("  Current Point :")
        print(f"    point_name        : {get_current_point_name(m)}")
        print(f"    tool_name         : {get_current_tool_name(m)}")
        print(f"    wobj_name         : {get_current_wobj_name(m)}")
        print(f"    robottarget       : {get_current_robottarget(m)}")
        print(f"    jointtarget       : {get_current_jointtarget(m)}")

        # 模型运行状态
        print("  Model Info :")
        print(f"    error_code          : {get_model_error_code(m)}")
        print(f"    error_msg           : {get_model_error_msg(m)}")
        print(f"    model_state         : {get_model_state(m)}")
        print(f"    model_time_rate     : {get_model_time_rate(m)}")
        print(f"    current_func_name   : {get_model_current_func_name(m)}")
        print(f"    ee_pe321            : {get_model_ee_pe321(m)}")

    print("==========================================================\n")


def print_nrt():
    """直接获取方式打印 NRT 数据 —— 每个 get_*() 内部自动获取快照"""
    if not has_nrt_data():
        print("No NRT data yet.")
        return

    print("\n==================== NRT Data (Direct) ====================")
    print(f"header_timestamp      : {get_header_timestamp()}")
    print(f"header_frame_id       : {get_header_frame_id()}")
    print(f"system_running_state  : {is_system_running()}")
    print(f"system_is_init        : {is_system_init()}")

    # 从站
    print(f"controller.slaves     : {get_slave_count()} slaves")
    for i in range(get_slave_count()):
        print(f"  slave[{i}] : {get_slave_name(i)} "
              f"state={get_slave_state(i)} online={get_slave_is_online(i)}")

    # 模型数据（NRT 包含关节限制等）
    for m in range(get_model_count_nrt()):
        print(f"\n--- Model {m} : {get_model_name(m)} ({get_model_type(m)}) ---")
        print(f"  model_name           : {get_model_name(m)}")
        print(f"  model_type           : {get_model_type(m)}")
        print(f"  is_using_sp          : {is_model_using_sp(m)}")
        print(f"  is_collision_detection: {is_model_collision_detection(m)}")
        print(f"  take_photo           : {get_model_take_photo(m)}")
        print(f"  joint_count          : {get_joint_count(m)}")

        # 关节限制
        for j in range(get_joint_count(m)):
            print(f"    Joint {j} limits:")
            print(f"      max_position          : {get_joint_max_position(m, j)}")
            print(f"      min_position          : {get_joint_min_position(m, j)}")
            print(f"      max_vel               : {get_joint_max_vel(m, j)}")
            print(f"      min_vel               : {get_joint_min_vel(m, j)}")
            print(f"      max_acc               : {get_joint_max_acc(m, j)}")
            print(f"      min_acc               : {get_joint_min_acc(m, j)}")
            print(f"      max_collision_torque  : {get_joint_max_collision_torque(m, j)}")

        # 注：示教点、工具、工件、IO 等 NRT 数据如需逐项获取
        # 可参照 system_state_reader.py 中的模式自行扩展 get_*() 函数
        # 批量 NRT 读取推荐使用 topic_sub_snapshot.py 的快照方式

    # 子系统 —— 包含用户端 TwoFingerGripperYSStatus 解析示例
    print(f"\n--- Subsystems ({get_subsystem_count()}) ---")
    for i in range(get_subsystem_count()):
        print(f"  {get_subsystem_name(i)} state={get_subsystem_state(i)} "
              f"data_size={get_subsystem_data_size(i)}")

        # 用户根据 data 大小判断是否匹配自己的结构体，直接做解析
        if get_subsystem_data_size(i) >= struct.calcsize(TWO_FINGER_GRIPPER_YS_FORMAT):
            try:
                unpacked = parse_subsystem_data(i, TWO_FINGER_GRIPPER_YS_FORMAT)
                print("    Parsed as TwoFingerGripperYSStatus:")
                print(f"      actual_pos={unpacked[0]}")
            except Exception as e:
                print(f"    Parse failed: {e}")

    # 接口
    print(f"\n--- Interfaces ({get_interface_count()}) ---")
    for i in range(get_interface_count()):
        print(f"  {get_interface_name(i)} state={get_interface_state(i)}")

    print("============================================================\n")
