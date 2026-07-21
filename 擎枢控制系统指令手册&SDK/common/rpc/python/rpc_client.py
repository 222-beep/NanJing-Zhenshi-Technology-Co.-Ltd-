"""
rpc_client.py — RPC 工具模块
封装了：平台检测、动态库路径、客户端连接、同步/异步发送
"""
import sys
import os
import platform
import time
import threading
from concurrent.futures import Future, ThreadPoolExecutor


# ======================================================================
#  平台检测 & 动态库路径设置
# ======================================================================

def _get_platform_subpath():
    """返回 lib 下的平台子目录"""
    system = platform.system().lower()
    if system == 'windows':
        return 'win'
    elif system == 'linux':
        machine = platform.machine().lower()
        if machine in ('x86_64', 'amd64', 'i386', 'i686'):
            arch = 'x86'
        elif machine in ('armv7l', 'aarch64', 'arm64'):
            arch = 'arm'
        else:
            raise RuntimeError(f"Unsupported Linux architecture: {machine}")
        return os.path.join('linux', arch, _get_linux_version_dir(arch))
    else:
        raise RuntimeError(f"Unsupported OS: {system}")



def _get_linux_version_dir(arch):
    """返回 Linux 库的 Ubuntu 版本目录：2004 / 2204"""
    override = os.environ.get('RPC_LINUX_VERSION')
    if override in ('2004', '2204'):
        return override

    default_by_arch = {
        'arm': '2204',
        'x86': '2004',
    }

    try:
        with open('/etc/os-release', 'r', encoding='utf-8') as f:
            for line in f:
                if line.startswith('VERSION_ID='):
                    version = line.split('=', 1)[1].strip().strip('"')
                    if version.startswith('20.04'):
                        return '2004'
                    if version.startswith('22.04'):
                        return '2204'
                    break
    except OSError:
        pass

    return default_by_arch[arch]


def _setup_rpc_import():
    """设置 rpc 模块的导入路径"""
    base_dir = os.path.dirname(os.path.abspath(__file__))
    target_dir = os.path.join(base_dir, 'lib', _get_platform_subpath())

    if not os.path.isdir(target_dir):
        raise RuntimeError(f"Platform directory not found: {target_dir}")

    if target_dir not in sys.path:
        sys.path.insert(0, target_dir)

    if platform.system() == 'Windows':
        os.environ['PATH'] = target_dir + ';' + base_dir + ';' + os.environ.get('PATH', '')
        if hasattr(os, 'add_dll_directory'):
            os.add_dll_directory(target_dir)
            os.add_dll_directory(base_dir)
    elif platform.system() == 'Linux':
        ld_path = target_dir + ':' + base_dir
        current_ld = os.environ.get('LD_LIBRARY_PATH', '')
        if current_ld:
            ld_path = ld_path + ':' + current_ld
        os.environ['LD_LIBRARY_PATH'] = ld_path


# 模块加载时自动执行路径设置
_setup_rpc_import()
import rpc


_STOP_EXECUTOR = ThreadPoolExecutor(max_workers=1, thread_name_prefix="rpc-stop")


# ======================================================================
#  客户端连接（端口固定 5868，只需提供 IP）
# ======================================================================

class RpcClient:
    """RPC 客户端封装，端口固定 5868"""

    PORT = 5868

    def __init__(self, ip: str, connect_timeout_ms: int = 3000):
        self._ip = ip
        self._seq_id = 0
        self._seq_lock = threading.Lock()
        self._client = rpc.CPPClient(ip, self.PORT, connect_timeout_ms)

    @property
    def ip(self):
        return self._ip

    @property
    def seq_id(self):
        """获取当前消息序列号（只读，由 new_msg() 自动递增）"""
        return self._seq_id

    @property
    def inner(self) -> rpc.CPPClient:
        """获取底层 rpc.CPPClient，用于高级操作"""
        return self._client

    def is_connected(self) -> bool:
        return self._client.IsConnected()

    def error_info(self) -> str:
        return self._client.GetErrorInfo()

    def _new_msg_with_seq(self, cmd: str):
        """创建消息并返回本次消息序列号。"""
        msg = rpc.Msg(cmd)
        msg.setMsgID(10001)
        with self._seq_lock:
            self._seq_id += 1
            seq_id = self._seq_id
        msg.setMsgSeqID(seq_id)
        return msg, seq_id

    def new_msg(self, cmd: str) -> rpc.Msg:
        """创建消息并自动设置 ID 和序列号"""
        msg, _ = self._new_msg_with_seq(cmd)
        return msg


