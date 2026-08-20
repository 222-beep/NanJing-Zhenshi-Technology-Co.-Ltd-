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

# ReadSdo 指令参数（可在菜单中随时修改 slave_id/index/sub_index/size）
# slave_id  : 从站编号（从 0 开始，0 通常代表网络中第一个从站）
# index     : 对象字典主索引（十六进制）
# sub_index : 对象字典子索引（十六进制）
# size      : 字节长度，指定要读取的数据大小
# loop      : 循环次数，指令命令执行的总次数
slave_id = 5
sdo_index = 0x6064
sdo_sub_index = 0x00
sdo_size = 4


def build_readsdo_cmd() -> str:
    """用当前参数拼接 ReadSdo 指令"""
    return (f"{{ReadSdo --slave_id={slave_id} --index=0x{sdo_index:04X} "
            f"--sub_index=0x{sdo_sub_index:02X} --size={sdo_size} --loop=1}}")


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
    """主函数 - ReadSdo 指令交互发送"""
    global slave_id, sdo_index, sdo_sub_index, sdo_size
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # 发送初始化指令
    send_rpcsy(client, init_cmds, sleep_s=0.1, timeout_ms=500, debug=True)
    print("初始化完成")

    while True:
        print("\n=== ReadSdo 指令菜单 ===")
        print("readsdo  - 读取 SDO 数据  ")
        print("set      - 修改参数(slave_id/index/sub_index/size)")
        print("exit     - 退出程序      ")
        print(f"当前参数: slave_id={slave_id}  index=0x{sdo_index:04X}  sub_index=0x{sdo_sub_index:02X}  size={sdo_size}")

        user_input = input("请输入命令: ").strip()

        if user_input == "readsdo":
            # 用当前参数拼接 ReadSdo 指令，走原始 JSON 接口解析完整返回值
            if not client.is_connected():
                print(f"[SYNC] Connection lost! {client.error_info()}")
            else:
                resp_json = send_readsdo_raw(client, build_readsdo_cmd())
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
                print("[ReadSdo] 指令已发送")

        elif user_input == "set":
            param = input("请输入要修改的参数名 (slave_id/index/sub_index/size): ").strip()
            value = input("请输入新值（index/sub_index 支持 0x 十六进制）: ").strip()
            try:
                if param == "slave_id":
                    slave_id = int(value)
                    print(f"slave_id 已修改为: {slave_id}")
                elif param == "index":
                    sdo_index = int(value, 0)
                    print(f"index 已修改为: 0x{sdo_index:04X}")
                elif param == "sub_index":
                    sdo_sub_index = int(value, 0)
                    print(f"sub_index 已修改为: 0x{sdo_sub_index:02X}")
                elif param == "size":
                    sdo_size = int(value)
                    print(f"size 已修改为: {sdo_size}")
                else:
                    print("未知参数，可修改: slave_id/index/sub_index/size")
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
