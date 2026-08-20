# topic_sub_socket_state.py
# 通过 Topic SDK 订阅系统实时状态，读取关节位置、速度、末端位姿和错误码。
# 使用 Direct Mode（自动快照），在主循环中直接调用函数即可。

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import time
from platform_loader import get_topic_module

topic = get_topic_module()

from system_state_reader import (
    has_rt_data, SystemStateReader,
)

PUBLISHER_IP = "192.168.11.11"   # 修改为实际发布者 IP

if __name__ == "__main__":
    topic.start_subscriber(PUBLISHER_IP)
    print(f"Subscriber started, listening to {PUBLISHER_IP}:19091")

    try:
        while True:
            time.sleep(0.05)

            if not has_rt_data():
                continue

            rt = SystemStateReader.snapshot_rt()
            if not rt.valid():
                continue

            # 系统是否正在运行
            print(f"system_running: {rt.is_system_running()}")

            for m in range(rt.model_count()):
                print(f"\n--- Model {m} : {rt.model_name(m)} ---")

                # 模型错误码
                print(f"  model err_code : {rt.model_error_code(m)}")

                # 末端位姿（笛卡尔坐标）
                if rt.has_current_point(m):
                    print(f"  robottarget : {rt.current_robottarget(m)}")

                # 各关节的位置、速度和错误码
                for j in range(rt.joint_count(m)):
                    pos = rt.joint_position(m, j)
                    vel = rt.joint_velocity(m, j)
                    err = rt.joint_error_code(m, j)
                    print(f"  Joint {j}  pos={pos}  vel={vel}  err={err}")

            print("\n==================================================\n")

    except KeyboardInterrupt:
        print("\nExited.")
