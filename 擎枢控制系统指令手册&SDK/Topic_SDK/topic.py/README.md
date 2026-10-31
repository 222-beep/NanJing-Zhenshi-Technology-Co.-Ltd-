# Topic 系统状态订阅程序（Python）

基于 C++ pybind11 绑定的 Python 系统状态数据订阅客户端，通过 ZMQ 消息总线实时接收系统的实时(RT)和非实时(NRT)状态数据。

NRT 消息的底层默认上报频率为 **20 Hz**。RT 消息将在后续版本中以
**250 Hz** 作为默认上报频率；当前版本的实际接收频率以现场发布端配置和
`examples/topic_frequency.py` 的测量结果为准，并可能受到网络和客户端负载影响。

可运行 `examples/topic_frequency.py` 检测实际接收频率。统计在订阅回调中完成，
每收到一条消息计数一次，与 Python 调用 `get_*()` 的频率无关。

对应的 C++ 接收工程为 `get_status_c`。

## 项目功能

- **实时数据订阅**：接收实时系统状态，包括控制器信息、机器人关节位置/力矩、力传感器等
- **非实时数据订阅**：接收系统配置信息，包括关节限制、工具、工件、负载、示教点、IO、MatrixVariable、拖动系数、干涉区和子系统等
- **双模式访问**：提供快照方式和直接获取方式两种 API，适应不同使用场景

- **model 夹爪状态接口**：独立读取 model1 第 0 关节的位置，与外设夹爪接口区分
- **跨平台支持**：Windows (x64)、Linux (x86/ARM)，自动检测平台加载对应动态库

## 系统要求

| 项目 | 要求 |
|------|------|
| **Python** | 3.10 |
| **操作系统** | Windows 10/11 x64；Linux x86_64/AArch64（Ubuntu 20.04 ABI 基线） |
| **依赖库** | 预编译的 topic 模块及动态库（已包含在 lib/ 中） |

### 已验证的部署环境

| 平台 | OS | Python | 架构 |
|------|----|--------|------|
| **Windows** | Windows 10/11 | 3.10.x | x64 |
| **Linux (x86)** | Ubuntu 20.04.3 LTS | 3.10.11 | x86_64 |
| **Linux (ARM)** | Ubuntu 20.04.6 LTS | 3.10.x | aarch64 |

> **Linux x86_64和AArch64模块统一在Ubuntu 20.04基线环境编译，**
> 可用于相同架构的Ubuntu 20.04及更新版本。

各平台详细说明见 `lib/win/README.md`、`lib/linux/<arch>/README.md`。

## 快速开始

### 1. 安装 Python 3.10

