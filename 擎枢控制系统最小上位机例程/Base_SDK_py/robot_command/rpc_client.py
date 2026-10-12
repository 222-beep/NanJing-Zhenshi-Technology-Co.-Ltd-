"""
rpc_client.py — RPC 工具模块
封装了：平台检测、动态库路径、客户端连接、同步/异步发送
"""
import sys
import os
import platform
import json
import time
import threading
from dataclasses import dataclass
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
DEFAULT_RPC_TIMEOUT_MS = 864000000


@dataclass(frozen=True)
class RpcTimingInfo:
    """一条RPC请求的响应时间信息。"""
    seq_id: int
    born_time_ms: int
    return_time_ms: int
    header_delta_ms: int | None
    client_rtt_ms: float
    server_time_valid: bool


def _make_timing_info(seq_id, response_msg, request_started_ns,
                      status=0, request_born_time=0):
    # 超时或发送失败时，底层返回的默认Msg不代表原请求。
    if status == 0:
        born_time = response_msg.msgBornTime()
        return_time = response_msg.msgReturnTime()
    else:
        born_time = request_born_time
        return_time = 0
    valid = born_time > 0 and return_time > 0 and return_time != 202401
    return RpcTimingInfo(
        seq_id=seq_id,
        born_time_ms=born_time,
        return_time_ms=return_time,
        header_delta_ms=return_time - born_time if valid else None,
        client_rtt_ms=(time.monotonic_ns() - request_started_ns) / 1_000_000.0,
        server_time_valid=valid,
    )


def _print_timing_info(timing):
    print(f"  seq_id={timing.seq_id}")
    print(f"  born_time_ms={timing.born_time_ms}")
    print(f"  return_time_ms={timing.return_time_ms}")
    delta = timing.header_delta_ms if timing.server_time_valid else "unavailable"
    print(f"  header_delta_ms={delta}")
    print(f"  client_rtt_ms={timing.client_rtt_ms:.3f}")


def _decode_comm_responses(body):
    data = json.loads(body)
    if not isinstance(data, list):
        raise ValueError("RPC response must be a JSON array")
    responses = []
    for item in data:
        response = rpc.CommResp()
        response.code = item["return_code"]
        response.index = item["subcmd_index"]
        response.message = item["return_message"]
        responses.append(response)
    return responses


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

    def call_await_raw_msg(
            self, msg: rpc.Msg,
            timeout_ms: int = DEFAULT_RPC_TIMEOUT_MS):
        """同步发送并返回(status, 完整响应Msg)，保留响应头时间戳。"""
        if not hasattr(self._client, "CallAwaitRawMsg"):
            raise RuntimeError(
                "The loaded rpc extension does not support CallAwaitRawMsg; "
                "rebuild rpc.so/rpc.pyd from the updated pybind project"
            )
        return self._client.CallAwaitRawMsg(msg, timeout_ms)

    def call_async_raw_msg(self, msg: rpc.Msg, callback,
                           timeout_ms: int = DEFAULT_RPC_TIMEOUT_MS) -> bool:
        """异步发送；通过callback(status, 完整响应Msg)获取响应头时间戳。"""
        if not hasattr(self._client, "CallAsyncRawMsg"):
            raise RuntimeError(
                "The loaded rpc extension does not support CallAsyncRawMsg; "
                "rebuild rpc.so/rpc.pyd from the updated pybind project"
            )
        return self._client.CallAsyncRawMsg(msg, timeout_ms, callback)


# ======================================================================
#  5900实时关节目标通道
# ======================================================================

