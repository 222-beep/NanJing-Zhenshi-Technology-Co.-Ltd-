import arch

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
    client = arch.create_client(ROBOT_IP)
    
    # 发送初始化指令
    arch.send_rpcsy(client, init_cmds,500,0.1)  # 同步rpc (client, 指令)
    
    # 主循环 - 等待用户输入控制拖动
    while True: 
        user_input = input("开始机器人拖动? (start/stop): ").strip().lower()
        
        if user_input == "start":
            print("Start DragInCST!!!")
            arch.send_rpcsy(client, Dra_sta, 5000, 1.0)  # 超时时间5秒，间隔1秒
            user_input = input("Stop dragging in CST? (stop): ").strip().lower()
            
        elif user_input == "stop":
            arch.send_rpcsy(client, Dra_stp, 5000, 1.0)
            print("Dragging stopped!!")
            print()
            
        else:
            print("Please enter 'start' or 'stop'")


# 程序入口
if __name__ == "__main__":
    main()