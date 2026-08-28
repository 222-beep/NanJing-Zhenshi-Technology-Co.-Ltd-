import threading
import time

from robot_runtime import PRINT_INTERVAL_S, RPC_TIMEOUT_MS, initialize_system
from robot_command.rpc_client import send_rpcsy, send_rpc_async
from robot_state.system_state_reader import get_current_jointtarget, get_current_robottarget


def format_target(values):
    """把关节角数组转换成控制器指令需要的 {j1,j2,...} 字符串格式。"""
    return "{" + ",".join(f"{value:.10g}" for value in values) + "}"


def move_absj(client, jointtarget):
    """用 jointtarget 数值直接下发 MoveAbsJ，不定义命名点位。"""
    target_value = format_target(jointtarget)
    cmds = [f"{{MoveAbsJ --jointtarget_value={target_value}}}"]
    send_rpcsy(client, cmds, timeout_ms=RPC_TIMEOUT_MS, sleep_s=0.1)


def print_state_loop():
    """打印当前末端位姿和当前关节角度"""
    while True:
        print(f"current robottarget: {get_current_robottarget(0)}")
        print(f"current jointtarget: {get_current_jointtarget(0)}")
        time.sleep(PRINT_INTERVAL_S)


def main():
    """主函数：读取当前状态，修改第六关节，用 MoveAbsJ 下发。"""
    robot_ip = "192.168.11.11"
    client = initialize_system(robot_ip)

    printer = threading.Thread(target=print_state_loop, daemon=False)
    printer.start()

    end_pose = get_current_robottarget(0)
    current_joint = list(get_current_jointtarget(0))

    joint_angles = [0.0] * 10
    joint_angles[:len(current_joint)] = current_joint
    joint_angles[5] = 0.0
    move_absj(client, joint_angles)

    printer.join()


if __name__ == "__main__":
    main()
