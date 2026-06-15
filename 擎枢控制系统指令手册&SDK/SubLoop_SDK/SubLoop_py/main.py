import arch
import sys

# 初始化命令列表
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

def main():
    robot_ip = "192.168.2.241"
    client = arch.create_client(robot_ip)

    # 发送初始化指令
    # arch.send_rpcsy(client, init_cmds, 500, 1)

    # 第一条SubLoop指令
    first_subloop_cmds = [
        "{SubLoop --exec={Recover}||SubLoop --exec={Recover}}",
    ]

    # 第一条指令必须是异步，且timeout必须设置得足够大，否则后续指令会被丢弃
    # arch.send_rpc_async(client, first_subloop_cmds, sys.maxsize, 1)

    your_old_cmds = [
        "{MoveAbsJ||MoveAbsJ}",
    ]

    # 示例指令列表，替换为你需要测试的指令（双模型）
    your_cmds = [
        "{SubLoop --exec=Recover||SubLoop --exec=Recover}",
        "{SubLoop --exec=MoveAbsJ||SubLoop --exec=MoveAbsJ}",
        "{SubLoop --exec=exit||SubLoop --exec=exit}",
        "{SubLoop --exec||SubLoop --exec}",
    ]

    # 普通异步RPC调用示例
    # arch.send_rpc_async(client, your_old_cmds, 50000, 1)

    # 主循环发送运动指令
    while True:
        sys.stderr.write("Input commands: \n")
        sys.stderr.flush()
        input_cmd = input().strip()
        your_cmds.clear()
        your_cmds.append(input_cmd)
        # arch.send_rpcsy(client, your_cmds, 50000, 5)
        arch.send_rpc_async(client, your_cmds, 1000000, 3)

    return 0


if __name__ == "__main__":
    main()