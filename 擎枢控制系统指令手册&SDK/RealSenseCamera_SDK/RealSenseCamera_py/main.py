"""两路 RealSense HTTP 图像接收、预览和模型 observation 示例。"""

import os
import sys
import time

import cv2


COMMON_RPC_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..", "common", "rpc", "python")
)
if COMMON_RPC_DIR not in sys.path:
    sys.path.insert(0, COMMON_RPC_DIR)

from rpc_client import RpcClient, send_rpcsy
from realsense_stream import CodeitRealSenseStream, get_camera_observation


ROBOT_IP = "192.168.11.11"
SHOW_PREVIEW = True
PRINT_INTERVAL_S = 1.0

CAMERA_SENSORS = {
    "realsense_0": "RsCameraSensor#0#0",
    "realsense_1": "RsCameraSensor#1#0",
}


def build_camera_urls(robot_ip):
    return {
        name: f"http://{robot_ip}:6888/{sensor.replace('#', '/')}/color"
        for name, sensor in CAMERA_SENSORS.items()
    }


def set_camera_streaming(client, enabled):
    command = "ShowImageCameraSensor" if enabled else "StopShowImageCameraSensor"
    commands = [
        f"{{{command} --camera_sensor={sensor}}}"
        for sensor in CAMERA_SENSORS.values()
    ]
    send_rpcsy(client, commands, timeout_ms=10000, sleep_s=0.1, debug=True)


def print_observation(observation):
    """打印模型 observation 中每路图片的格式、时间戳和帧序号。"""
    for name, image in observation["images"].items():
        timestamp_ns = observation["image_timestamps_ns"][name]
        sequence = observation["image_sequences"][name]
        print(
            f"{name}: shape={image.shape}, dtype={image.dtype}, "
            f"timestamp_ns={timestamp_ns}, sequence={sequence}"
        )


def show_preview(images):
    """显示 RGB 图片；q 或 Esc 退出。"""
    for name, rgb in images.items():
        cv2.imshow(name, cv2.cvtColor(rgb, cv2.COLOR_RGB2BGR))
    key = cv2.waitKey(1) & 0xFF
    return key not in (ord("q"), 27)


def main():
    client = RpcClient(ROBOT_IP)
    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    camera = CodeitRealSenseStream(build_camera_urls(ROBOT_IP))
    streaming_started = False
    try:
        set_camera_streaming(client, True)
        streaming_started = True
        camera.start()
        if not camera.wait_ready(timeout_s=10.0):
            print("WARNING: not all RealSense streams are ready after 10 seconds.")

        print("Camera observation started. Press q, Esc or Ctrl+C to exit.")
        next_print_time = 0.0
        while True:
            observation = get_camera_observation(camera)
            if len(observation["images"]) == len(CAMERA_SENSORS):
                # 客户模型接入位置：
                # model_output = model(observation)
                now = time.monotonic()
                if now >= next_print_time:
                    print_observation(observation)
                    next_print_time = now + PRINT_INTERVAL_S

                if SHOW_PREVIEW and not show_preview(observation["images"]):
                    break
            else:
                time.sleep(0.01)
    except KeyboardInterrupt:
        print("\nStopping ...")
    finally:
        camera.stop()
        cv2.destroyAllWindows()
        if streaming_started and client.is_connected():
            set_camera_streaming(client, False)


if __name__ == "__main__":
    main()