# ======================================================================
#  同步 RPC 发送
# ======================================================================

def send_rpcsy(client: RpcClient, cmd_list: list, sleep_s: float = 0,
               timeout_ms: int = 5000):
    """
    逐条同步发送指令，等待响应后发下一条

    Args:
        client:   RpcClient 实例
        cmd_list: 指令字符串列表
        sleep_s:    指令间隔 (s)
        timeout_ms: 每条超时 (ms)
    """
    for cmd in cmd_list:
        if not client.is_connected():
            print(f"[SYNC] Connection lost! {client.error_info()}")
            break

        msg, seq_id = client._new_msg_with_seq(cmd)
        print(f"\nsend[seq={seq_id}]: {cmd}")
        status, resp_list = client.inner.CallAwait(msg, timeout_ms)

        if status == 0:
            print(f"************* Sync[seq={seq_id}] ***************")
            for r in resp_list:
                print(f"  [{r.index}] code={r.code}  {r.message}")
            print("***********************************************")
        else:
            print(f"[SYNC] Failed: '{cmd}'  status={status}")
            if not client.is_connected():
                print(f"[SYNC] Connection lost: {client.error_info()}")
                break

        time.sleep(sleep_s)


# ======================================================================
#  异步 RPC 发送
# ======================================================================

def send_rpc_async(client: RpcClient, cmd_list: list, wait_s: float = 0.5,
                   timeout_ms: int = 10000):
    """
    逐条异步发送指令（不等响应）

    Args:
        client:   RpcClient 实例
        cmd_list: 指令字符串列表
        wait_s:    指令间隔 (s)
        timeout_ms: 每条超时 (ms)
    """
    for cmd in cmd_list:
        if not client.is_connected():
            print(f"[ASYNC] Connection lost! {client.error_info()}")
            break

        msg, seq_id = client._new_msg_with_seq(cmd)
        print(f"\nsend[seq={seq_id}]: {cmd}")

        def _on_response(status, resp_list, _seq=seq_id):
            print(f"************** Async[seq={_seq}] **************")
            if status < 0:
                print(f"  timeout! status={status}")
            else:
                for r in resp_list:
                    print(f"  [{r.index}] code={r.code}  {r.message}")
            print("**********************************************")

        ok = client.inner.CallAsync(msg, timeout_ms, _on_response)
        if not ok:
            print(f"[ASYNC] Send failed: {cmd}")

        time.sleep(wait_s)

    print("[ASYNC] All commands sent.")

# ======================================================================
#  Stop 专用快速下发
# ======================================================================

def send_stop_async(client: RpcClient, timeout_ms: int = 10000) -> Future:
    """
    在独立线程中快速下发 {Stop} 指令。

    普通同步 RPC 等待返回时，调用线程会被阻塞。Stop 属于打断类指令，
    可从其他线程、按钮回调或控制入口调用本函数，返回的 Future 结果为
    Stop 是否成功提交到底层异步发送接口。
    """
    def _send_stop() -> bool:
        if not client.is_connected():
            print(f"[STOP] Connection lost! Stop command not sent. {client.error_info()}")
            return False

        msg, seq_id = client._new_msg_with_seq("{Stop}")
        print(f"\nsend stop[seq={seq_id}]: {{Stop}}")

        def _on_response(status, raw_response, _seq=seq_id):
            print(f"************** Stop[seq={_seq}] **************")
            if status < 0:
                print(f"  timeout! status={status}")
            print(f"response: {raw_response}")
            print("*********************************************")

        ok = client.inner.CallAsyncRaw(msg, timeout_ms, _on_response)
        if not ok:
            print("[STOP] Failed to send stop command.")
        return ok

    return _STOP_EXECUTOR.submit(_send_stop)






