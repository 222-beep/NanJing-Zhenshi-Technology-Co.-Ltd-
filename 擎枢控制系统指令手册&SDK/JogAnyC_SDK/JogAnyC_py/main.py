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

# JogAnyC 运动命令
joganyc_cmd = [
    # "{JogAnyC --robottarget_value={ 笛卡尔位姿 x,y,z,q1,q2,q3,q4，x/y/z 单位：米 }
    #            --cartesian_vel={ 笛卡尔速度 }
    #            --cartesian_acc={ 笛卡尔加速度 }
    #            --cartesian_dec={ 笛卡尔减速度 }}"
    "{JogAnyC --robottarget_value={0.6,0.1,0.64,-0.5,0.5,-0.5,0.5} --cartesian_vel={1.0} --cartesian_acc={1.0} --cartesian_dec={1.0}}"
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
    #  for 循环持续发送 JogAnyC 保持点动
    # ==================================================================
    #  send_rpc_async(client, joganyc_cmd, wait_s=间隔秒, timeout_ms=超时毫秒)

    # 持续发送 10 条指令
    for _ in range(10):
        send_rpc_async(client, joganyc_cmd, wait_s=0, timeout_ms=10000)
        time.sleep(0.2)


# 程序入口
if __name__ == "__main__":
    main()
