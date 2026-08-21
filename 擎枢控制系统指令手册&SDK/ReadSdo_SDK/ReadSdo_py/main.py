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

# ReadSdo 指令参数（可自行修改）
# slave_id  : 从站编号（从 0 开始，0 通常代表网络中第一个从站）
# index     : 对象字典主索引（十六进制）
# sub_index : 对象字典子索引（十六进制）
# size      : 字节长度，指定要读取的数据大小
# loop      : 循环次数，指令命令执行的总次数
readsdo_cmds = [
    "{ReadSdo --slave_id=5 --index=0x6064 --sub_index=0x00 --size=4 --loop=1}"
]

# 用户自定义指令列表
your_cmds = [
    # 添加你的指令
]


def send_readsdo_raw(client: RpcClient, cmd: str, timeout_ms: int = 5000):
    """用 CallAwaitRaw 同步发送 ReadSdo，返回原始 JSON（含 sdo_value 等全部字段）"""
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
        print(f"[ReadSdo] 响应解析失败: {body}")
        return None


def main():
    """主函数"""
    # 创建客户端
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # 示例 1：通用同步 RPC（最常见用法）
    # ReadSdo 走原始 JSON 接口解析扩展返回值（sdo_value）
    send_rpcsy(client, init_cmds, sleep_s=0.1, timeout_ms=500)

    for cmd in readsdo_cmds:
        resp_json = send_readsdo_raw(client, cmd)
        if isinstance(resp_json, list):
            for r in resp_json:
                print(f"[ReadSdo] subcmd_index: {r.get('subcmd_index')}")
                print(f"[ReadSdo] return_code: {r.get('return_code')}")
                print(f"[ReadSdo] return_message: {r.get('return_message')}")
                if 'sdo_value' in r:
                    # 兼容数字和 "0x..." 字符串两种返回形式
                    try:
                        v = int(r['sdo_value'], 0) if isinstance(r['sdo_value'], str) else int(r['sdo_value'])
                        print(f"[ReadSdo] sdo_value: {v} (0x{v:X})")
                    except (ValueError, TypeError):
                        print(f"[ReadSdo] sdo_value: {r['sdo_value']}")


# 程序入口
if __name__ == "__main__":
    main()
