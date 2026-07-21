import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

# 初始化命令列表 - 双臂版本
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
    # 定义机械臂1的关节目标变量
    "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j3 --value={-0.1,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j4 --value={-0.2,0,0,0,0,0,0,0,0,0}}",
    # 定义机械臂2的关节目标变量
    "{Var --type=jointtarget --name=j11 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j21 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j22 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j23 --value={-0.1,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j24 --value={-0.2,0,0,0,0,0,0,0,0,0}}"
]

# 双臂运动指令列表 - 使用|分隔两个机械臂的指令
motion_cmds = [  
    # 双臂同时运动到初始位置
    "{MoveAbsJ --jointtarget_var=j0||MoveAbsJ --jointtarget_var=j11}",
    # 双臂同时运动到不同位置
    "{MoveAbsJ --jointtarget_var=j1||MoveAbsJ --jointtarget_var=j21}",
    "{MoveAbsJ --jointtarget_var=j2||MoveAbsJ --jointtarget_var=j22}",                          
    "{MoveAbsJ --jointtarget_var=j3||MoveAbsJ --jointtarget_var=j23}",
    "{MoveAbsJ --jointtarget_var=j4||MoveAbsJ --jointtarget_var=j24}"
]

# 双臂在线规划指令
motion_speedL_cmds = [
    # 双臂同时进行在线规划运动
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000||SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000||SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
    "{Stop||Stop}",
    "{Start||Start}",
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
    send_rpcsy(client, init_cmds, timeout_ms=500, sleep_s=0.1)  # 同步rpc (client, 指令, 超时ms, 间隔s)
    
    # 主循环发送运动指令
    while True:
        # 发送同步双臂运动指令
        send_rpcsy(client, motion_cmds, timeout_ms=5000, sleep_s=0.5)
        # 发送异步双臂在线规划指令
        send_rpc_async(client, motion_speedL_cmds, timeout_ms=10000, wait_s=0.5)

# 程序入口
if __name__ == "__main__":
    main()
