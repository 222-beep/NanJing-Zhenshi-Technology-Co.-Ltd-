import math
import arch

# 初始化指令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Enable}",
]

ROBOT_IP = "192.168.2.199"

# 常量定义
REQUIRED_DIM = 10        # 关节指令字符串需要的总维度
USER_INPUT_AXIS = 6      # 用户输入的关节数
MIN_SPEED_ACCEL = 0.1
MAX_SPEED_ACCEL = 1.0


def parse_joint_angles(input_str):
    """解析关节角度字符串（度 -> 弧度），并补全到 REQUIRED_DIM 维度"""
    angles_deg = []
    for item in input_str.split(","):
        item = item.strip()
        if item == "":
            continue
        deg = float(item)
        if deg < -360.0 or deg > 360.0:
            raise ValueError(f"角度值超出合理范围: {deg}")
        angles_deg.append(deg)

    # 补全或截断到 USER_INPUT_AXIS
    while len(angles_deg) < USER_INPUT_AXIS:
        angles_deg.append(0.0)
    angles_deg = angles_deg[:USER_INPUT_AXIS]

    # 度转弧度
    angles_rad = [d * math.pi / 180.0 for d in angles_deg]

    # 补全到要求的维度
    while len(angles_rad) < REQUIRED_DIM:
        angles_rad.append(0.0)
    return angles_rad


def build_joint_string(angles):
    """构建关节角度指令字符串 {a1,a2,...,a10}"""
    return "{" + ",".join(f"{a:.5f}" for a in angles) + "}"


def show_joint_angles(angles):
    """显示关节角度（弧度 -> 度），仅展示前 USER_INPUT_AXIS 个"""
    deg_list = [f"{a * 180.0 / math.pi:.1f}" for a in angles[:USER_INPUT_AXIS]]
    return "[" + ", ".join(deg_list) + "]"


def clamp(value, min_val, max_val):
    return max(min_val, min(value, max_val))


def main():
    # 创建客户端
    client = arch.create_client(ROBOT_IP)

    # 发送初始化指令
    arch.send_rpcsy(client, init_cmds, 500, 0.1)

    # 轨迹点列表：每项为 {"name": ..., "angles": [...]}（angles 为弧度）
    trajectory_list = []
    speed = 0.95
    accel = 0.95

    while True:
        print("\n=====================================================")
        print("          MoveSeriesToppJ 关节轨迹控制")
        print("=====================================================")
        print("1）设置轨迹起点（当前位置）")
        print("2）添加关节轨迹点")
        print(f"3）设置速度系数 [{MIN_SPEED_ACCEL}~{MAX_SPEED_ACCEL}]")
        print(f"4）设置加速度系数 [{MIN_SPEED_ACCEL}~{MAX_SPEED_ACCEL}]")
        print("5）执行轨迹运动")
        print("6）清空所有轨迹点")
        print("7）查看已添加轨迹点")
        print("8）安全停止并退出")
        print("=====================================================")
        print(f"当前：速度：{speed:.2f}   加速度：{accel:.2f}")

        user_input = input("选择（1-8）：").strip()
        if user_input == "":
            print("错误：请输入有效选项！")
            continue

        try:
            if user_input == "1":
                print("[操作] 设置轨迹起点...")
                arch.send_rpcsy(
                    client,
                    ["{MoveSeriesToppJ --type=first_insert}"],
                    5000, 0.5,
                )
                trajectory_list = []
                print("[成功] 轨迹起点已设置！")

            elif user_input == "2":
                angle_input = input("[操作] 输入6个关节角（逗号分隔，单位：度）：")
                angles_rad = parse_joint_angles(angle_input)
                joint_str = build_joint_string(angles_rad)

                cmd = (
                    "{MoveSeriesToppJ --type=insert "
                    f"--jointtarget_value={joint_str}}}"
                )
                arch.send_rpcsy(client, [cmd], 5000, 0.5)

                point_name = f"j{len(trajectory_list) + 1}"
                trajectory_list.append({"name": point_name, "angles": angles_rad})
                print(f"[成功] 已添加：{point_name} {show_joint_angles(angles_rad)}")

            elif user_input == "3":
                s = input(f"[操作] 输入速度系数({MIN_SPEED_ACCEL}~{MAX_SPEED_ACCEL})：")
                speed = clamp(float(s), MIN_SPEED_ACCEL, MAX_SPEED_ACCEL)
                print(f"[成功] 速度已设为：{speed:.2f}")

            elif user_input == "4":
                s = input(f"[操作] 输入加速度系数({MIN_SPEED_ACCEL}~{MAX_SPEED_ACCEL})：")
                accel = clamp(float(s), MIN_SPEED_ACCEL, MAX_SPEED_ACCEL)
                print(f"[成功] 加速度已设为：{accel:.2f}")

            elif user_input == "5":
                if len(trajectory_list) == 0:
                    print("[错误] 请先添加轨迹点！")
                    continue

                print("[操作] 开始执行轨迹...")
                cmd = (
                    "{MoveSeriesToppJ --type=start "
                    f"--vel_coef={speed:.5f} --acc_coef={accel:.5f}}}"
                )
                arch.send_rpcsy(client, [cmd], 30000, 1.0)
                print("[成功] 轨迹执行完成！")

            elif user_input == "6":
                print("[操作] 清空轨迹点...")
                arch.send_rpcsy(
                    client,
                    ["{MoveSeriesToppJ --type=clear}"],
                    5000, 0.5,
                )
                trajectory_list = []
                print("[成功] 已清空所有轨迹点！")

            elif user_input == "7":
                print(f"[信息] 轨迹点总数：{len(trajectory_list)}")
                if len(trajectory_list) == 0:
                    print("  暂无轨迹点")
                else:
                    for i, p in enumerate(trajectory_list, start=1):
                        print(f"  {i}. {p['name']}: {show_joint_angles(p['angles'])}")

            elif user_input == "8":
                print("[操作] 安全停止机器人...")
                # 停止机器人运动
                arch.send_rpcsy(client, ["{Stop --last_count=10}"], 5000, 1.0)
                # 恢复伺服使能
                arch.send_rpcsy(client, ["{Start}"], 5000, 1.0)
                print("[成功] 程序安全退出！")
                break

            else:
                print("[错误] 请输入 1~8 的数字！")

        except ValueError as e:
            print(f"[错误] {e}")
        except Exception as e:
            print(f"[错误] {e}")


# 程序入口
if __name__ == "__main__":
    main()
