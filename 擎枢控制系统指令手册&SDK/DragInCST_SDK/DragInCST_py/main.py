import time
import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy

ROBOT_IP = "192.168.11.11"

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Recover}",
    "{Mode}",
    "{Enable}",
    "{SetUsingSP --state=on}",   # 开启最优求解器（笛卡尔空间运动适配）
    "{Start}"
]

# 切换到 CST 拖拽模式（先清除运动记录，再设置全部轴及最大力矩）
switch_cst_cmd = [
    "{GetDefaultVfCf}", 
    "{DeleteMotionRecord}",
    "{SwitchToCST --all --max_torque=1000}"
]

# DragInCST 拖拽命令
dragincst_cmd = [
    "{DragInCST --cbfSwitch={0} --cutoff_freq=20.0 --gravity_cof={1.0} --ping_pong=100 --ping_pong_amp=0.5 --vel_limit={0.1} --zero_check={0.0001}}"
]

# 退出 CST 拖拽，恢复到 CSP 位置模式
# 顺序：Stop 停止拖拽 → Sleep 等待停止生效 → SwitchToCSP 切回位置模式
#       → Recover 同步模型与实际状态 → Start 恢复运动控制
exit_cmds = [
    "{Stop --last_count=10}",
    "{Sleep --count=20}",
    "{SwitchToCSP --all --max_torque=1000}",
    "{Recover}",
    "{Start --last_count=50}"
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
    #  send_rpcsy(client, init_cmds, sleep_s=间隔秒)
    send_rpcsy(client, init_cmds, sleep_s=0.1)

    # 切换到 CST 拖拽模式（同步下发一次）
    send_rpcsy(client, switch_cst_cmd, sleep_s=0.1)

    # ==================================================================
    #  示例 2：通用同步 RPC
    #  DragInCST 只需同步发送一次，随后等待 Ctrl+C 退出拖拽
    # ==================================================================
    #  send_rpcsy(client, dragincst_cmd, sleep_s=间隔秒)

    try:
        # DragInCST 只需发送一次，控制器会持续保持拖拽状态
        send_rpcsy(client, dragincst_cmd, sleep_s=0.1)
        print("DragInCST started. Press Ctrl+C to exit.")

        # 保持程序运行；Ctrl+C 会抛出 KeyboardInterrupt 跳出循环
        # 不要在循环里一直重复发 DragInCST
        while True:
            time.sleep(0.2)

    except KeyboardInterrupt:
        # Ctrl+C 触发 KeyboardInterrupt，结束等待并进入 finally 退出流程
        print("\nCtrl+C detected. Exit drag...")

    finally:
        # finally 保证 Ctrl+C 或异常后都尝试退出拖拽，避免模式残留
        # ==================================================================
        #  Ctrl+C 后退出 CST 拖拽模式，恢复到 CSP 位置模式
        # ==================================================================
        try:
            # 正常 Ctrl+C 或上面异常后，都尝试发送退出序列
            send_rpcsy(client, exit_cmds, sleep_s=0.1)
            print("Exit commands sent.")
        except Exception as exc:
            print(f"Failed to send exit commands: {exc}")


# 程序入口
if __name__ == "__main__":
    main()
