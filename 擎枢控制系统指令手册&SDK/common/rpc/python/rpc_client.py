"""
rpc_client.py — RPC 工具模块
封装了：平台检测、动态库路径、客户端连接、同步/异步发送
"""
import sys
import os
import platform
import ctypes
import random
import time


# ======================================================================
#  平台检测 & 动态库路径设置
# ======================================================================

def _detect_ubuntu_version():
    """从 /etc/os-release 检测 Ubuntu 版本号，如 '20.04', '22.04'。失败返回 None。"""
    try:
        with open('/etc/os-release', 'r') as f:
            for line in f:
                if line.startswith('VERSION_ID='):
                    version = line.strip().split('=')[1].strip('"')
                    parts = version.split('.')
                    if len(parts) >= 2:
                        return f"{parts[0]}.{parts[1]}"
                    return version
    except Exception:
        pass
    return None


def _get_platform_subdir():
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

        # 检测 Ubuntu 版本，构建版本化路径 lib/linux/{arch}/{version}/
        ubuntu_ver = _detect_ubuntu_version()
        versions_to_try = [ubuntu_ver] if ubuntu_ver else []
        versions_to_try += ['20.04', '22.04']  # fallback 版本列表

        for v in versions_to_try:
            candidate = os.path.join('linux', arch, v)
            if os.path.isdir(os.path.join(_root, 'lib', candidate)):
                return candidate

        raise RuntimeError(
            f"Platform directory not found: lib/linux/{arch}/{{20.04,22.04}}"
        )
    else:
        raise RuntimeError(f"Unsupported OS: {system}")


_root = os.path.dirname(os.path.abspath(__file__))
_tdir = os.path.join(_root, 'lib', _get_platform_subdir())

if not os.path.isdir(_tdir):
    raise RuntimeError(f"Platform directory not found: {_tdir}")

sys.path.insert(0, _tdir)

if platform.system() == 'Windows':
    os.environ['PATH'] = _tdir + ';' + os.environ.get('PATH', '')
    if hasattr(os, 'add_dll_directory'):
        os.add_dll_directory(_tdir)
elif platform.system() == 'Linux':
    _ld = os.environ.get('LD_LIBRARY_PATH', '')
    os.environ['LD_LIBRARY_PATH'] = _tdir + (':' + _ld if _ld else '')
    # 重新加载动态库配置（使新路径生效）
    ctypes.CDLL(None, ctypes.RTLD_GLOBAL)

import rpc


# ======================================================================
#  客户端连接（端口固定 5868，只需提供 IP）
# ======================================================================

class RpcClient:
    """RPC 客户端封装，端口固定 5868"""

    PORT = 5868

    def __init__(self, ip: str, connect_timeout_ms: int = 3000):
        self._ip = ip
        self._client = rpc.CPPClient(ip, self.PORT, connect_timeout_ms)

    @property
    def ip(self):
        return self._ip

    @property
    def inner(self):
        """获取底层 rpc.CPPClient，用于高级操作"""
        return self._client

    def is_connected(self) -> bool:
        return self._client.IsConnected()

    def error_info(self) -> str:
        return self._client.GetErrorInfo()

    def new_msg(self, cmd: str):
        """创建消息并自动设置 ID 和序列号"""
        msg = rpc.Msg(cmd)
        msg.setMsgID(10001)
        msg.setMsgSeqID(random.randint(1, 10000))
        return msg


# ======================================================================
#  同步 RPC 发送
# ======================================================================

def send_rpcsy(client: RpcClient, cmd_list: list, timeout_ms: int = 5000,
               sleep_s: float = 0):
    """
    逐条同步发送指令，等待响应后发下一条

    Args:
        client:     RpcClient 实例
        cmd_list:   指令字符串列表
        timeout_ms: 每条超时 (ms)
        sleep_s:    指令间隔 (s)
    """
    for cmd in cmd_list:
        if not client.is_connected():
            print(f"[SYNC] Connection lost! {client.error_info()}")
            break

        msg = client.new_msg(cmd)
        status, resp_list = client.inner.CallAwait(msg, timeout_ms)

        if status == 0:
            print("************* Sync ***************")
            print(f"  cmd: {cmd}")
            for r in resp_list:
                print(f"  [{r.index}] code={r.code}  {r.message}")
            print("**********************************")
        else:
            print(f"[SYNC] Failed: '{cmd}'  status={status}")
            if not client.is_connected():
                print(f"[SYNC] Connection lost: {client.error_info()}")
                break

        time.sleep(sleep_s)


# ======================================================================
#  异步 RPC 发送
# ======================================================================

def send_rpc_async(client: RpcClient, cmd_list: list, timeout_ms: int = 10000,
                   wait_s: float = 0.5):
    """
    逐条异步发送指令（不等响应）

    Args:
        client:     RpcClient 实例
        cmd_list:   指令字符串列表
        timeout_ms: 每条超时 (ms)
        wait_s:     指令间隔 (s)
    """
    def _on_response(status, resp_list):
        print("************** Async **************")
        if status < 0:
            print(f"  timeout! status={status}")
        else:
            for r in resp_list:
                print(f"  [{r.index}] code={r.code}  {r.message}")
        print("***********************************")

    for cmd in cmd_list:
        if not client.is_connected():
            print(f"[ASYNC] Connection lost! {client.error_info()}")
            break

        msg = client.new_msg(cmd)
        ok = client.inner.CallAsync(msg, timeout_ms, _on_response)
        if not ok:
            print(f"[ASYNC] Send failed: {cmd}")

        time.sleep(wait_s)

    print("[ASYNC] All commands sent.")
