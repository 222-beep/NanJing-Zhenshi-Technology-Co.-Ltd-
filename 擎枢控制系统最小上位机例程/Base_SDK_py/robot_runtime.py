import os
import sys


ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
ROBOT_STATE_DIR = os.path.join(ROOT_DIR, "robot_state")

INIT_CMDS = [
    "{Clear}",
    "{Mode}",
    "{SetMaxToq}",
    "{Recover}",
    "{SetRate}",
]

RPC_TIMEOUT_MS = 5000
PRINT_INTERVAL_S = 1.0

for path in (ROBOT_STATE_DIR,):
    if path not in sys.path:
        sys.path.insert(0, path)

from robot_command.rpc_client import RpcClient, send_rpcsy, send_rpc_async
from robot_state.platform_loader import get_topic_module


def start_robot_state(ip):
    """启动机器人状态订阅。"""
    state_module = get_topic_module()
    state_module.start_subscriber(ip)


def connect_robot_command(ip):
    """连接机器人指令客户端。"""
    client = RpcClient(ip)
    if not client.is_connected():
        raise RuntimeError(f"Robot command connection failed: {client.error_info()}")
    return client


def initialize_controller(client):
    """下发控制器初始化指令，例如 Clear/Enable 等。"""
    send_rpc_async(client, INIT_CMDS, timeout_ms=RPC_TIMEOUT_MS, wait_s=0.02)


def initialize_system(ip):
    """完成状态订阅、指令连接和控制器初始化。"""
    start_robot_state(ip)
    client = connect_robot_command(ip)
    initialize_controller(client)
    return client