class RealtimeRpcClient:
    """
    5900实时关节目标发送封装。

    该端口不接受普通字符串指令，只接受固定的5×10浮点关节目标数组。
    每次send()只发送一帧，发送周期由调用方控制。使用前需通过5868普通
    RPC启动JogAnyJFast并设置--index=0。目标需要持续刷新，停止发送后不应
    认为机械臂会继续运动到目标位置。
    """

    PORT = 5900
    ARM_COUNT = 5
    JOINT_COUNT = 10

    def __init__(self, ip: str, port: int = PORT,
                 connect_timeout_ms: int = 2000,
                 send_buffer_size: int = 0,
                 send_timeout_ms: int = 100):
        if not hasattr(rpc, "RealtimeChannel"):
            raise RuntimeError(
                "The loaded rpc extension does not support the 5900 realtime "
                "channel; rebuild rpc.so/rpc.pyd from the updated pybind project"
            )
        self._channel = rpc.RealtimeChannel(
            ip, port, connect_timeout_ms, send_buffer_size, send_timeout_ms)

    def connect(self) -> bool:
        return self._channel.Connect()

    def close(self):
        self._channel.Close()

    def is_connected(self) -> bool:
        return self._channel.IsConnected()

    def error_info(self) -> str:
        return self._channel.LastError()

    def send(self, joint_target_pos):
        """
        发送一帧关节目标。

        joint_target_pos必须为5×10的二维数值序列：外层5项对应机械臂，
        每项10个double兼容数值对应关节。返回rpc.RealtimeSendResult。
        """
        if len(joint_target_pos) != self.ARM_COUNT:
            raise ValueError("joint_target_pos must contain exactly 5 arms")
        if any(len(arm) != self.JOINT_COUNT for arm in joint_target_pos):
            raise ValueError("each arm must contain exactly 10 joints")
        return self._channel.Send(joint_target_pos)

    @staticmethod
    def send_succeeded(result) -> bool:
        """判断send()的返回结果是否表示发送成功。"""
        return result == rpc.RealtimeSendResult.Ok

    def __enter__(self):
        if not self.connect():
            raise ConnectionError(self.error_info())
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
        return False


# ======================================================================
#  同步 RPC 发送
# ======================================================================

