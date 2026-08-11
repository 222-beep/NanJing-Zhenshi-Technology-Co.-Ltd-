# ==================================================================
#  main-sine.py  ——  单臂 JogAnyJ 正弦运动控制
#
#  功能：
#    1. 连接机器人控制器，执行 Clear → Disable → Enable 初始化
#    2. 支持 start / stop / custom / exit 交互式命令
#    3. start：以 100Hz 异步下发正弦运动指令，同步导出 CSV 指令数据
#    4. custom：手动输入关节角度，同步下发单次指令
#    5. 退出或停止时自动发送 Stop 指令
# ==================================================================

import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

import math
import time
import csv
import threading

# ==================================================================
#  常量配置
# ==================================================================

NUM_JOINTS = 7

# 初始关节角度（度）
XX = [0.0] * NUM_JOINTS
# 各关节振幅（度）
AMP_DEG = [3.0] * NUM_JOINTS

# 正弦运动参数
SINE_FREQ = 2.0    # 频率 2Hz（周期 0.5s）
SINE_DT   = 0.01   # 控制步长 20ms（100Hz 下发）

# JogAnyJ 运动指令参数（正弦运动时使用）
MOTION_ACC = "{12.0}"   # 加速度
MOTION_DEC = "{12.0}"   # 减速度
MOTION_VEL = "{3.0}"    # 速度
MOTION_LAST_COUNT = 20

# JogAnyJ 启动/自定义指令参数（归零、custom 时使用）
START_ACC = "{1.0}"     # 加速度（低速）
START_DEC = "{1.0}"     # 减速度（低速）
START_VEL = "{1.0}"     # 速度（低速）
START_LAST_COUNT = 300

# 机器人 IP
ROBOT_IP = "192.168.11.11"

# 输出文件路径：固定写到脚本所在目录，避免受启动目录的权限影响
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CSV_PATH = os.path.join(SCRIPT_DIR, "sine_motion_data.csv")
START_TIME_PATH = os.path.join(SCRIPT_DIR, "jog_start_time.txt")

# 运行标志
running = False

# ==================================================================
#  工具函数
# ==================================================================

# 获取壁钟时间戳（毫秒级）
def wall_clock_ms():
    return int(time.time() * 1000)

# 生成 {0,0,...,0} 格式字符串（num_joints 个关节 + 3 个填充 0）
def make_zero_joint_pos(num_joints):
    total = num_joints + 3
    return "{" + ",".join(["0"] * total) + "}"

# 根据弧度数组生成 {val,val,...,0,0,0} 格式的关节位置字符串
def make_joint_pos(rad, n):
    values = [f"{rad[i]:.6f}" for i in range(n)]
    return "{" + ",".join(values) + ",0,0,0}"

# 构造 JogAnyJ 指令字符串
def make_jog_cmd(joint_pos, acc, dec, vel, last_count):
    return (f"{{JogAnyJ --jointtarget_value={joint_pos}"
            f" --joint_acc={acc}"
            f" --joint_dec={dec}"
            f" --joint_vel={vel}"
            f" --last_count={last_count}}}")

# ==================================================================
#  正弦运动循环（异步发送，100Hz 下发指令，同步导出 CSV 指令数据）
# ==================================================================

