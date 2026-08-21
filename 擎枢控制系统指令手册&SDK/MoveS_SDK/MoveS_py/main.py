import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Enable}",
    "{Var --clear}",
    # 定义轨迹目标点变量（笛卡尔位姿 x,y,z,q1,q2,q3,q4，x,y,z 单位：米）
    "{Var --type=robottarget --name=p1 --value={0.32,-0.32,0.48,0,1,0,0}}",
]

# MoveS 轨迹命令：first_insert 设置起点 -> insert 添加轨迹点 -> start 执行
moves_cmds = [
    "{MoveS --type=first_insert}",
    "{MoveS --type=insert --robottarget_var=p1}",
    "{MoveS --type=start}",
]

# 停止命令
stop_cmds = [
    "{Stop --last_count=10}",
]

# 用户自定义指令列表
your_cmds = [
    # 添加你的指令
]

ROBOT_IP = "192.168.11.11"

def main():
    """主函数"""
    # 创建客户端
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # 示例 1：通用同步 RPC（最常见用法）
    # 可选参数: send_rpcsy(client, cmds, timeout_ms, sleep_s)
    send_rpcsy(client, init_cmds, timeout_ms=500, sleep_s=0.1)
    send_rpcsy(client, moves_cmds, timeout_ms=10000, sleep_s=0.5)
    send_rpcsy(client, stop_cmds, timeout_ms=5000, sleep_s=1.0)

    # 示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    # 可选参数: send_rpc_async(client, cmds, timeout_ms, wait_s)
    # send_rpc_async(client, moves_cmds, timeout_ms=10000, wait_s=0.5)


# 程序入口
if __name__ == "__main__":
    main()
