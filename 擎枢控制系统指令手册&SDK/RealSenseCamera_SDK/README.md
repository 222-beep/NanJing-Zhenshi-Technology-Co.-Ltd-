# RealSenseCamera SDK（Python）

通过 codeit-deploy 的 HTTP MJPEG 接口获取两路 RealSense 彩色图像。该模块为
Python 独立示例，不使用相机 Topic，也不包含机械臂或夹爪状态。

## 环境要求

- Linux x86/x64
- CPython 3.10
- 机器人控制器与客户电脑网络互通
- codeit-deploy 已配置两路 `RsCameraSensor`

安装 Python 依赖：

```bash
cd RealSenseCamera_SDK/RealSenseCamera_py
python3.10 -m pip install -r requirements.txt
```

## 运行

修改 `main.py` 中的 `ROBOT_IP`，然后运行：

```bash
python3.10 main.py
```

程序通过公共 RPC 库发送 `ShowImageCameraSensor`，读取以下两路 HTTP 流：

```text
http://<robot-ip>:6888/RsCameraSensor/0/0/color
http://<robot-ip>:6888/RsCameraSensor/1/0/color
```

按 `q`、`Esc` 或 `Ctrl+C` 退出。退出时会停止本地接收线程，并发送
`StopShowImageCameraSensor`。

## 将图片提供给模型

```python
from realsense_stream import CodeitRealSenseStream, get_camera_observation

camera = CodeitRealSenseStream(camera_urls)
camera.start()
camera.wait_ready(timeout_s=10.0)

observation = get_camera_observation(camera)
image0 = observation["images"]["realsense_0"]
image1 = observation["images"]["realsense_1"]
```

每张图片均为 `RGB/HWC/numpy.uint8`，可直接交给客户的预处理或模型代码。
模块同时返回每路图像的本机接收时间戳和递增帧序号：

```python
observation["image_timestamps_ns"]["realsense_0"]
observation["image_sequences"]["realsense_0"]
```

每路相机只保留最新帧，不会因模型推理速度低于相机帧率而积压旧图片。两路 HTTP
连接独立接收，因此时间戳不保证完全一致；如模型需要严格的双目同步，应在模型接入
层根据时间戳进行筛选。
