import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

ROBOT_IP = "192.168.11.11"

# 初始化命令列表 - 双臂版本
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Recover}",
    "{Mode}",
    "{Enable}",
    "{Var --clear}",
    # "{Var --type=jointtarget --name=（变量名）
    #           --value={ jointtarget 共 10 位，不足补 0，单位：弧度 }}"
    # 机械臂 1 关节目标变量
    "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    # 机械臂 2 关节目标变量
    "{Var --type=jointtarget --name=j11 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j21 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j22 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Start}"
]

# 双臂 MoveAbsJ 指令使用 || 分隔机器人1和机器人2
moveabsj_cmd = [
    # "{MoveAbsJ --jointtarget_var=（机器人1关节目标变量名）
    #  ||MoveAbsJ --jointtarget_var=（机器人2关节目标变量名）}"
    "{MoveAbsJ --jointtarget_var=j1||MoveAbsJ --jointtarget_var=j21}",
    "{MoveAbsJ --jointtarget_var=j2||MoveAbsJ --jointtarget_var=j22}",
    "{MoveAbsJ --jointtarget_var=j0||MoveAbsJ --jointtarget_var=j11}"
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
    #  for 循环持续发送双臂 MoveAbsJ
    #  依次运动到 (j1,j21) -> (j2,j22) -> (j0,j11)
    # ==================================================================
    #  send_rpc_async(client, moveabsj_cmd, wait_s=间隔秒, timeout_ms=超时毫秒)

    # 持续发送 10 组指令
    for _ in range(10):
        send_rpc_async(client, moveabsj_cmd, wait_s=0, timeout_ms=10000)
        time.sleep(0.2)


# 程序入口
if __name__ == "__main__":
    main()
