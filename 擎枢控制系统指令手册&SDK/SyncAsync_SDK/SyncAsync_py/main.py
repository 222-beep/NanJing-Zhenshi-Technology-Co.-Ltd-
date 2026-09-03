import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

ROBOT_IP = "192.168.11.11"

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{SetUsingSP --state=on}",   # 开启最优求解器（异步示例含 SpeedL 笛卡尔运动）
    "{Recover}",
    "{Enable}",
    "{Var --clear}",
    # "{Var --type=jointtarget --name=（变量名）
    #           --value={ jointtarget 共 10 位，不足补 0，单位：弧度 }}"
    "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Start}"
]

# 同步示例指令：MoveAbsJ 依次到 j0 -> j1 -> j2
sync_cmd = [
    # "{MoveAbsJ --jointtarget_var=（关节目标变量名，需先在 init_cmds 中通过 Var 预定义）}"
    "{MoveAbsJ --jointtarget_var=j0}",
    "{MoveAbsJ --jointtarget_var=j1}",
    "{MoveAbsJ --jointtarget_var=j2}"
]

# 异步示例指令：SpeedL 在线规划往返
async_cmd = [
    # "{SpeedL --vel={ 笛卡尔速度 vx,vy,vz,wx,wy,wz }
    #           --last_count=（末尾保持周期数）}"
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}"
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
    #  执行初始化 + MoveAbsJ 同步运动序列
    # ==================================================================
    #  send_rpcsy(client, cmds, sleep_s=间隔秒, timeout_ms=超时毫秒)
    send_rpcsy(client, init_cmds, sleep_s=0.1, timeout_ms=50000)
    send_rpcsy(client, sync_cmd, sleep_s=0.1, timeout_ms=50000)

    # ==================================================================
    #  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    #  SpeedL 在线规划往返
    # ==================================================================
    #  send_rpc_async(client, cmds, wait_s=间隔秒, timeout_ms=超时毫秒)
    send_rpc_async(client, async_cmd, wait_s=0, timeout_ms=10000)


# 程序入口
if __name__ == "__main__":
    main()
