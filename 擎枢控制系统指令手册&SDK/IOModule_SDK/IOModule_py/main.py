import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

ROBOT_IP = "192.168.11.11"

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Recover}",
    "{Enable}",
    "{Start}"
]

# 读取数字输入
getdi_cmd = [
    # "{GetDI --di_name=（数字输入端口名，如 DI0、DI1 ...）}"
    "{GetDI --di_name=DI0}"
]

# 设置数字输出
setdo_cmd = [
    # "{SetDO --do_name=（数字输出端口名，如 DO0、DO1 ...）
    #          --do_value=（输出值：0-低电平，1-高电平）}"
    "{SetDO --do_name=DO2 --do_value=1}"
]

# 数字输出脉冲
dopulse_cmd = [
    # "{DOPulse --do_name=（数字输出端口名）
    #            --pulse_active=（有效电平：0-低，1-高）
    #            --high_cycles=（高电平持续周期数）
    #            --low_cycles=（低电平持续周期数）}"
    "{DOPulse --do_name=DO1 --pulse_active=1 --high_cycles=10 --low_cycles=10}"
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
    #  示例 2：通用同步 RPC 依次执行 IO 操作
    #  GetDI 读取数字输入 -> SetDO 设置数字输出 -> DOPulse 输出脉冲
    # ==================================================================
    #  send_rpcsy(client, cmds, sleep_s=间隔秒, timeout_ms=超时毫秒)
    send_rpcsy(client, getdi_cmd, sleep_s=0.1, timeout_ms=50000)
    send_rpcsy(client, setdo_cmd, sleep_s=0.1, timeout_ms=50000)
    send_rpcsy(client, dopulse_cmd, sleep_s=0.1, timeout_ms=50000)


# 程序入口
if __name__ == "__main__":
    main()
