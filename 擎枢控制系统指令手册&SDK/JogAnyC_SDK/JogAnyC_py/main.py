import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

# 笛卡尔目标点维数：x,y,z,q1,q2,q3,q4
NUM_CARTESIAN = 7

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Enable}",
]

# JogAnyC 启动指令 - 执行初始动作
Jog_start = [
    "{JogAnyC --robottarget_value={0.6,0.1,0.64,-0.5,0.5,-0.5,0.5} --cartesian_vel={1.0} --cartesian_acc={1.0} --cartesian_dec={1.0}}"
]

# JogAnyC 停止指令
Jog_stop = [
    "{Stop --last_count=10}",
]

ROBOT_IP = "192.168.11.11"

# 拼接 JogAnyC 指令
def build_jog_cmd(robottarget_value, speed):
    return f"{{JogAnyC --robottarget_value={robottarget_value} --cartesian_vel={{{speed}}} --cartesian_acc={{1.0}} --cartesian_dec={{1.0}}}}"

def main():
    """主函数"""
    # 创建客户端
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return
    
    # 发送初始化指令
    send_rpcsy(client, init_cmds, timeout_ms=500, sleep_s=0.1)
    
    # 轨迹点位列表（add 添加，run 按序执行）：[(robottarget_value, speed), ...]
    trajectory_points = []
    
    # 主循环 - 等待用户输入控制JogAnyC
    while True:
        print("\n可用命令:")
        print("start - 启动JogAnyC控制")
        print("stop - 停止运动")
        print("custom - 输入自定义笛卡尔目标点(立即执行)")
        print("add - 添加点位到轨迹列表")
        print("show - 显示轨迹点位列表")
        print("clear - 清空轨迹点位列表")
        print("run - 按序执行轨迹点位列表")
        print("exit - 退出程序")
        
        user_input = input("请输入命令: ").strip().lower()
        
        if user_input == "start":
            print("启动JogAnyC控制!")
            send_rpcsy(client, Jog_start, timeout_ms=5000, sleep_s=1.0)
            print("机器人已执行初始动作")
            
        elif user_input == "stop":
            send_rpcsy(client, Jog_stop, timeout_ms=5000, sleep_s=1.0)
            print("运动已停止!")
            
        elif user_input == "custom":
            try:
                # 获取用户输入的笛卡尔位姿值
                cartesian_input = input(f"请输入{NUM_CARTESIAN}个笛卡尔位姿值(x,y,z,q1,q2,q3,q4，x,y,z单位:米)，用逗号分隔: ")
                cartesians = [float(x.strip()) for x in cartesian_input.split(",")]
                
                if len(cartesians) != NUM_CARTESIAN:
                    print(f"错误: 需要输入{NUM_CARTESIAN}个笛卡尔位姿值!")
                    continue
                    
                # 获取速度参数
                speed = float(input("请输入运动速度(默认1.0): ") or "1.0")
                
                # 构建自定义指令
                custom_cmd = build_jog_cmd(f"{{{','.join(map(str, cartesians))}}}", speed)
                print(f"执行指令: {custom_cmd}")
                send_rpcsy(client, [custom_cmd], timeout_ms=5000, sleep_s=1.0)
                
            except ValueError:
                print("输入格式错误! 请确保输入的是数字。")
            except Exception as e:
                print(f"发生错误: {e}")
                
        elif user_input == "add":
            try:
                # 获取用户输入的笛卡尔位姿值
                cartesian_input = input(f"请输入{NUM_CARTESIAN}个笛卡尔位姿值(x,y,z,q1,q2,q3,q4，x,y,z单位:米)，用逗号分隔: ")
                cartesians = [float(x.strip()) for x in cartesian_input.split(",")]
                
                if len(cartesians) != NUM_CARTESIAN:
                    print(f"错误: 需要输入{NUM_CARTESIAN}个笛卡尔位姿值!")
                    continue
                    
                # 获取速度参数
                speed = float(input("请输入运动速度(默认1.0): ") or "1.0")
                
                robottarget_value = f"{{{','.join(map(str, cartesians))}}}"
                trajectory_points.append((robottarget_value, speed))
                print(f"点位 {len(trajectory_points)} 已添加，当前共 {len(trajectory_points)} 个点位")
                
            except ValueError:
                print("输入格式错误! 请确保输入的是数字。")
            except Exception as e:
                print(f"发生错误: {e}")
                
        elif user_input == "show":
            print(f"当前轨迹点位数量: {len(trajectory_points)}")
            for i, (robottarget_value, speed) in enumerate(trajectory_points):
                print(f"  {i + 1}. {robottarget_value}, speed={speed}")
                
        elif user_input == "clear":
            trajectory_points.clear()
            print("轨迹点位列表已清空")
            
        elif user_input == "run":
            if not trajectory_points:
                print("错误: 没有轨迹点可执行! 请先用 add 添加点位")
                continue
                
            print(f"开始执行轨迹，共 {len(trajectory_points)} 个点位...")
            for i, (robottarget_value, speed) in enumerate(trajectory_points):
                cmd = build_jog_cmd(robottarget_value, speed)
                print(f"[{i + 1}/{len(trajectory_points)}] {cmd}")
                # 同步发送，等待到达当前点后再发下一个点
                send_rpcsy(client, [cmd], timeout_ms=30000, sleep_s=0.5)
            print("轨迹执行完成!")
            
        elif user_input == "exit":
            print("退出程序...")
            send_rpcsy(client, Jog_stop, timeout_ms=5000, sleep_s=1.0)  # 确保停止运动
            break
            
        else:
            print("未知命令，请重新输入!")

# 程序入口
if __name__ == "__main__":
    main()
