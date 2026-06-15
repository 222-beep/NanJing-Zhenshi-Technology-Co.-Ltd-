import arch

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Mode}",
    "{SetMaxToq}",
    "{Recover}",
    "{SetRate}",
    "{Enable}",
]

# 三条 IO 指令（示例参数，可自行修改）
getdi_cmds = [
    "{GetDI --di_name=DO0}",
]

setdo_cmds = [
    "{SetDO --do_name=DO0 --do_value=1}",
]

dopulse_cmds = [
    "{DOPulse --do_name=DO0 --pulse_active=1 --high_cycles=10 --low_cycles=10}",
]

ROBOT_IP = "192.168.2.217"


def main():
    """主函数 - IO 模块指令交互发送"""
    client = arch.create_client(ROBOT_IP)

    # 发送初始化指令
    arch.send_rpcsy(client, init_cmds, 500, 0.1)
    print("初始化完成")

    while True:
        print("\n=== IO 模块指令菜单 ===")
        print("getdi    - 读取数字输入  ")
        print("setdo    - 设置数字输出  ")
        print("dopulse  - 脉冲输出      ")
        print("exit     - 退出程序")

        user_input = input("请输入命令: ").strip().lower()

        if user_input == "getdi":
            arch.send_rpcsy(client, getdi_cmds, 5000, 0.5)
            print("[GetDI] 指令已发送")

        elif user_input == "setdo":
            arch.send_rpcsy(client, setdo_cmds, 5000, 0.5)
            print("[SetDO] 指令已发送")

        elif user_input == "dopulse":
            arch.send_rpcsy(client, dopulse_cmds, 5000, 0.5)
            print("[DOPulse] 指令已发送")

        elif user_input == "exit":
            print("退出程序...")
            break

        else:
            print("未知命令，请重新输入!")


# 程序入口
if __name__ == "__main__":
    main()
