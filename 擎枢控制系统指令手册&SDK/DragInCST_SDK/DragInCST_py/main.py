import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Enable}",
]

# 拖动启动指令列表
Dra_sta = [
    "{SwitchToCST}",
    "{DragInCST --cf_coef={0,0,0,0,0,0,0} --vf_coef={0,0,0,0,0,0,0} --vel_limit={0.3,0.3,0.3,0.3,0.3,0.3,0.3} --ping_pong_amp=0 --zero_check=0.004}"
]

# 拖动停止指令列表
Dra_stp = [
    "{Stop --last_count=10}",
    "{SwitchToCSP}",
    "{Recover}",
    "{Start}"
]

ROBOT_IP = "192.168.2.199"

def main():
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return
    
    # 发送初始化指令
    send_rpcsy(client, init_cmds, timeout_ms=500, sleep_s=0.1)  # 同步rpc (client, 指令)
    
    # 主循环 - 等待用户输入控制拖动
    while True: 
        user_input = input("开始机器人拖动? (start/stop): ").strip().lower()
        
        if user_input == "start":
            print("Start DragInCST!!!")
            send_rpcsy(client, Dra_sta, timeout_ms=5000, sleep_s=1.0)  # 超时5秒，间隔1秒
            user_input = input("Stop dragging in CST? (stop): ").strip().lower()
            
        elif user_input == "stop":
            send_rpcsy(client, Dra_stp, timeout_ms=5000, sleep_s=1.0)
            print("Dragging stopped!!")
            print()
            
        else:
            print("Please enter 'start' or 'stop'")


# 程序入口
if __name__ == "__main__":
    main()