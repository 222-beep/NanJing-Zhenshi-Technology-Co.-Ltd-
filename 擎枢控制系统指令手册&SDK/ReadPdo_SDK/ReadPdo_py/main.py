import sys, os
import json
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy

ROBOT_IP = "192.168.11.11"

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Mode}",
    "{SetMaxToq}",
    "{Recover}",
    "{SetRate}",
    "{Enable}",
]

# ReadPdo 指令参数（可自行修改）
# slave_id  : 从站编号
# index     : PDO 对象索引（十六进制）
# sub_index : PDO 对象子索引（十六进制）
# size      : 读取数据位宽（bit）
# interval  : 读取间隔（s）
# loop      : 循环读取次数
readpdo_cmds = [
    "{ReadPdo --slave_id=6 --index=0x6041 --sub_index=0x00 --size=16 --interval=1 --loop=1}"
]

# 用户自定义指令列表
your_cmds = [
    # 添加你的指令
]


def send_readpdo_raw(client: RpcClient, cmd: str, timeout_ms: int = 5000):
    """用 CallAwaitRaw 同步发送 ReadPdo，返回原始 JSON（含 pdo_value 等全部字段）"""
    msg, seq_id = client._new_msg_with_seq(cmd)
    print(f"\nsend[seq={seq_id}]: {cmd}")
    status, raw_msg = client.inner.CallAwaitRaw(msg, timeout_ms)
    if status != 0:
        print(f"[SYNC] Failed: '{cmd}'  status={status}")
        if not client.is_connected():
            print(f"[SYNC] Connection lost: {client.error_info()}")
        return None

    # Python 版 CallAwaitRaw 第二个返回值直接是响应体字符串
    body = raw_msg
    if isinstance(body, (bytes, bytearray)):
        body = bytes(body).decode('utf-8', errors='replace')
    elif hasattr(body, 'data'):
        data = body.data()
        body = bytes(data).decode('utf-8', errors='replace') if isinstance(data, (bytes, bytearray)) else str(data)
    try:
        return json.loads(body)
    except (json.JSONDecodeError, TypeError):
        print(f"[ReadPdo] 响应解析失败: {body}")
        return None


def main():
    """主函数"""
    # 创建客户端
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # 示例 1：通用同步 RPC（最常见用法）
    # ReadPdo 走原始 JSON 接口解析扩展返回值（pdo_value）
    send_rpcsy(client, init_cmds, sleep_s=0.1, timeout_ms=500)

    for cmd in readpdo_cmds:
        resp_json = send_readpdo_raw(client, cmd)
        if isinstance(resp_json, list):
            for r in resp_json:
                print(f"[ReadPdo] subcmd_index: {r.get('subcmd_index')}")
                print(f"[ReadPdo] return_code: {r.get('return_code')}")
                print(f"[ReadPdo] return_message: {r.get('return_message')}")
                if 'pdo_value' in r:
                    # 兼容数字和 "0x..." 字符串两种返回形式
                    try:
                        v = int(r['pdo_value'], 0) if isinstance(r['pdo_value'], str) else int(r['pdo_value'])
                        print(f"[ReadPdo] pdo_value: {v} (0x{v:X})")
                    except (ValueError, TypeError):
                        print(f"[ReadPdo] pdo_value: {r['pdo_value']}")


# 程序入口
if __name__ == "__main__":
    main()
