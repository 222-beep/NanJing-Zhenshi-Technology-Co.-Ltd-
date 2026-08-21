import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Mode}",
    "{SetMaxToq}",
    "{Recover}",
    "{SetRate}",
    "{Enable}",
    "{Var --clear}",
    "{Recover}",
    "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j3 --value={-0.1,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j4 --value={-0.2,0,0,0,0,0,0,0,0,0}}",
]

# 同步示例指令（MoveAbsJ 依次到 j0~j4）
sync_cmds = [
    "{MoveAbsJ --jointtarget_var=j0}",
    "{MoveAbsJ --jointtarget_var=j1}",
    "{MoveAbsJ --jointtarget_var=j2}",
    "{MoveAbsJ --jointtarget_var=j3}",
    "{MoveAbsJ --jointtarget_var=j4}",
]

# 异步示例指令（SpeedL 往返 + Stop/Start）
async_cmds = [
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
    "{Stop}",
    "{Start}",
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
    send_rpcsy(client, sync_cmds, timeout_ms=10000, sleep_s=0.5)

    # 示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    # 可选参数: send_rpc_async(client, cmds, timeout_ms, wait_s)
    send_rpc_async(client, async_cmds, timeout_ms=10000, wait_s=0.5)


# 程序入口
if __name__ == "__main__":
    main()