def sine_motion_loop(client):
    global running
    try:
        csv_file = open(CSV_PATH, "w", newline="")
    except OSError as e:
        print(f"无法创建数据文件！({CSV_PATH})")
        print(f"错误信息: {e}")
        running = False   # 复位运行标志，可重新 start
        return

    writer = csv.writer(csv_file)
    # CSV 表头：增加 wall_clock_ms 列，用于与 Topic CSV 对齐时间轴
    writer.writerow(["time", "wall_clock_ms"] + [f"joint{i + 1}" for i in range(NUM_JOINTS)])

    start_wall = wall_clock_ms()
    # 记录启动时间戳到独立文件，方便后续与 Topic CSV 对比
    try:
        with open(START_TIME_PATH, "w") as log_file:
            log_file.write(f"{start_wall}\n")
        print("[时间同步] 运动启动壁钟时间已写入 jog_start_time.txt")
        print(f"[时间同步] start_wall_clock = {start_wall} ms")
    except OSError as e:
        print(f"[时间同步] 写入 jog_start_time.txt 失败: {e}")

    t = 0.0
    omega = 2.0 * math.pi * SINE_FREQ  # 角频率 ω = 2πf

    while running:
        t += SINE_DT

        # 计算各关节角度
        x_deg = [XX[i] + AMP_DEG[i] * math.sin(omega * t) for i in range(NUM_JOINTS)]
        x_rad = [d / 180.0 * math.pi for d in x_deg]

        # 写入 CSV（度数 + 壁钟时间戳）
        writer.writerow([f"{t:.6f}", wall_clock_ms()] + [f"{d:.6f}" for d in x_deg])

        # 构造 JogAnyJ 指令并异步发送
        joint_pos = make_joint_pos(x_rad, NUM_JOINTS)
        cmd = make_jog_cmd(joint_pos, MOTION_ACC, MOTION_DEC, MOTION_VEL, MOTION_LAST_COUNT)
        send_rpc_async(client, [cmd], wait_s=0.02, timeout_ms=600000)

        # 维持 100Hz 下发节奏
        time.sleep(SINE_DT)

    csv_file.close()
    print(f"数据已保存到 {CSV_PATH}")

# ==================================================================
#  主函数
# ==================================================================

def main():
    global running

    # 初始化命令：清除错误 → 去使能 → 使能
    init_cmds = [
        "{Clear}",
        "{Disable}",
        "{Enable}",
    ]

    # 全零初始关节位置
    zero_joint_pos = make_zero_joint_pos(NUM_JOINTS)

    # JogAnyJ 初始动作：归零，低速到达初始位置
    jog_start_cmds = [
        make_jog_cmd(zero_joint_pos, START_ACC, START_DEC, START_VEL, START_LAST_COUNT)
    ]

    # 停止动作
    jog_stop_cmds = [
        "{Stop --last_count=10}"
    ]

    # 创建 RPC 客户端连接
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # 执行初始化命令序列（Clear → Disable → Enable）
    send_rpcsy(client, init_cmds, timeout_ms=5000, sleep_s=0.1)

    # 主循环
    while True:
        print("\n可用命令:")
        print("start - 启动 JogAnyJ 正弦运动")
        print("stop - 停止运动")
        print("custom - 输入自定义关节位置")
        print("exit - 退出程序")

        user_input = input("请输入命令: ").strip().lower()

        if user_input == "start":
            if not running:
                running = True
                print("启动正弦运动控制...")
                motion_thread = threading.Thread(target=sine_motion_loop, args=(client,), daemon=True)
                motion_thread.start()
                print("正弦运动已启动，输入 'stop' 停止")
            else:
                print("运动已在运行中")

        elif user_input == "stop":
            running = False
            time.sleep(0.1)
            send_rpcsy(client, jog_stop_cmds, timeout_ms=5000, sleep_s=0.1)
            print("运动已停止")

        elif user_input == "custom":
            try:
                print(f"请输入{NUM_JOINTS}个关节角度(弧度)，用逗号分隔: ", end="")
                joint_input = input()

                joints_str = joint_input.split(",")
                if len(joints_str) != NUM_JOINTS:
                    print(f"错误: 需要输入{NUM_JOINTS}个关节角度!")
                    continue

                joint_pos = "{"
                for i in range(NUM_JOINTS):
                    joint_pos += joints_str[i]
                    joint_pos += ","
                joint_pos += "0,0,0}"

                custom_cmd = make_jog_cmd(joint_pos, START_ACC, START_DEC, START_VEL, START_LAST_COUNT)
                print(f"执行指令: {custom_cmd}")

                send_rpcsy(client, [custom_cmd], timeout_ms=5000, sleep_s=0.1)

            except ValueError:
                print("输入格式错误，请确保输入的是数字")
            except Exception as e:
                print(f"错误信息: {e}")

        elif user_input == "exit":
            print("退出程序...")
            running = False
            time.sleep(0.1)
            send_rpcsy(client, jog_stop_cmds, timeout_ms=5000, sleep_s=0.1)
            break

        else:
            print("未知命令，请重新输入!")

# 程序入口
if __name__ == "__main__":
    main()
