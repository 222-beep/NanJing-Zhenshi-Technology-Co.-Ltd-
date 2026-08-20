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

# ReadPdo 指令参数（可在菜单中随时修改 slave_id/index/size）
# slave_id  : 从站编号
# index     : PDO 对象索引（十六进制）
# sub_index : PDO 对象子索引（十六进制）
# size      : 读取数据位宽（bit）
# interval  : 读取间隔（s）
# loop      : 循环读取次数
slave_id = 6
pdo_index = 0x6041
pdo_size = 16


def build_readpdo_cmd() -> str:
    """用当前参数拼接 ReadPdo 指令"""
    return (f"{{ReadPdo --slave_id={slave_id} --index=0x{pdo_index:04X} --sub_index=0x00 "
            f"--size={pdo_size} --interval=1 --loop=1}}")


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
    """主函数 - ReadPdo 指令交互发送"""
    global slave_id, pdo_index, pdo_size
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # 发送初始化指令
    send_rpcsy(client, init_cmds, sleep_s=0.1, timeout_ms=500, debug=True)
    print("初始化完成")

    while True:
        print("\n=== ReadPdo 指令菜单 ===")
        print("readpdo  - 读取 PDO 数据  ")
        print("set      - 修改参数(slave_id/index/size)")
        print("exit     - 退出程序      ")
        print(f"当前参数: slave_id={slave_id}  index=0x{pdo_index:04X}  size={pdo_size}")

        user_input = input("请输入命令: ").strip()

        if user_input == "readpdo":
            # 用当前参数拼接 ReadPdo 指令，走原始 JSON 接口解析完整返回值
            if not client.is_connected():
                print(f"[SYNC] Connection lost! {client.error_info()}")
            else:
                resp_json = send_readpdo_raw(client, build_readpdo_cmd())
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
                print("[ReadPdo] 指令已发送")

        elif user_input == "set":
            param = input("请输入要修改的参数名 (slave_id/index/size): ").strip()
            value = input("请输入新值（index 支持 0x 十六进制）: ").strip()
            try:
                if param == "slave_id":
                    slave_id = int(value)
                    print(f"slave_id 已修改为: {slave_id}")
                elif param == "index":
                    pdo_index = int(value, 0)
                    print(f"index 已修改为: 0x{pdo_index:04X}")
                elif param == "size":
                    pdo_size = int(value)
                    print(f"size 已修改为: {pdo_size}")
                else:
                    print("未知参数，可修改: slave_id/index/size")
            except ValueError:
                print("输入无效，参数保持不变!")

        elif user_input == "exit":
            print("退出程序...")
            break

        else:
            print("未知命令，请重新输入!")


# 程序入口
if __name__ == "__main__":
    main()
