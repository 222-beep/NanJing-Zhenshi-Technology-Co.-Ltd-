import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

ROBOT_IP = "192.168.11.11"

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{SetUsingSP --state=on}",   # 开启最优求解器（笛卡尔空间运动适配）
    "{Recover}",
    "{Enable}",
    "{Var --clear}",
    "{Start}"
]

# JogC 运动命令
jogc_cmd = [
    # "{JogC --motion_type=（运动方式：0-x平动、1-y平动、2-z平动、3-x旋转、4-y旋转、5-z旋转）
    #         --direction=（方向：1-正方向，-1-负方向）
    #         --step=（步长）
    #         --coordinate=（坐标系：0-绝对世界坐标系，1-工具坐标系）
    #         --speed=（速度档位：v1、v5、v10、v25、v50、v100 ...）}"
    "{JogC --motion_type=0 --direction=1 --step=0.1 --coordinate=0 --speed=v100}",
    "{JogC --motion_type=0 --direction=-1 --step=0.1 --coordinate=0 --speed=v100}"
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
    #  for 循环持续发送 JogC 保持点动
    # ==================================================================
    #  send_rpc_async(client, jogc_cmd, wait_s=间隔秒, timeout_ms=超时毫秒)

    # 持续发送 10 条指令
    for _ in range(10):
        send_rpc_async(client, jogc_cmd, wait_s=0, timeout_ms=10000)
        time.sleep(0.2)


# 程序入口
if __name__ == "__main__":
    main()
