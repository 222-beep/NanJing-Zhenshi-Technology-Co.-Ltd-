import arch

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Enable}",
]

ROBOT_IP = "192.168.2.199"

def main():
    """主函数"""
    # 创建客户端
    client = arch.create_client(ROBOT_IP)
    
    # 发送初始化指令
    arch.send_rpcsy(client, init_cmds, 500, 0.1)
    
    # 存储轨迹点
    trajectory_points = []
    current_trajectory_type = None  # "line" 或 "circle"
    
    # 主循环
    while True:
        print("\n=== MoveBlend 轨迹控制 ===")
        print("first_insert  - 添加起点（当前位置）")
        print("add_line      - 添加直线轨迹点")
        print("add_circle    - 添加圆弧轨迹点（需要中间点）")
        print("start         - 执行轨迹")
        print("clear_points  - 清除所有轨迹点")
        print("show_points   - 显示当前轨迹点")
        print("exit          - 退出程序")
        
        user_input = input("请输入命令: ").strip().lower()
        
        if user_input == "first_insert":
            # 设置起点
            print("设置起点（当前位置）")
            start_cmd = "{MoveBlend --type=first_insert}"
            arch.send_rpcsy(client, [start_cmd], 5000, 0.5)
            trajectory_points = []# 存储用户添加的轨迹点
            current_trajectory_type = "line"
            print("起点已设置")
            
        elif user_input == "add_line":
            try:
                # 获取目标点坐标
                print("请输入目标点坐标和姿态（x,y,z,q1,q2,q3,q4）：")
                print("示例：0.32,-0.32,0.48,0,1,0,0")
                target_input = input("目标点: ")
                values = [float(x.strip()) for x in   target_input.split(",")]
                
                if len(values) != 7:
                    print("错误: 需要7个值（x,y,z,q1,q2,q3,q4）!")
                    continue
                    
                # 获取zone参数
                zone_input = input("请输入zone值（如 0.1,0.1 或输入默认值）: ").strip()
                if zone_input == "":
                    zone = "{0.1,0.1}"
                else:
                    zone_values = [float(x.strip()) for x in zone_input.split(",")]
                    if len(zone_values) != 2:
                        zone = "{0.1,0.1}"
                    else:
                        zone = "{" + f"{zone_values[0]},{zone_values[1]}" + "}"

                # 获取speed参数
                speed_input = input("请输入speed值（如 v50，回车默认v50）: ").strip()
                speed = speed_input if speed_input else "v50"
                
                # 定义目标点变量
                point_name = f"p{len(trajectory_points) + 1}"
                var_cmd = f"{{Var --type=robottarget --name={point_name} --value={{{','.join(map(str, values))}}}}}"
                arch.send_rpcsy(client, [var_cmd], 5000, 0.2)
                
                # 添加直线轨迹点
                line_cmd = f"{{MoveBlend --type=insert_line --robottarget_var={point_name} --zone={zone} --speed={speed}}}"
                arch.send_rpcsy(client, [line_cmd], 5000, 0.5)
                
                trajectory_points.append({
                    "name": point_name,
                    "type": "line",
                    "values": values,
                    "zone": zone,
                    "speed": speed
                })
                print(f"直线轨迹点 {point_name} 已添加")
                
            except ValueError:
                print("输入格式错误! 请确保输入的是数字。")
            except Exception as e:
                print(f"发生错误: {e}")
                
        elif user_input == "add_circle":
            try:
                # 获取中间点和目标点
                print("请输入中间点坐标和姿态（x,y,z,q1,q2,q3,q4）：")
                mid_input = input("中间点: ")
                mid_values = [float(x.strip()) for x in mid_input.split(",")]
                
                print("请输入目标点坐标和姿态（x,y,z,q1,q2,q3,q4）：")
                target_input = input("目标点: ")
                target_values = [float(x.strip()) for x in target_input.split(",")]
                
                if len(mid_values) != 7 or len(target_values) != 7:
                    print("错误: 需要7个值（x,y,z,q1,q2,q3,q4）!")
                    continue
                    
                # 获取zone参数
                zone_input = input("请输入zone值（如 0.1,0.1 或输入默认值）: ").strip()
                if zone_input == "":
                    zone = "{0.1,0.1}"
                else:
                    zone_values = [float(x.strip()) for x in zone_input.split(",")]
                    if len(zone_values) != 2:
                        zone = "{0.1,0.1}"
                    else:
                        zone = "{" + f"{zone_values[0]},{zone_values[1]}" + "}"

                # 获取speed参数
                speed_input = input("请输入speed值（如 v50，回车默认v50）: ").strip()
                speed = speed_input if speed_input else "v50"
                
                # 定义中间点和目标点变量
                mid_name = f"mid{len(trajectory_points) + 1}"
                target_name = f"target{len(trajectory_points) + 1}"
                
                var_mid_cmd = f"{{Var --type=robottarget --name={mid_name} --value={{{','.join(map(str, mid_values))}}}}}"
                var_target_cmd = f"{{Var --type=robottarget --name={target_name} --value={{{','.join(map(str, target_values))}}}}}"
                
                arch.send_rpcsy(client, [var_mid_cmd, var_target_cmd], 5000, 0.2)
                
                # 添加圆弧轨迹点
                circle_cmd = f"{{MoveBlend --type=insert_circle --mid_robottarget_var={mid_name} --robottarget_var={target_name} --zone={zone} --speed={speed}}}"
                arch.send_rpcsy(client, [circle_cmd], 5000, 0.5)
                
                trajectory_points.append({
                    "name": target_name,
                    "type": "circle",
                    "mid_point": mid_values,
                    "target_point": target_values,
                    "zone": zone,
                    "speed": speed
                })
                print(f"圆弧轨迹点 {target_name} 已添加")
                
            except ValueError:
                print("输入格式错误! 请确保输入的是数字。")
            except Exception as e:
                print(f"发生错误: {e}")
                
        elif user_input =="start":
            if len(trajectory_points) == 0:
                print("错误: 没有轨迹点可执行!")
                continue
                
            print(f"开始执行轨迹，共 {len(trajectory_points)} 个轨迹点...")
            execute_cmd = "{MoveBlend --type=start}"
            arch.send_rpcsy(client, [execute_cmd], 10000, 1.0)
            print("轨迹执行完成!")
            
        elif user_input == "clear_points":
            # 清除所有变量
            clear_var_cmd = "{Var --clear}"
            arch.send_rpcsy(client, [clear_var_cmd], 5000, 0.5)
            trajectory_points = []
            print("所有轨迹点已清除")
            
        elif user_input == "show_points":
            print(f"当前轨迹点数量: {len(trajectory_points)}")
            for i, point in enumerate(trajectory_points):
                if point["type"] == "line":
                    print(f"  {i+1}. 直线点 {point['name']}: {point['values']}, zone={point['zone']}, speed={point.get('speed', 'N/A')}")
                elif point["type"] == "circle":
                    print(f"  {i+1}. 圆弧点 {point['name']}: 中间点={point['mid_point']}, 目标点={point['target_point']}, zone={point['zone']}, speed={point.get('speed', 'N/A')}")
                    
        elif user_input == "exit":
            print("退出程序...")
            # 发送停止指令
            stop_cmd = "{Stop --last_count=10}"
            arch.send_rpcsy(client, [stop_cmd], 5000, 1.0)
            break
            
        else:
            print("未知命令，请重新输入!")

# 程序入口
if __name__ == "__main__":
    main()
