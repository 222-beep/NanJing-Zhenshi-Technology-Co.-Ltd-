import arch

# 单侧机械臂关节数，按实际机型修改（与单臂 main1.py 一致，例如 6、7），默认 7 轴
NUM_JOINTS_PER_ARM = 7

# 初始化命令列表
init_cmds = [
    "{Clear}",
]

# JogAnyJ 启动指令 - 双臂初始位置
zeros = ",".join(["0"] * NUM_JOINTS_PER_ARM)
Jog_start = [
    f"{{JogAnyJ --joint_pos={{{zeros}}} --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5"
    f"||JogAnyJ --joint_pos={{{zeros}}} --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5}}",
]

# 双臂回零位（MoveAbsJ）
home_cmds = [
    f"{{MoveAbsJ --jointtarget={{{zeros}}}||MoveAbsJ --jointtarget={{{zeros}}}}}",
]

# 停止指令
Jog_stop = [
    "{Stop --last_count=10||Stop --last_count=10}",
]


def build_joganyj_cmd(left_joints, right_joints, joint_vel, joint_acc, joint_dec):
    """构建双臂 JogAnyJ 指令字符串，返回可直接下发的 RPC 命令。"""
    left_str = ",".join(str(x) for x in left_joints)
    right_str = ",".join(str(x) for x in right_joints)
    return (
        f"{{JogAnyJ --joint_pos={{{left_str}}} --joint_vel={joint_vel} "
        f"--joint_acc={joint_acc} --joint_dec={joint_dec}"
        f"||JogAnyJ --joint_pos={{{right_str}}} --joint_vel={joint_vel} "
        f"--joint_acc={joint_acc} --joint_dec={joint_dec}}}"
    )

 
def custom_joganyj_motion(client):
    """读取左右臂自定义关节输入并发送一次双臂 JogAnyJ 运动。"""
    print("\n========== 自定义双臂 JogAnyJ ==========")
    try:
        left_input = input(
            f"请输入左臂 {NUM_JOINTS_PER_ARM} 个关节角度(弧度)，逗号分隔: "
        ).strip()
        left_joints = [float(x.strip()) for x in left_input.split(",")]
        if len(left_joints) != NUM_JOINTS_PER_ARM:
            print(f"错误: 左臂需要 {NUM_JOINTS_PER_ARM} 个关节角度!")
            return

        right_input = input(
            f"请输入右臂 {NUM_JOINTS_PER_ARM} 个关节角度(弧度)，逗号分隔: "
        ).strip()
        right_joints = [float(x.strip()) for x in right_input.split(",")]
        if len(right_joints) != NUM_JOINTS_PER_ARM:
            print(f"错误: 右臂需要 {NUM_JOINTS_PER_ARM} 个关节角度!")
            return

        speed = float(input("请输入运动速度(默认0.1): ") or "0.1")
        joint_acc = 0.5
        joint_dec = 0.5

        custom_cmd = build_joganyj_cmd(
            left_joints, right_joints, speed, joint_acc, joint_dec
        )
        print(f"执行指令: {custom_cmd}")
        arch.send_rpcsy(client, [custom_cmd], 5000, 1.0)

    except ValueError:
        print("输入格式错误! 请确保输入的是数字。")
    except Exception as e:
        print(f"发生错误: {e}")

ROBOT_IP = "192.168.2.199"

def main():
    client = arch.create_client(ROBOT_IP)

    arch.send_rpcsy(client, init_cmds, 500, 0.1)

    while True:
        print("\n可用命令:")
        print("start - 启动 JogAnyJ 控制，双臂到零位")
        print("home  - 双臂 MoveAbsJ 回零")
        print("stop  - 停止运动")
        print("custom- 输入自定义双臂关节位置(弧度)")
        print("exit  - 退出程序")

        user_input = input("请输入命令: ").strip().lower()

        if user_input == "start":
            print("启动双臂 JogAnyJ 控制，双臂到零位!")
            arch.send_rpcsy(client, Jog_start, 5000, 1.0)
            print("双臂已移动到初始位置")

        elif user_input == "home":
            print("双臂回零位(MoveAbsJ)...")
            arch.send_rpcsy(client, home_cmds, 50000, 0.5)

        elif user_input == "stop":
            arch.send_rpcsy(client, Jog_stop, 5000, 1.0)
            print("运动已停止!")

        elif user_input == "custom":
            custom_joganyj_motion(client)

        elif user_input == "exit":
            print("退出程序...")
            arch.send_rpcsy(client, Jog_stop, 5000, 1.0)
            break

        else:
            print("未知命令，请重新输入!")


if __name__ == "__main__":
    main()
