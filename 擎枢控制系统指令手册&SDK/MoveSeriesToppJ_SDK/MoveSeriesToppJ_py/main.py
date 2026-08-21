import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Enable}",
]

# MoveSeriesToppJ 关节轨迹命令：first_insert 设置起点 -> insert 添加轨迹点 -> start 执行
# jointtarget 共 10 位，不足补 0，单位：弧度
trajectory_cmds = [
    "{MoveSeriesToppJ --type=first_insert}",
    "{MoveSeriesToppJ --type=insert --jointtarget_value={0.1,-0.5,0.3,0,0,0,0,0,0,0}}",
    "{MoveSeriesToppJ --type=insert --jointtarget_value={0.2,0,0.5,-0.2,0,0,0,0,0,0}}",
    "{MoveSeriesToppJ --type=insert --jointtarget_value={0,0,0,0,0,0,0,0,0,0}}",
    "{MoveSeriesToppJ --type=start --vel_coef=0.95 --acc_coef=0.95}",
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
    send_rpcsy(client, trajectory_cmds, timeout_ms=30000, sleep_s=0.5)
    send_rpcsy(client, stop_cmds, timeout_ms=5000, sleep_s=1.0)

    # 示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    # 可选参数: send_rpc_async(client, cmds, timeout_ms, wait_s)
    # send_rpc_async(client, trajectory_cmds, timeout_ms=30000, wait_s=0.5)


# 程序入口
if __name__ == "__main__":
    main()
