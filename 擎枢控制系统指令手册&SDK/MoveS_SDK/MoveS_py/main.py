import arch

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Enable}",
]

ROBOT_IP = "192.168.2.217"

def main():
    """主函数 - MoveS 轨迹控制"""
    # 创建客户端
    client = arch.create_client(ROBOT_IP)
    
    # 发送初始化指令
    arch.send_rpcsy(client, init_cmds, 500, 0.1)
    
    # 存储轨迹点
    trajectory_points = []
    
    # 主循环
    while True:
        print("\n=== MoveS 轨迹控制 ===")
        print("start_moves    - 开始MoveS轨迹（设置起点）")
        print("add_moves      - 添加MoveS轨迹点")
        print("execute        - 执行轨迹")
        print("clear_points   - 清除所有轨迹点")
        print("show_points    - 显示当前轨迹点")
        print("exit           - 退出程序")
        
        user_input = input("请输入命令: ").strip().lower()
        
        if user_input == "start_moves":
            # 设置MoveS起点
            print("设置MoveS起点（当前位置）")
            start_cmd = "{MoveS --type=first_insert}"
            arch.send_rpcsy(client, [start_cmd], 5000, 0.5)
            trajectory_points = []  # 清空轨迹点列表
            print("MoveS起点已设置")
            
        elif user_input == "add_moves":
            try:
                # 获取目标点坐标姿态（x,y,z,q1,q2,q3,q4）
                print("请输入目标点坐标和姿态（x,y,z,q1,q2,q3,q4）：")
                print("示例：0.32,-0.32,0.48,0,1,0,0")
                target_input = input("目标点: ")
                values = [float(x.strip()) for x in target_input.split(",")]
                
                if len(values) != 7:
                    print("错误: 需要7个值（x,y,z,q1,q2,q3,q4）!")
                    continue

                # 定义目标点变量
                point_name = f"p{len(trajectory_points) + 1}"
                var_cmd = f"{{Var --type=robottarget --name={point_name} --value={{{','.join(map(str, values))}}}}}"
                arch.send_rpcsy(client, [var_cmd], 5000, 0.2)
                
                # 添加MoveS轨迹点指令
                move_cmd = f"{{MoveS --type=insert --robottarget_var={point_name}}}"
                arch.send_rpcsy(client, [move_cmd], 5000, 0.5)
                
                # 记录轨迹点
                trajectory_points.append({
                    "name": point_name,
                    "type": "moves",
                    "values": values
                })
                print(f"MoveS轨迹点 {point_name} 已添加")
                
            except ValueError:
                print("输入格式错误! 请确保输入的是数字。")
            except Exception as e:
                print(f"发生错误: {e}")
                
        elif user_input == "execute":
            if len(trajectory_points) == 0:
                print("错误: 没有轨迹点可执行!")
                continue
                
            print(f"开始执行MoveS轨迹，共 {len(trajectory_points)} 个轨迹点...")
            execute_cmd = "{MoveS --type=start}"
            arch.send_rpcsy(client, [execute_cmd], 10000, 1.0)
            print("MoveS轨迹执行完成!")
            
        elif user_input == "clear_points":
            # 清除所有变量
            clear_var_cmd = "{Var --clear}"
            arch.send_rpcsy(client, [clear_var_cmd], 5000, 0.5)
            trajectory_points = []
            print("所有MoveS轨迹点已清除")
            
        elif user_input == "show_points":
            print(f"当前MoveS轨迹点数量: {len(trajectory_points)}")
            for i, point in enumerate(trajectory_points):
                print(f"  {i+1}. MoveS点 {point['name']}: {point['values']}")
                    
        elif user_input == "exit":
            print("退出程序...")
            # 发送停止指令
            stop_cmd = "{Stop --last_count=10}"
            arch.send_rpcsy(client, [stop_cmd], 5000, 1.0)
            break
            
        else:
            print("未知命令，请重新输入!")


if __name__ == "__main__":
    main()
