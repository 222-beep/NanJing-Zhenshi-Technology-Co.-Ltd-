import sys, os
import time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_thread

# ==================================================================
#  main  ——  使用示例
#  MoveS 主流程同步发送；Stop / SetEgmOffset 通过 send_rpc_thread
#  在独立线程中发送（见 rpc_client.py），不阻塞主流程
# ==================================================================

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Enable}",
    "{Var --clear}",
    # 定义轨迹目标点变量（笛卡尔位姿 x,y,z,q1,q2,q3,q4，x,y,z 单位：米）
    "{Var --type=robottarget --name=p1 --value={0.32,-0.32,0.48,0,1,0,0}}",
    "{Var --type=robottarget --name=p2 --value={0.38,-0.26,0.52,0,1,0,0}}",
    "{Var --type=robottarget --name=p3 --value={0.44,-0.32,0.48,0,1,0,0}}",
    "{Var --type=robottarget --name=p4 --value={0.38,-0.38,0.44,0,1,0,0}}",
    "{Var --type=robottarget --name=p5 --value={0.32,-0.32,0.48,0,1,0,0}}",
]

# MoveS 轨迹命令（同步发送）：first_insert 设置起点 -> insert 添加轨迹点 -> start 执行
moves_cmds = [
    "{MoveS --type=first_insert}",
    "{MoveS --type=insert --robottarget_var=p1}",
    "{MoveS --type=insert --robottarget_var=p2}",
    "{MoveS --type=insert --robottarget_var=p3}",
    "{MoveS --type=insert --robottarget_var=p4}",
    "{MoveS --type=insert --robottarget_var=p5}",
    "{MoveS --type=start}",
]

# 独立线程发送的新指令
stop_cmd = "{Stop}"
setegmoffset_cmd = "{SetEgmOffset --pos_offset={0.01,0,0} --max_vel=0.005 --max_acc=0.3 --max_dec=0.3 --coordinate=0}"

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
    send_rpcsy(client, moves_cmds, timeout_ms=10000, sleep_s=0.5)

    # 示例 2：独立线程发送（send_rpc_thread，见 rpc_client.py）
    # 在独立线程中发送，不阻塞当前线程，返回 Future，
    # 可在之后任意时机 .result() 获取发送结果
    # 在独立线程中发送 SetEgmOffset 进行在线偏移
    setegmoffset_future = send_rpc_thread(client, setegmoffset_cmd, timeout_ms=10000, debug=True)

    # 两个独立线程发送之间加延时，保证 SetEgmOffset 先于 Stop 到达
    time.sleep(0.5)

    # 在独立线程中发送 Stop 停止运动
    stop_future = send_rpc_thread(client, stop_cmd, timeout_ms=10000, debug=True)

    # 等待两个独立线程的发送结果
    print(f"SetEgmOffset sent: {'ok' if setegmoffset_future.result() else 'failed'}")
    print(f"Stop sent: {'ok' if stop_future.result() else 'failed'}")


# 程序入口
if __name__ == "__main__":
    main()