def send_rpcsy(client: RpcClient, cmd_list: list, sleep_s: float = 0,
               timeout_ms: int = DEFAULT_RPC_TIMEOUT_MS, debug: bool = False,
               response_callback=None, timing_callback=None):
    """
    逐条同步发送指令，等待响应后发下一条

    Args:
        client:   RpcClient 实例
        cmd_list: 指令字符串列表
        sleep_s:    指令间隔 (s)
        timeout_ms: 每条超时 (ms)
        debug:      是否打印发送和返回信息
        response_callback: 可选回调，签名为 callback(status, resp_list, seq_id, cmd)
        timing_callback: 可选回调，签名为 callback(RpcTimingInfo)
    """
    all_responses = []
    for cmd in cmd_list:
        if not client.is_connected():
            print(f"[SYNC] Connection lost! {client.error_info()}")
            break

        msg, seq_id = client._new_msg_with_seq(cmd)
        request_born_time = msg.msgBornTime()
        if debug:
            print(f"\nsend[seq={seq_id}]: {cmd}")
        request_started_ns = time.monotonic_ns()
        status, response_msg = client.call_await_raw_msg(msg, timeout_ms)
        timing = _make_timing_info(
            seq_id, response_msg, request_started_ns,
            status, request_born_time)
        resp_list = []
        if status == 0:
            try:
                resp_list = _decode_comm_responses(response_msg.toString())
            except (KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
                status = -1
                if debug:
                    print(f"[SYNC] JSON parse error: {exc}")
        if response_callback:
            response_callback(status, resp_list, seq_id, cmd)
        if timing_callback:
            timing_callback(timing)

        if status == 0:
            all_responses.extend(resp_list)
            if debug:
                print(f"************* Sync[seq={seq_id}] ***************")
                _print_timing_info(timing)
                for r in resp_list:
                    print(f"  [{r.index}] code={r.code}  {r.message}")
                print("***********************************************")
        else:
            if debug:
                print(f"[SYNC] Failed: '{cmd}'  status={status}")
                _print_timing_info(timing)
            if not client.is_connected():
                print(f"[SYNC] Connection lost: {client.error_info()}")
                break

        if sleep_s > 0:
            time.sleep(sleep_s)
    return all_responses


# ======================================================================
#  异步 RPC 发送
# ======================================================================

def send_rpc_async(client: RpcClient, cmd_list: list, wait_s: float = 0.5,
                   timeout_ms: int = DEFAULT_RPC_TIMEOUT_MS, debug: bool = False,
                   response_callback=None, timing_callback=None):
    """
    逐条异步发送指令（不等响应）

    Args:
        client:   RpcClient 实例
        cmd_list: 指令字符串列表
        wait_s:    指令间隔 (s)
        timeout_ms: 每条超时 (ms)
        debug:      是否打印发送和返回信息
        response_callback: 可选回调，签名为 callback(status, resp_list, seq_id, cmd)
        timing_callback: 可选回调，签名为 callback(RpcTimingInfo)
    """
    for cmd in cmd_list:
        if not client.is_connected():
            print(f"[ASYNC] Connection lost! {client.error_info()}")
            break

        msg, seq_id = client._new_msg_with_seq(cmd)
        request_born_time = msg.msgBornTime()
        if debug:
            print(f"\nsend[seq={seq_id}]: {cmd}")

        request_started_ns = time.monotonic_ns()

        def _on_response(status, response_msg, _seq=seq_id, _cmd=cmd,
                         _started_ns=request_started_ns,
                         _born_time=request_born_time):
            timing = _make_timing_info(
                _seq, response_msg, _started_ns, status, _born_time)
            resp_list = []
            if status == 0:
                try:
                    resp_list = _decode_comm_responses(response_msg.toString())
                except (KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
                    status = -1
                    if debug:
                        print(f"[ASYNC] JSON parse error: {exc}")
            if response_callback:
                response_callback(status, resp_list, _seq, _cmd)
            if timing_callback:
                timing_callback(timing)
            if debug:
                print(f"************** Async[seq={_seq}] **************")
                _print_timing_info(timing)
                if status < 0:
                    print(f"  timeout! status={status}")
                else:
                    for r in resp_list:
                        print(f"  [{r.index}] code={r.code}  {r.message}")
                print("**********************************************")

        ok = client.call_async_raw_msg(msg, _on_response, timeout_ms)
        if not ok:
            if not client.is_connected():
                print(f"[ASYNC] Connection lost! {client.error_info()}")
            elif debug:
                print(f"[ASYNC] Send failed: {cmd}. {client.error_info()}")

        if wait_s > 0:
            time.sleep(wait_s)

    if debug:
        print("[ASYNC] All commands sent.")

# ======================================================================
#  独立线程通用 RPC
# ======================================================================

def send_rpc_thread(client: RpcClient, cmd: str,
                    timeout_ms: int = DEFAULT_RPC_TIMEOUT_MS,
                    debug: bool = False, response_callback=None,
                    timing_callback=None) -> Future:
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
        request_born_time = msg.msgBornTime()
        if debug:
            print(f"\nsend thread[seq={seq_id}]: {cmd}")

        request_started_ns = time.monotonic_ns()

        def _on_response(status, response_msg, _seq=seq_id, _cmd=cmd,
                         _started_ns=request_started_ns,
                         _born_time=request_born_time):
            timing = _make_timing_info(
                _seq, response_msg, _started_ns, status, _born_time)
            raw_response = response_msg.toString()
            if response_callback:
                response_callback(status, raw_response, _seq, _cmd)
            if timing_callback:
                timing_callback(timing)
            if debug:
                print(f"************** Thread[seq={_seq}] **************")
                _print_timing_info(timing)
                if status < 0:
                    print(f"  timeout! status={status}")
                print(f"response: {raw_response}")
                print("*********************************************")

        ok = client.call_async_raw_msg(msg, _on_response, timeout_ms)
        if not ok:
            if not client.is_connected():
                print(f"[THREAD] Connection lost! Command not sent: {cmd}. {client.error_info()}")
            elif debug:
                print(f"[THREAD] Failed to send command: {cmd}. {client.error_info()}")
        return ok

    return _THREAD_SEND_EXECUTOR.submit(_send)






