import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

ROBOT_IP = "192.168.11.11"

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Recover}",
    "{Mode}",
    "{Enable}",
    "{Start}"
]

# MoveSeriesToppJ 关节轨迹命令：first_insert 设置起点 -> insert 添加轨迹点 -> start 执行
trajectory_cmd = [
    # "{MoveSeriesToppJ --type=first_insert}  设置轨迹起点
    #  {MoveSeriesToppJ --type=insert --jointtarget_value={ 关节目标，共 10 位，不足补 0，单位：弧度 }}
    #  {MoveSeriesToppJ --type=start --vel_coef=（速度系数 0~1） --acc_coef=（加速度系数 0~1）}"
    "{MoveSeriesToppJ --type=first_insert}",
    "{MoveSeriesToppJ --type=insert --jointtarget_value={0.1,-0.5,0.3,0,0,0,0,0,0,0}}",
    "{MoveSeriesToppJ --type=insert --jointtarget_value={0.2,0,0.5,-0.2,0,0,0,0,0,0}}",
    "{MoveSeriesToppJ --type=insert --jointtarget_value={0,0,0,0,0,0,0,0,0,0}}",
    "{MoveSeriesToppJ --type=start --vel_coef=0.95 --acc_coef=0.95}"
]


def main():
    """主函数"""
    # 创建客户端连接机器人控制器
    client = RpcClient(ROBOT_IP)
    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # ==================================================================
    #  示例 1：通用同步 RPC（最常见用法）
    #  返回值只有 return_code / subcmd_index / return_message
    #  执行初始化
    # ==================================================================
    #  send_rpcsy(client, init_cmds, sleep_s=间隔秒, timeout_ms=超时毫秒)
    send_rpcsy(client, init_cmds, sleep_s=0.1, timeout_ms=50000)

    # ==================================================================
    #  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    #  for 循环重复下发 MoveSeriesToppJ 轨迹序列
    # ==================================================================
    #  send_rpc_async(client, trajectory_cmd, wait_s=间隔秒, timeout_ms=超时毫秒)

    # 持续发送 10 组指令
    for _ in range(10):
        send_rpc_async(client, trajectory_cmd, wait_s=0, timeout_ms=10000)
        time.sleep(5) 


# 程序入口
if __name__ == "__main__":
    main()
