import time
import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

ROBOT_IP = "192.168.2.199"

# 可选模式：
# "free_drag"      ：6 个方向都可拖动，零力拖动
# "z_force"        ：只开启 Z 方向恒力
# "hybrid_move_z"  ：直线运动 + Z 方向力控，力位混合
DRAG_MODE = "free_drag"

# 初始化命令
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Mode}", 
    "{SetMaxToq}",
    "{Recover}",
    "{SetRate}",
    "{Enable}",
    "{Var --clear}",
    "{Recover}",
]

# 停止力控
stop_cmds = [
    "{Stop}",
]

# 例 1：6 自由度零力拖动
free_drag_cmds = [
    "{ForcePositionHybridControl "
    "--force_direction={1,1,1,1,1,1} "
    "--force_target={0,0,0,0,0,0} "
    "--damping_gain={0.001,0.001,0.001,0.1,0.1,0.1}}"
]

# 例 2：只开启工具坐标系 Z 方向恒力
z_force_cmds = [
    "{ForcePositionHybridControl "
    "--force_direction={0,0,1,0,0,0} "
    "--force_target={0,0,3,0,0,0} "
    "--damping_gain={0.001,0.001,0.001,0.1,0.1,0.1} "
    "--coordinate=0}"
]

# 例 3：运动过程中开启 Z 方向力控，力位混合控制
# 注意：p1 要改成你现场安全、可达的位置
hybrid_move_z_cmds = [
    "{Var --type=robottarget --name=p1 --value={0.490,0.100,0.680,0,0.70710678,0,0.70710678} }",
    "{MoveBlend --type=first_insert}",
    "{MoveBlend --type=insert_line --robottarget_var=p1}",
    "{ForcePositionHybridControl "
    "--force_direction={0,0,1,0,0,0} "
    "--force_target={0,0,3,0,0,0} "
    "--damping_gain={0.001,0.001,0.001,0.1,0.1,0.1} "
    "--open_tg=1}"
]

DRAG_CMD_MAP = {
    "free_drag": free_drag_cmds,
    "z_force": z_force_cmds,
    "hybrid_move_z": hybrid_move_z_cmds,
}


def main():
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    drag_cmds = DRAG_CMD_MAP.get(DRAG_MODE)
    if drag_cmds is None:
        raise ValueError(f"未知 DRAG_MODE: {DRAG_MODE}")

    send_rpcsy(client, init_cmds, timeout_ms=500, sleep_s=0.1)

    try:
        # ForcePositionHybridControl 是持续型控制，不要在 while True 里一直重复发
        # 发一次异步指令即可
        send_rpc_async(client, drag_cmds, timeout_ms=86400000, wait_s=0.1)

        # 保持程序运行，等待 Ctrl+C
        while True:
            time.sleep(0.2)

    except KeyboardInterrupt:
        print("\nCtrl+C detected. Send Stop...")

    finally:
        # 退出时一定要 Stop，不然力控可能还在执行
        try:
            send_rpcsy(client, stop_cmds, timeout_ms=1000, sleep_s=0.1)
            print("Stop command sent.")
        except Exception as exc:
            print(f"Failed to send Stop: {exc}")


if __name__ == "__main__":
    main()