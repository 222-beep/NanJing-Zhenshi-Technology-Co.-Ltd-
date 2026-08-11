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
        if machine in ('x86_64', 'amd64'):
            arch = 'x86'
        elif machine in ('aarch64', 'arm64'):
            arch = 'arm'
        else:
            raise RuntimeError(f"Unsupported Linux architecture: {machine}")
        return os.path.join('linux', arch, _get_python_abi_dir())
    else:
        raise RuntimeError(f"Unsupported OS: {system}")



def _get_python_abi_dir():
    """返回当前CPython ABI目录，例如CPython 3.10对应cp310。"""
    if sys.implementation.name != 'cpython':
        raise RuntimeError(
            f"Unsupported Python implementation: {sys.implementation.name}; "
            "only CPython is supported"
        )
    return f"cp{sys.version_info.major}{sys.version_info.minor}"


def _setup_rpc_import():
    """设置 rpc 模块的导入路径"""
    base_dir = os.path.dirname(os.path.abspath(__file__))
    target_dir = os.path.join(base_dir, 'lib', _get_platform_subpath())

    if not os.path.isdir(target_dir):
        raise RuntimeError(
            f"RPC library directory not found for the current architecture and "
            f"Python ABI: {target_dir}"
        )

    if platform.system() == 'Linux':
        rpc_library = os.path.join(target_dir, 'rpc.so')
        if not os.path.isfile(rpc_library):
            raise RuntimeError(f"RPC library not found: {rpc_library}")

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


_THREAD_SEND_EXECUTOR = ThreadPoolExecutor(max_workers=1, thread_name_prefix="rpc-send")


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
               timeout_ms: int = 5000, debug: bool = False,
               response_callback=None):
    """
    逐条同步发送指令，等待响应后发下一条

    Args:
        client:   RpcClient 实例
        cmd_list: 指令字符串列表
        sleep_s:    指令间隔 (s)
        timeout_ms: 每条超时 (ms)
        debug:      是否打印发送和返回信息
        response_callback: 可选回调，签名为 callback(status, resp_list, seq_id, cmd)
    """
    all_responses = []
    for cmd in cmd_list:
        if not client.is_connected():
            print(f"[SYNC] Connection lost! {client.error_info()}")
            break

        msg, seq_id = client._new_msg_with_seq(cmd)
        if debug:
            print(f"\nsend[seq={seq_id}]: {cmd}")
        status, resp_list = client.inner.CallAwait(msg, timeout_ms)
        if response_callback:
            response_callback(status, resp_list, seq_id, cmd)

        if status == 0:
            all_responses.extend(resp_list)
            if debug:
                print(f"************* Sync[seq={seq_id}] ***************")
                for r in resp_list:
                    print(f"  [{r.index}] code={r.code}  {r.message}")
                print("***********************************************")
        else:
            if debug:
                print(f"[SYNC] Failed: '{cmd}'  status={status}")
            if not client.is_connected():
                print(f"[SYNC] Connection lost: {client.error_info()}")
                break

        time.sleep(sleep_s)
    return all_responses


# ======================================================================
#  异步 RPC 发送
# ======================================================================

def send_rpc_async(client: RpcClient, cmd_list: list, wait_s: float = 0.5,
                   timeout_ms: int = 10000, debug: bool = False,
                   response_callback=None):
    """
    逐条异步发送指令（不等响应）

    Args:
        client:   RpcClient 实例
        cmd_list: 指令字符串列表
        wait_s:    指令间隔 (s)
        timeout_ms: 每条超时 (ms)
        debug:      是否打印发送和返回信息
        response_callback: 可选回调，签名为 callback(status, resp_list, seq_id, cmd)
    """
    for cmd in cmd_list:
        if not client.is_connected():
            print(f"[ASYNC] Connection lost! {client.error_info()}")
            break

        msg, seq_id = client._new_msg_with_seq(cmd)
        if debug:
            print(f"\nsend[seq={seq_id}]: {cmd}")

        def _on_response(status, resp_list, _seq=seq_id, _cmd=cmd):
            if response_callback:
                response_callback(status, resp_list, _seq, _cmd)
            if debug:
                print(f"************** Async[seq={_seq}] **************")
                if status < 0:
                    print(f"  timeout! status={status}")
                else:
                    for r in resp_list:
                        print(f"  [{r.index}] code={r.code}  {r.message}")
                print("**********************************************")

        ok = client.inner.CallAsync(msg, timeout_ms, _on_response)
        if not ok:
            if not client.is_connected():
                print(f"[ASYNC] Connection lost! {client.error_info()}")
            elif debug:
                print(f"[ASYNC] Send failed: {cmd}. {client.error_info()}")

        time.sleep(wait_s)

    if debug:
        print("[ASYNC] All commands sent.")

# ======================================================================
#  独立线程通用 RPC
# ======================================================================

def send_rpc_thread(client: RpcClient, cmd: str, timeout_ms: int = 10000,
                    debug: bool = False, response_callback=None) -> Future:
    """
    在独立线程中发送任意指令。

    普通同步 RPC 等待返回时，调用线程会被阻塞。本接口可从其他线程、
    按钮回调或控制入口调用，返回的 Future 结果为指令是否成功提交到
    底层异步发送接口。response_callback 可选，签名为
    callback(status, raw_response, seq_id, cmd)。
    """
    def _send() -> bool:
        if not client.is_connected():
            print(f"[THREAD] Connection lost! Command not sent: {cmd}. {client.error_info()}")
            return False

        msg, seq_id = client._new_msg_with_seq(cmd)
        if debug:
            print(f"\nsend thread[seq={seq_id}]: {cmd}")

        def _on_response(status, raw_response, _seq=seq_id, _cmd=cmd):
            if response_callback:
                response_callback(status, raw_response, _seq, _cmd)
            if debug:
                print(f"************** Thread[seq={_seq}] **************")
                if status < 0:
                    print(f"  timeout! status={status}")
                print(f"response: {raw_response}")
                print("*********************************************")

        ok = client.inner.CallAsyncRaw(msg, timeout_ms, _on_response)
        if not ok:
            if not client.is_connected():
                print(f"[THREAD] Connection lost! Command not sent: {cmd}. {client.error_info()}")
            elif debug:
                print(f"[THREAD] Failed to send command: {cmd}. {client.error_info()}")
        return ok

    return _THREAD_SEND_EXECUTOR.submit(_send)






