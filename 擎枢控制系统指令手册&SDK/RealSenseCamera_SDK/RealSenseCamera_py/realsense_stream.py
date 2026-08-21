"""从 codeit-deploy HTTP MJPEG 接口接收 RealSense 彩色图像。"""

from __future__ import annotations

import threading
import time
import urllib.error
import urllib.request

import cv2
import numpy as np


class CodeitRealSenseStream:
    """后台接收多路 Codeit RealSense MJPEG 流并保存每路最新帧。"""

    def __init__(self, camera_urls, timeout_s=5.0):
        if not camera_urls:
            raise ValueError("camera_urls must not be empty")
        self.camera_urls = dict(camera_urls)
        self.timeout_s = float(timeout_s)
        self._frames = {}
        self._sequences = {name: 0 for name in self.camera_urls}
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._threads = []

    @property
    def camera_names(self):
        return list(self.camera_urls)

    @staticmethod
    def _take_jpeg(buffer):
        start = buffer.find(b"\xff\xd8")
        if start < 0:
            return None, buffer[-200000:]
        end = buffer.find(b"\xff\xd9", start + 2)
        if end < 0:
            return None, buffer[start:]
        end += 2
        return buffer[start:end], buffer[end:]

    def _publish_jpeg(self, name, jpeg):
        bgr = cv2.imdecode(np.frombuffer(jpeg, dtype=np.uint8), cv2.IMREAD_COLOR)
        if bgr is None:
            return
        rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
        with self._lock:
            self._sequences[name] += 1
            self._frames[name] = {
                "image": rgb,
                "timestamp_ns": time.time_ns(),
                "sequence": self._sequences[name],
            }

    def _camera_loop(self, name, url):
        while not self._stop.is_set():
            try:
                request = urllib.request.Request(
                    url,
                    headers={"User-Agent": "codeit-python-camera/1.0"},
                )
                with urllib.request.urlopen(request, timeout=self.timeout_s) as response:
                    buffer = b""
                    while not self._stop.is_set():
                        chunk = response.read(8192)
                        if not chunk:
                            break
                        buffer += chunk
                        while True:
                            jpeg, buffer = self._take_jpeg(buffer)
                            if jpeg is None:
                                break
                            self._publish_jpeg(name, jpeg)
                        if len(buffer) > 2000000:
                            buffer = buffer[-200000:]
            except (urllib.error.URLError, TimeoutError, OSError) as exc:
                if not self._stop.is_set():
                    print(f"[camera] {name} disconnected: {exc}")
                self._stop.wait(0.5)

    def start(self):
        if self._threads:
            return
        self._stop.clear()
        for name, url in self.camera_urls.items():
            thread = threading.Thread(
                name=f"camera-{name}",
                target=self._camera_loop,
                args=(name, url),
                daemon=True,
            )
            thread.start()
            self._threads.append(thread)

    def wait_ready(self, timeout_s=10.0):
        deadline = time.monotonic() + float(timeout_s)
        expected = set(self.camera_urls)
        while time.monotonic() < deadline:
            with self._lock:
                ready = set(self._frames)
            if expected.issubset(ready):
                return True
            time.sleep(0.05)
        return False

    def get_frames(self):
        """返回每路最新 RGB/HWC/uint8 图像、接收时间戳和帧序号。"""
        with self._lock:
            return {
                name: {
                    "image": frame["image"].copy(),
                    "timestamp_ns": frame["timestamp_ns"],
                    "sequence": frame["sequence"],
                }
                for name, frame in self._frames.items()
            }

    def stop(self):
        self._stop.set()
        for thread in self._threads:
            thread.join(timeout=1.0)
        self._threads.clear()


def get_camera_observation(camera):
    """返回适合模型读取的最新多相机 observation。"""
    frames = camera.get_frames()
    return {
        "images": {
            name: frame["image"]
            for name, frame in frames.items()
        },
        "image_timestamps_ns": {
            name: frame["timestamp_ns"]
            for name, frame in frames.items()
        },
        "image_sequences": {
            name: frame["sequence"]
            for name, frame in frames.items()
        },
    }
