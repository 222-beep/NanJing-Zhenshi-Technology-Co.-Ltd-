import sys, os, time
from concurrent.futures import ThreadPoolExecutor
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_thread

# ==================================================================
#  main  ——  使用示例（运动指令同步发送，Start / Stop 用独立线程）
#  流程结构：
#    1. 定义初始化指令（不含 Start）
#    2. 连接机器人控制器
#    3. 同步发送初始化指令
#    4. 在独立线程中发送 Start（send_rpc_thread，见 rpc_client.py）
#    5. 同步发送运动指令（send_rpcsy，在主线程，不使用独立线程）
#    6. 在独立线程中发送 Stop 停止运动
#
#  说明：Start 与 Stop 都通过独立线程发送；Start 发完先 .result() 等其返回，
#  保证 Start 先于运动指令到达。主线程用 send_rpcsy 同步发送运动指令时会被
#  阻塞（直到运动执行完或被打断），因此打断用的 Stop 必须放到独立线程里发。
# ==================================================================

ROBOT_IP = "192.168.11.11"

# 初始化命令列表（同步发送）：不含 Start，Start 移到独立线程发送
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Recover}",
    "{Mode}",
    "{Enable}",
    "{Var --clear}",
    # "{Var --type=jointtarget --name=（变量名）
    #           --value={ jointtarget 共 10 位，不足补 0，单位：弧度 }}"
    "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}"
]

# MoveAbsJ 运动命令（主线程同步发送，不用独立线程）：j1 -> j2 -> j0
moveabsj_cmd = [
    # "{MoveAbsJ --jointtarget_var=（关节目标变量名，需先在 init_cmds 中通过 Var 预定义）}"
    "{MoveAbsJ --jointtarget_var=j1}",
    "{MoveAbsJ --jointtarget_var=j2}",
    "{MoveAbsJ --jointtarget_var=j0}"
]

# 独立线程发送的启动指令
start_cmd = "{Start}"

# 独立线程发送的停止指令
stop_cmd = "{Stop}"


def send_stop_after(client, cmd, delay_s):
    """独立线程中执行：先延时 delay_s 秒等机器人动起来，再发送 Stop 打断阻塞中的运动"""
    time.sleep(delay_s)
    return send_rpc_thread(client, cmd, timeout_ms=10000, debug=True).result()


def main():
    """主函数"""
    # ---- 连接机器人控制器 -------------------------------------------
    client = RpcClient(ROBOT_IP)
    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # ==================================================================
    #  示例 1：通用同步 RPC 发送初始化指令
    #  返回值只有 return_code / subcmd_index / return_message
    # ==================================================================
    #  send_rpcsy(client, cmds, sleep_s=间隔秒, timeout_ms=超时毫秒)
    send_rpcsy(client, init_cmds, sleep_s=0.1, timeout_ms=50000)

    # ==================================================================
    #  示例 2：Start / Stop 独立线程发送，运动指令主线程同步发送
    #
    #  Start 在独立线程中发送，发完先 .result() 等其返回，保证 Start 先于
    #  运动指令到达。主线程 send_rpcsy(moveabsj_cmd) 会阻塞直到运动跑完/被
    #  打断，所以把 Stop 安排到独立线程：先延时 1s 等机器人动起来，再从独立
    #  线程发 Stop 打断运动。
    # ==================================================================

    # 在独立线程中发送 Start，并等待其返回，保证 Start 先于运动指令到达
    start_future = send_rpc_thread(client, start_cmd, timeout_ms=10000, debug=True)
    print(f"Start sent: {'ok' if start_future.result() else 'failed'}")

    with ThreadPoolExecutor(max_workers=1) as executor:
        # 独立线程：延时 1s 后发送 Stop，用于打断下面阻塞中的运动
        stop_future = executor.submit(send_stop_after, client, stop_cmd, 1.0)

        # 主线程同步发送运动指令（阻塞，期间会被上面的 Stop 打断）
        #  send_rpcsy(client, cmds, sleep_s=间隔秒, timeout_ms=超时毫秒)
        send_rpcsy(client, moveabsj_cmd, sleep_s=0.2, timeout_ms=50000)

        # 等待独立线程的 Stop 发送结果
        print(f"Stop sent: {'ok' if stop_future.result() else 'failed'}")


# 程序入口
if __name__ == "__main__":
    main()
