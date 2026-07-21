import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async
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
    client = RpcClient(robot_ip)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # 发送初始化指令
    # send_rpcsy(client, init_cmds, timeout_ms=500, sleep_s=1)

    # 第一条SubLoop指令
    first_subloop_cmds = [
        "{SubLoop --exec={Recover}||SubLoop --exec={Recover}}",
    ]

    # 第一条指令必须是异步，且timeout必须设置得足够大，否则后续指令会被丢弃
    # send_rpc_async(client, first_subloop_cmds, timeout_ms=sys.maxsize, wait_s=1)

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
    # send_rpc_async(client, your_old_cmds, timeout_ms=50000, wait_s=1)

    # 主循环发送运动指令
    while True:
        sys.stderr.write("Input commands: \n")
        sys.stderr.flush()
        input_cmd = input().strip()
        your_cmds.clear()
        your_cmds.append(input_cmd)
        # send_rpcsy(client, your_cmds, timeout_ms=50000, sleep_s=5)
        send_rpc_async(client, your_cmds, timeout_ms=1000000, wait_s=3)

    return 0


if __name__ == "__main__":
    main()