import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

ROBOT_IP = "192.168.11.11"

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{SetUsingSP --state=on}",   # 开启最优求解器（笛卡尔空间运动适配）
    "{Enable}",
    "{Start}"
]

# 切换到 CST 拖拽模式（拖拽前单独同步下发一次）
switch_cst_cmd = [
    "{SwitchToCST}"
]

# DragInCST 拖拽命令
dragincst_cmd = [
    # "{DragInCST --cf_coef={ 力控系数，共 7 位 }
    #              --vf_coef={ 速度反馈系数，共 7 位 }
    #              --vel_limit={ 各方向速度限制，共 7 位，单位 m/s 或 rad/s }
    #              --ping_pong_amp=（乒乓幅度）
    #              --zero_check=（零漂检测阈值）}"
    "{DragInCST --cf_coef={0,0,0,0,0,0,0} --vf_coef={0,0,0,0,0,0,0} --vel_limit={0.3,0.3,0.3,0.3,0.3,0.3,0.3} --ping_pong_amp=0 --zero_check=0.004}"
]

# 退出 CST 拖拽，恢复到 CSP 位置模式
exit_cmds = [
    "{Stop --last_count=10}",
    "{SwitchToCSP}",
    "{Recover}",
    "{Start}"
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

    # 切换到 CST 拖拽模式（同步下发一次）
    send_rpcsy(client, switch_cst_cmd, sleep_s=0.1, timeout_ms=50000)

    # ==================================================================
    #  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    #  for 循环持续发送 DragInCST 保持拖拽状态
    # ==================================================================
    #  send_rpc_async(client, dragincst_cmd, wait_s=间隔秒, timeout_ms=超时毫秒)

    # 持续发送 10 组指令
    for _ in range(10):
        send_rpc_async(client, dragincst_cmd, wait_s=0, timeout_ms=10000)
        time.sleep(0.2)

    # ==================================================================
    #  退出 CST 拖拽模式，恢复到 CSP 位置模式
    # ==================================================================
    send_rpcsy(client, exit_cmds, sleep_s=0.1, timeout_ms=50000)


# 程序入口
if __name__ == "__main__":
    main()
