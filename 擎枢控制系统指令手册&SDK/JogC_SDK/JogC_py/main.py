import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

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

# 运动指令列表
motion_cmds = [
    "{JogC --motion_type=0 --direction=1 --step=0.1 --coordinate=0 --speed=v100}",
    "{JogC --motion_type=0 --direction=-1 --step=0.1 --coordinate=0 --speed=v100}",
]

# 用户自定义指令列表
your_cmds = [
    # 添加你的指令
]

ROBOT_IP = "192.168.2.199"

def main():
    """主函数"""
    # 创建客户端
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # 发送初始化指令
    send_rpcsy(client, init_cmds, timeout_ms=500, sleep_s=0.1)  # 同步rpc (client, 指令)
    
    # 主循环发送运动指令
    while True:
        #send_rpc_async(client, motion_cmds, timeout_ms=10000, wait_s=0.5)  # 异步rpc
        send_rpcsy(client, motion_cmds, timeout_ms=10000, sleep_s=0.5)


# 程序入口
if __name__ == "__main__":
    main()
