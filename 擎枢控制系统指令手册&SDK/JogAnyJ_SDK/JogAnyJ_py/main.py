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

# JogAnyJ 运动命令
joganyj_cmd = [
    # "{JogAnyJ --jointtarget_value={ 关节目标，共 10 位，不足补 0，单位：弧度 }
    #            --joint_vel=（关节速度）
    #            --joint_acc=（关节加速度）
    #            --joint_dec=（关节减速度）
    #            --last_count=（末尾保持周期数）}"
    "{JogAnyJ --jointtarget_value={0.1,-0.5,0.3,0.6,0,0,0,0,0,0} --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5 --last_count=100}",
    "{JogAnyJ --jointtarget_value={0.2,-0.4,0.2,0.5,0,0,0,0,0,0} --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5 --last_count=100}",
    "{JogAnyJ --jointtarget_value={0.3,-0.3,0.1,0.4,0,0,0,0,0,0} --joint_vel=0.1 --joint_acc=0.5 --joint_dec=0.5 --last_count=100}",
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
    #  for 循环持续发送 JogAnyJ 保持点动
    # ==================================================================
    #  send_rpc_async(client, joganyj_cmd, wait_s=间隔秒, timeout_ms=超时毫秒)

    # 持续发送 10 条指令
    for _ in range(10):
        send_rpc_async(client, joganyj_cmd, wait_s=0, timeout_ms=10000)
        time.sleep(0.2)


# 程序入口
if __name__ == "__main__":
    main()
