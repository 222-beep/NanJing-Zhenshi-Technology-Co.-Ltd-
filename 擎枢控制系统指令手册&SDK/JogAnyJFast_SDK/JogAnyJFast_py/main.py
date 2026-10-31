import sys, os, time, math
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import (
    RpcClient,
    send_rpcsy,
    send_rpc_async,
    RealtimeRpcClient
)

# ==================================================================
#  main  ——  JogAnyJFast 使用示例
# ==================================================================

ROBOT_IP = "192.168.11.11"
# index=0时 realtime_port = 5900，index=1时 realtime_port = 5901
realtime_port = 5900
DEG2RAD = 3.1415926 / 180.0   # 度→弧度 换算系数

init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Recover}",
    "{Mode}",
    "{Enable}",
    "{Start}"
]

joganyjfast_cmd = [
    #  JogAnyJFast 参数：
    #    --joint_vel   关节速度上限
    #    --joint_acc   关节加速度上限
    #    --joint_dec   关节减速度上限
    #    --last_count  末尾保持周期数
    #    --index       0:127.0.0.1的5900端口发送的指令，1：192.168.11.11的5901端口发送的指令
    "{JogAnyJFast --joint_vel={1.0} --joint_acc={1.0} --joint_dec={1.0} --last_count=10000 --index=0}"
]

# 停止命令
stop_cmds = [
    "{Stop --last_count=10}"
]

# 目标姿态（单位：度）
ARM_TARGET_DEG = [
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0
]

SEND_COUNT = 2000             # 发送总帧数
SEND_INTERVAL_S = 0.001       # 发送周期（秒）

def main():
    # ---- 连接机器人控制器（5868：控制指令通道）----------------------
    print("Connecting 5868: ")
    client = RpcClient(ROBOT_IP)
    if not client.is_connected():
        print(f"5868 connection failed: {client.error_info()}")
        return 1
    print("Connected 5868: ")

    # ==================================================================
    #  示例 1：通用同步 RPC
    #  经 5868 执行初始化
    # ==================================================================
    send_rpcsy(client, init_cmds)
    # ==================================================================
    #  示例 2：通用异步 RPC（不等返回）
    #  经 5868 异步进入快速运动模式
    # ==================================================================
    send_rpc_async(client, joganyjfast_cmd)

    # ---- 连接 5900 \5901实时通道（关节目标帧通道）------------------------
    realtime = RealtimeRpcClient(ROBOT_IP, realtime_port)
    if not realtime.connect():
        print(f"5900 connection failed: {realtime.error_info()}")
        send_rpcsy(client, stop_cmds)
        return 2
    # ==================================================================
    #  示例 3：5900 \ 5901端口下发关节目标
    #  目标为 5×10 二维列表（rad），本示例只填第 1 组臂；
    #  每次 send 发送一帧，发送周期由调用方控制；
    #  本示例以 1ms 周期持续发送 2000 帧；
    # ==================================================================
    target = [[0.0] * 10 for _ in range(5)]
    for joint in range(10):
        target[0][joint] = ARM_TARGET_DEG[joint] * DEG2RAD  # 第 1 组机械臂
    result = 0
    for i in range(SEND_COUNT):
        result = realtime.send(target)
        if not realtime.send_succeeded(result):
            print(f"5900 \ 5901 send failed at frame {i}: {realtime.error_info()}")
            break
        time.sleep(SEND_INTERVAL_S)
    realtime.close()
    # ---- 停止运动（5868）--------------------------------------------
    send_rpcsy(client, stop_cmds)
    print("Motion stopped.")
    return 0 if result == 0 else 3
# 程序入口
if __name__ == "__main__":
    sys.exit(main())
