import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

# 机器人关节轴数，按实际机型修改（例如 6、7），这里默认7轴
NUM_JOINTS = 7

# 初始化命令列表
init_cmds = [
    "{Clear}",
]

# JogAnyJ 启动指令 - 设置初始位置
Jog_start = [
    f"{{JogAnyJ --joint_pos={{{','.join(['0'] * NUM_JOINTS)}}} --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5}}"
]

# JogAnyJ 停止指令
Jog_stop = [
    "{Stop --last_count=10}",
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
    send_rpcsy(client, init_cmds, 500, 0.1)
    
    # 主循环 - 等待用户输入控制JogAnyJ
    while True:
        print("\n可用命令:")
        print("start - 启动JogAnyJ控制")
        print("stop - 停止运动")
        print("custom - 输入自定义关节位置")
        print("exit - 退出程序")
        
        user_input = input("请输入命令: ").strip().lower()
        
        if user_input == "start":
            print("启动JogAnyJ控制!")
            send_rpcsy(client, Jog_start, 5000, 1.0)
            print("机器人已移动到初始位置")
            
        elif user_input == "stop":
            send_rpcsy(client, Jog_stop, 5000, 1.0)
            print("运动已停止!")
            
        elif user_input == "custom":
            try:
                # 获取用户输入的关节位置
                joint_input = input(f"请输入{NUM_JOINTS}个关节角度(弧度)，用逗号分隔: ")
                joints = [float(x.strip()) for x in joint_input.split(",")]
                
                if len(joints) != NUM_JOINTS:
                    print(f"错误: 需要输入{NUM_JOINTS}个关节角度!")
                    continue
                    
                # 获取速度参数
                speed = float(input("请输入运动速度(默认0.1): ") or "0.1")
                
                # 构建自定义指令
                custom_cmd = f"{{JogAnyJ --joint_pos={{{','.join(map(str, joints))}}} --joint_vel={speed} --joint_acc={0.5} --joint_dec={0.5}}}"
                print(f"执行指令: {custom_cmd}")
                send_rpcsy(client, [custom_cmd], 5000, 1.0) 
                
            except ValueError:
                print("输入格式错误! 请确保输入的是数字。")
            except Exception as e:
                print(f"发生错误: {e}")
                
        elif user_input == "exit":
            print("退出程序...")
            arch.send_rpcsy(client, Jog_stop, 5000, 1.0)  # 确保停止运动
            break
            
        else:
            print("未知命令，请重新输入!")

# 程序入口
if __name__ == "__main__":
    main()
