# message.py
from __future__ import annotations
from platform_loader import get_topic_module

_topic = get_topic_module()   # 获取正确的 topic 模块（仅加载一次）

def print_rt(data: topic.SystemStateData):
    """打印实时数据（RT）"""
    if data is None:
        print("No RT data yet.")
        return

    print("\n==================== RT Data ====================")
    print(f"header_timestamp      : {data.header_timestamp}")
    print(f"header_frame_id       : {data.header_frame_id}")
    print(f"system_running_state  : {data.system_running_state}")
    print(f"system_info           : {data.system_info}")

    ctrl = data.controller
    print(f"controller.controller_name : {ctrl.controller_name}")
    print(f"controller.control_cycle   : {ctrl.control_cycle}")
    print(f"controller.global_count    : {ctrl.global_count}")
    print(f"controller.master_info     : {ctrl.master_info}")
    print(f"controller.is_link_up      : {ctrl.is_link_up}")

    # 六维力传感器
    for i, ft in enumerate(ctrl.ftvalues):
        print(f"  ftvalues[{i}]: {ft}")   # ft 是 list[float]

    # 模型数据
    for model_idx, model in enumerate(data.models):
        print(f"\n--- Model {model_idx} : {model.model_name} ({model.model_type}) ---")
        print(f"  is_using_sp           : {model.is_using_sp}")
        print(f"  is_collision_detection: {model.is_collision_detection}")
        print(f"  joint_start_idx       : {model.joint_start_idx}")
        print(f"  joint_count           : {model.joint_count}")

        # 关节数据（RT部分）
        start = model.joint_start_idx
        end = start + model.joint_count
        for j in range(start, end):
            joint = data.models_joints[j]
            print(f"    Joint {j - start}:")
            print(f"      joint_type      : {joint.joint_type}")
            print(f"      position        : {joint.position}")
            print(f"      torque          : {joint.torque}")
            print(f"      is_enabled      : {joint.is_enabled}")
            print(f"      mode            : {joint.mode}")
            print(f"      error_code      : {joint.error_code}")
            print(f"      digit_output    : {joint.digit_output}")
            print(f"      digit_input     : {joint.digit_input}")
            print(f"      sensor_torque   : {joint.sensor_torque}")
            print(f"      velocity        : {joint.velocity}")
            print(f"      target_position : {joint.target_position}")

        # 当前点信息
        if model_idx < len(data.models_current_points):
            cur = data.models_current_points[model_idx]
            print("  Current Point :")
            print(f"    point_name        : {cur.point_name}")
            print(f"    tool_name         : {cur.tool_name}")
            print(f"    wobj_name         : {cur.wobj_name}")
            print(f"    tool_data         : {cur.tool_data}")
            print(f"    wobj_data         : {cur.wobj_data}")
            print(f"    robottarget       : {cur.robottarget}")
            print(f"    jointtarget       : {cur.jointtarget}")

        # 模型运行状态信息
        if model_idx < len(data.models_info):
            info = data.models_info[model_idx]
            print("  Model Info :")
            print(f"    error_code          : {info.error_code}")
            print(f"    error_msg           : {info.error_msg}")
            print(f"    model_state         : {info.model_state}")
            print(f"    model_time_rate     : {info.model_time_rate}")
            print(f"    current_func_name   : {info.current_func_name}")
            print(f"    current_func_info   : {info.current_func_info}")
            print(f"    func_count          : {info.func_count}")
            print(f"    info_msg            : {info.info_msg}")
            print(f"    ee_pe321            : {info.ee_pe321}")

    print("==================================================\n")


