import time
import threading
from platform_loader import get_topic_module

# 获取平台相关的 topic 模块
topic = get_topic_module()

import message

print_lock = threading.Lock()
def safe_print(*args, **kwargs):
    with print_lock:
        print(*args, **kwargs)

# 2. 启动订阅（传入发布者的 IP 地址，端口固定为 19091）
PUBLISHER_IP = "192.168.11.11"   # 请修改为实际发布者 IP
topic.start_subscriber(PUBLISHER_IP)
safe_print(f"Subscriber started, listening to {PUBLISHER_IP}:19091")

# 3. 主循环：定期获取并打印数据
try:
    last_rt_time = 0
    last_nrt_time = 0
    while True:
        state = topic.get_system_state()

        # 实时数据（RT）
        if state.has_rt():
            rt_data = state.get_rt()
            # 可以添加一个简单的节流，例如每秒打印一次
            now = time.time()
            if now - last_rt_time >= 1.0:
                message.print_rt(rt_data)
                last_rt_time = now
        else:
            safe_print("No RT data yet...")

        # 非实时数据（NRT）
        if state.has_nrt():
            nrt_data = state.get_nrt()
            now = time.time()
            if now - last_nrt_time >= 5.0:   # NRT 数据变化较慢，5秒打印一次
                message.print_nrt(nrt_data)
                last_nrt_time = now
        else:
            safe_print("No NRT data yet...")

        time.sleep(0.5)   # 避免循环过紧

except KeyboardInterrupt:
    safe_print("\nShutting down...")
    # 注意：新模块没有提供显式的 Shutdown 函数，因为 node 是 static 对象，程序退出时会自动清理
    safe_print("Exited.")