- Windows：[python.org](https://www.python.org/downloads/) 下载安装
- Linux（x86/ARM）：安装 CPython 3.10。Ubuntu 20.04 默认提供 Python 3.8，建议将
  Python 3.10 独立安装到 `/opt/cpython310`，不要替换系统的 `/usr/bin/python3`。

### 2. 修改发布者 IP

编辑 [topic_all_py/main.py](topic_all_py/main.py)，将 `PUBLISHER_IP` 改为实际的发布者 IP：

```python
PUBLISHER_IP = "192.168.11.11"   # 修改为实际 IP
```

### 3. 运行

Linux：

```bash
cd topic_all_py
python3.10 main.py
```

Windows：

```powershell
cd topic_all_py
python main.py
```

`platform_loader.py` 会自动检测操作系统和架构，加载对应的 `lib/` 子目录中的动态库。

## 项目结构

```
get_status_py/
├── README.md
├── topic_all_py/
│   ├── main.py                  # 主程序入口（直接方式获取 jointtarget / robottarget）
│   ├── API.md                   # Python API 参考
│   ├── system_state_reader.py   # 封装层：快照方式类 + 直接获取方式便捷函数
│   ├── platform_loader.py       # 平台自动检测与动态库加载
│   ├── examples/
│   │   ├── topic_sub_direct.py  # 直接获取方式完整示例（所有字段）
│   │   ├── topic_sub_snapshot.py# 快照方式完整示例（所有字段）
│   │   └── topic_frequency.py   # RT/NRT 实际接收频率检测示例
│   └── lib/
│       ├── win/                 # Windows 库 (x64)
│       │   ├── README.md        # 环境说明
│       │   ├── libprotobuf.dll  # Protocol Buffers 动态库
│       │   ├── libzmq-v142-mt-4_3_6.dll  # ZMQ 动态库
│       │   └── cp310/
│       │       └── topic.pyd    # CPython 3.10 扩展模块
│       └── linux/
│           ├── x86/             # Ubuntu 20.04 ABI 基线 x86_64 库
│           │   ├── README.md
│           │   ├── libprotobuf.so.32
│           │   ├── libzmq.so.5
│           │   └── cp310/topic.so
│           └── arm/             # Ubuntu 20.04 ABI 基线 ARM64 库
│               ├── README.md
│               ├── libprotobuf.so.32
│               ├── libzmq.so.5
│               └── cp310/topic.so
└── PDF-old/                     # 历史 PDF 文档归档
```

## 两种访问方式

### 快照方式 (Snapshot Mode) — 推荐批量读取

先获取一份数据快照，再从快照中逐字段访问。适合一次读取多个字段，避免重复锁定。

```python
from system_state_reader import SystemStateReader

# 获取快照
rt = SystemStateReader.snapshot_rt()
nrt = SystemStateReader.snapshot_nrt()

# 从快照读取
if rt.valid():
    pos = rt.joint_position(0, 2)          # 模型0, 关节2
    name = rt.current_robottarget(0)        # 模型0 的 robottarget
    for m in range(rt.model_count()):
        for j in range(rt.joint_count(m)):
            print(rt.joint_position(m, j))
```

完整示例：[examples/topic_sub_snapshot.py](topic_all_py/examples/topic_sub_snapshot.py)

### 直接获取方式 (Direct Mode) — 适合单字段快速访问

每个 `get_*()` 函数内部自动获取快照，调用者无需关心快照管理。

```python
from system_state_reader import (
    has_rt_data, get_model_count_rt,
    get_current_robottarget, get_current_jointtarget,
)

if has_rt_data():
    for m in range(get_model_count_rt()):
        print(get_current_robottarget(m))
        print(get_current_jointtarget(m))
```

完整示例：[examples/topic_sub_direct.py](topic_all_py/examples/topic_sub_direct.py)

## 独立获取 model 夹爪状态

客户设备的 model 夹爪状态位于 RT Topic 的 `model1/joint0`。该接口只读取
model 夹爪，不会重复读取或组合机械臂位姿，也不同于 NRT 子系统中的外设夹爪：

```python
from system_state_reader import get_model_gripper_state

model_gripper = get_model_gripper_state()
if model_gripper is not None:
    print(model_gripper["position_m"])
    print(model_gripper["position_mm"])
```

`topic_all_py/main.py` 已包含 model 夹爪状态输出，直接运行：

```bash
cd topic_all_py
python3.10 main.py
```

运行前将 `main.py` 中的 `PUBLISHER_IP` 修改为实际发布者 IP。`topic_timestamp` 为
Topic 消息头的原始时间戳；单位由发布端定义。

## API 参考

完整的快照方式、直接获取方式和字段说明见 [topic_all_py/API.md](topic_all_py/API.md)。

## 数据说明

| 类型 | 更新频率 | 内容 |
|------|----------|------|
| **RT (Real-Time)** | 后续版本默认 250 Hz；当前版本以实测为准 | 关节位置/力矩/速度、力传感器、当前点、模型运行状态、控制器状态 |
| **NRT (Non-Real-Time)** | 默认 20 Hz | 关节限制、工具/工件/负载配置、示教点、从站列表、IO、MatrixVariable、拖动系数、干涉区、子系统、接口 |

RT 消息将在后续版本中以 250 Hz 作为底层默认上报频率，当前版本请以频率检测
示例的结果为准。Python 代码的读取或打印频率可根据业务需要独立设置。

实际频率检测：

```bash
cd topic_all_py
python3.10 examples/topic_frequency.py
```

示例默认统计 5 秒，并输出 RT/NRT 实际接收频率、窗口消息数、累计消息数和最新
发布端帧号。业务代码也可调用 `measure_receive_frequency(seconds)`。

输出中的 `publisher_timestamp_delta` 是发布端控制器原始计数差，单位随控制器采样
周期变化，不是固定单位的毫秒。`frame_delta` 与 `messages` 相同时，表示统计窗口内
未发现帧号跳变。

## 故障排除

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| `ImportError: No module named 'topic'` | 平台、Python ABI检测失败或缺少模块 | 检查 `lib/win/cp310/topic.pyd` 或 `lib/linux/<arch>/cp310/topic.so` |
| `libprotobuf.so.32` / `libzmq.so.5` 找不到 | 发布目录不完整 | 确保两个运行库位于对应架构目录；`topic.so` 通过 `$ORIGIN/..` 自动加载 |
| 收不到数据 | 网络不通或发布者未启动 | 检查 IP 是否正确、网络是否可达、19091 端口是否开放 |
| `DLL load failed` (Windows) | 缺少 VC 运行时 | 安装 [VC++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) |

## 最近更新

- **ver007**: 新增订阅回调频率统计与 `examples/topic_frequency.py`；NRT 默认频率更新为 20 Hz，并注明后续版本 RT 默认频率为 250 Hz
- **ver006**: Linux库统一采用Ubuntu 20.04 ABI基线，移除20.04/22.04目录层；Python扩展按CPython ABI放入`cp310/`，公共运行库保留在架构目录
- **ver005**: 同步新版 NRT proto，新增 MatrixVariable、`drag_in_cst_coef` 和 InfRng；更新 Windows、Linux x86_64、Linux ARM64 模块及 direct/snapshot 全字段示例；旧版 PDF 归档至 `PDF-old/`
- **ver004**: `lib/linux/` 目录按 Ubuntu 版本分层（`x86/{20.04,22.04}`、`arm/{20.04,22.04}`）；`platform_loader.py` 支持自动检测 Ubuntu 版本；示例文件移入 `examples/` 目录
- **ver003**: 新增 `topic_sub_snapshot.py`（快照方式示例）、`topic_sub_direct.py`（直接方式示例）；`main.py` 简化为直接方式获取 jointtarget / robottarget；子系统 `data` 字段支持 ctypes fallback 读取二进制数据
- **ver002**: 提供快照方式 (`SystemStateReader`) 和直接获取方式 (`get_*()` 函数) 双模式 API
- **ver001**: 初始版本，基于 `topic_c2py_by_pybind` 编译的 pyd/so 模块