def print_nrt(data: topic.SystemStateData):
    """打印非实时数据（NRT）"""
    if data is None:
        print("No NRT data yet.")
        return

    print("\n==================== NRT Data ====================")
    print(f"header_timestamp      : {data.header_timestamp}")
    print(f"header_frame_id       : {data.header_frame_id}")
    print(f"system_running_state  : {data.system_running_state}")
    print(f"system_is_init        : {data.system_is_init}")

    # 从站信息
    print(f"controller.slaves     : {len(data.slaves)} slaves")
    for i, slave in enumerate(data.slaves):
        print(f"  slave[{i}] : {slave.slave_name} phy_id={slave.phy_id} alias={slave.alias} "
              f"state={slave.slave_state} online={slave.is_online} virtual={slave.is_virtual} error={slave.is_error}")

    # 模型数据（NRT包含关节限制、工具、工件、负载、示教点等）
    for model_idx, model in enumerate(data.models):
        print(f"\n--- Model {model_idx} : {model.model_name} ({model.model_type}) ---")
        print(f"  model_name           : {model.model_name}")
        print(f"  model_type           : {model.model_type}")
        print(f"  is_using_sp          : {model.is_using_sp}")
        print(f"  is_collision_detection: {model.is_collision_detection}")
        print(f"  take_photo           : {model.take_photo}")
        print(f"  joint_start_idx      : {model.joint_start_idx}")
        print(f"  joint_count          : {model.joint_count}")

        # 关节限制数据
        start = model.joint_start_idx
        end = start + model.joint_count
        for j in range(start, end):
            joint = data.models_joints[j]
            print(f"    Joint {j - start} limits:")
            print(f"      max_position          : {joint.max_position}")
            print(f"      min_position          : {joint.min_position}")
            print(f"      max_vel               : {joint.max_vel}")
            print(f"      min_vel               : {joint.min_vel}")
            print(f"      max_acc               : {joint.max_acc}")
            print(f"      min_acc               : {joint.min_acc}")
            print(f"      max_collision_torque  : {joint.max_collision_torque}")

        # 工具
        print(f"  Tools ({model.tools_count}):")
        t_start = model.tools_start_idx
        for ti in range(model.tools_count):
            tool = data.models_tools[t_start + ti]
            print(f"    tool[{ti}] : {tool.tool_name} data={tool.data}")

        # 工件坐标系
        print(f"  Wobjs ({model.wobjs_count}):")
        w_start = model.wobjs_start_idx
        for wi in range(model.wobjs_count):
            wobj = data.models_wobjs[w_start + wi]
            print(f"    wobj[{wi}] : {wobj.wobj_name} data={wobj.data}")

        # 负载
        print(f"  Loads ({model.loads_count}):")
        l_start = model.loads_start_idx
        for li in range(model.loads_count):
            load = data.models_loads[l_start + li]
            print(f"    load[{li}] : {load.load_name} data={load.data}")

        # 示教点
        print(f"  Teach points ({model.teach_points_count}):")
        tp_start = model.teach_points_start_idx
        for tpi in range(model.teach_points_count):
            pt = data.models_teach_points[tp_start + tpi]
            print(f"    point[{tpi}] : {pt.point_name} tool={pt.tool_name} wobj={pt.wobj_name}")
            print(f"      tool_data={pt.tool_data}")
            print(f"      wobj_data={pt.wobj_data}")
            print(f"      robottarget={pt.robottarget}")
            print(f"      jointtarget={pt.jointtarget}")

    # IO 数据
    print(f"\n--- All IO Data ({len(data.models_io)}) ---")
    for i, io in enumerate(data.models_io):
        print(f"  io[{i}] : {io.io_name} data={io.io_data}")

    # 子系统
    print(f"\n--- Subsystems ({len(data.subsystems)}) ---")
    for sub in data.subsystems:
        print(f"  {sub.subsystem_name} id={sub.id} state={sub.state}")
        # print(f"  {sub.subsystem_name} id={sub.id} state={sub.state} data={sub.data}")

    # 接口
    print(f"\n--- Interfaces ({len(data.interfaces)}) ---")
    for iface in data.interfaces:
        print(f"  {iface.interface_name} id={iface.id} state={iface.state}")

    print("==================================================\n")
