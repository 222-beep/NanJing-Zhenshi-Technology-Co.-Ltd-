# Topic SDK — 实时数据订阅

Topic SDK 用于订阅机器人控制器发布的实时状态数据（关节角度/力矩、笛卡尔位姿、六维力、子系统状态等），提供 **C++** 和 **Python** 双语言版本。

与标准 RPC SDK 不同，Topic SDK 使用**独立的 ZeroMQ + Protobuf 通信栈**，不依赖 `common/rpc/`，其公共库统一收口在 `common/topic/` 下。

- **订阅端口**：固定为 **19091**
- **通信方向**：单向订阅（控制器发布 → 客户端订阅），只读，不下发任何控制指令

---

## 目录结构

```
Topic_SDK/
├── README.md                       # 本文件
│
├── topic_c++/                      # C++ 版本
│   ├── CMakeLists.txt              #   构建配置（含 protoc 自动生成，缺失时降级用预生成 pb）
│   ├── API.md                      #   SystemStateReader 完整 API 文档
│   ├── proto/                      #   Protobuf 消息定义
│   │   ├── overall_system_rtstate.proto    # 实时状态（RT）
│   │   └── overall_system_nrtstate.proto   # 非实时状态（NRT）
│   ├── proto_generated/            #   预生成的 pb 文件（无 protoc 时的兜底）
│   ├── message_struct/             #   子系统 data 字段解析用结构体示例
│   │   └── message_struct.h
│   └── src/                        #   示例源码
│       ├── main.cpp                #     精简示例（默认编译目标）
│       ├── topic_sub_direct.cpp    #     全字段示例 — Direct 模式
│       └── topic_sub_snapshot.cpp  #     全字段示例 — Snapshot 模式
│
└── topic_py/                       # Python 版本
    ├── main.py                     #   精简示例（与 C++ main.cpp 对应）
    ├── API.md                      #   Python 接口 API 文档
    ├── platform_loader.py          #   平台检测 + 动态库自动加载
    ├── system_state_reader.py      #   数据读取封装（Direct/Snapshot 双模式）
    ├── readme.txt                  #   Linux 运行库说明
    └── examples/                   #   完整示例
        ├── topic_sub_direct.py     #     全字段示例 — Direct 模式
        └── topic_sub_snapshot.py   #     全字段示例 — Snapshot 模式
```

公共库位于仓库根目录的 `common/topic/`，按两层组织：

```
common/topic/
├── c++/            # C++ 专属：include 头文件 + 预编译库（message、protobuf、zmq、protoc 工具）
└── python/         # Python 专属：cp310/topic.so/pyd 扩展 + 运行时依赖的 protobuf/zmq
```

> 库文件按平台与架构分目录存放（Windows、Linux x86、Linux ARM），Linux 统一使用 Ubuntu 20.04 ABI 基线，Python 按 CPython ABI 分目录（当前仅 cp310）。**禁止在 Topic_SDK 内独立复制 include/ 和 lib/ 目录**，所有引用均通过相对路径指向 `common/topic/`。

---

## 两种数据获取方式

SDK 在 `SharedSystemState` 之上提供 `SystemStateReader` 只读访问层，支持两种调用方式：

| 方式 | 特点 | C++ 示例 | Python 示例 |
|------|------|----------|-------------|
| **Direct（自由函数）** | 一步到位，直接调函数取单个字段，内部自动获取快照 | `topic_sub_direct.cpp` | `examples/topic_sub_direct.py` |
| **Snapshot（快照）** | 先取一致性快照再逐字段读取，适合批量读取，避免重复取快照 | `topic_sub_snapshot.cpp` | `examples/topic_sub_snapshot.py` |

两种方式共享同一套字段和命名体系，所有读取操作均为线程安全。

### C++ 快速开始

```cpp
#include "system_state_reader.hpp"

start_subscriber("192.168.11.11");   // 传入发布者 IP，端口固定 19091

// 方式一：自由函数
if (hasRtData()) {
    double pos = getJointPosition(0, 2);    // 模型0, 关节2 的位置
}

// 方式二：快照
auto rt = SystemStateReader::snapshotRt();
if (rt.valid()) {
    double pos = rt.jointPosition(0, 2);
    double vel = rt.jointVelocity(0, 2);    // 同一快照内多次读取
}
```

### Python 快速开始

```python
from platform_loader import get_topic_module
topic = get_topic_module()                 # 自动加载匹配平台的 topic 扩展
topic.start_subscriber("192.168.11.11")    # 传入发布者 IP

from system_state_reader import has_rt_data, get_joint_position
if has_rt_data():
    pos = get_joint_position(0, 2)         # 模型0, 关节2 的位置
```

客户设备的 RT 数据中，`model1/joint0` 为 model 夹爪。Python 便捷接口会同时
返回米、毫米和 Topic 时间戳：

```python
from system_state_reader import get_model_gripper_state

model_gripper = get_model_gripper_state()
if model_gripper is not None:
    print(model_gripper["position_m"])
    print(model_gripper["position_mm"])
```

该接口读取的是 RT model 关节数据，与 NRT 子系统中的 `TwoFingerGripperYS`
外设夹爪接口不同。完整字段说明见 `topic_py/API.md`。

> 运行前请将示例代码中的 IP 修改为实际发布者（控制器）地址。

完整字段列表、返回值类型和参数约定见 [`topic_c++/API.md`](topic_c++/API.md)。

---

## 数据来源：RT 与 NRT

订阅数据分为两类，分别对应两个 protobuf 消息：

| 类别 | 消息 | 频率 | 内容 |
|------|------|------|------|
| **RT（实时）** | `overall_system_rtstate` | 高频 | 关节位置/速度/力矩、当前点位、笛卡尔位姿、六维力、模型运行状态 |
| **NRT（非实时）** | `overall_system_nrtstate` | 低频 | 关节限位、工具/工件/负载、示教点、从站、子系统、接口、系统初始化状态 |

读取前先用 `hasRtData()` / `hasNrtData()`（Python：`has_rt_data()` / `has_nrt_data()`）判断数据是否已到达。

### 子系统 data 字段解析

子系统（Subsystem）的 `data` 字段是原始二进制数据，库不预设数据结构，由用户按协议自定义解析：

- **C++**：定义 1 字节对齐的 POD 结构体（`#pragma pack(1)`），用 `memcpy` 或 `parseSubsystemData<T>(idx)` 解析
- **Python**：定义 `struct` 格式串（如 `'<d'`），用 `parse_subsystem_data(idx, fmt)` 解析

示例参考 `main.cpp` / `main.py` 中的 `TwoFingerGripperYSStatus`（二指夹爪到位状态）。

---

## C++ 编译

CMake 构建时会用 `protoc` 从 `proto/` 自动生成 `.pb.cc/.pb.h`（输出到 build 目录的 `generated_proto/`），无需手动执行；若环境中找不到 `protoc`，自动降级使用 `proto_generated/` 下的预生成文件。

**Windows：**

```powershell
cd Topic_SDK\topic_c++
mkdir build; cd build
cmake .. -G "Visual Studio 18 2026" -A x64
cmake --build . --config Release
```

**Linux：**

```bash
cd Topic_SDK/topic_c++
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release   # 自动检测架构（x86/arm）
make
```

说明：

- 默认编译目标为 `src/main.cpp`（精简示例）。如需编译全字段示例，修改 `CMakeLists.txt` 中的 `SET(test src/main.cpp)` 为 `src/topic_sub_direct.cpp` 或 `src/topic_sub_snapshot.cpp` 后重新构建
- Windows 平台编译后会自动将 `libprotobuf.dll`、`libprotoc.dll`、`libzmq-v142-mt-4_3_6.dll`、`message.dll` 复制到 exe 目录
- Linux 平台链接 `libprotobuf.so`、`libzmq.so`、`libmessage.so`（来自 `common/topic/c++/lib/linux/<arch>/`）

---

## Python 运行

Python 版本无需编译，直接运行：

```bash
python Topic_SDK/topic_py/main.py            # 精简示例
python Topic_SDK/topic_py/examples/topic_sub_direct.py     # 全字段 Direct 示例
python Topic_SDK/topic_py/examples/topic_sub_snapshot.py   # 全字段 Snapshot 示例
```

`platform_loader.py` 会自动检测操作系统、架构与 CPython ABI，从 `common/topic/python/lib/` 加载匹配当前平台的 `topic.pyd` / `topic.so` 及依赖库（Linux 模块位于 `linux/<arch>/cp310/`，依赖库在其上级目录，仅支持 CPython 3.10）。

### Linux 常见 ImportError

若运行时报类似错误：

```
ImportError: topic.so: undefined symbol: _ZN6google8protobuf...
```

说明运行时未优先加载匹配的 `libprotobuf.so.32`。当前版本的 `topic.so` 已内置 RUNPATH（指向依赖库所在上级目录），通常直接运行即可；如仍需手动指定：

```bash
LD_PRELOAD=../../common/topic/python/lib/linux/x86/libprotobuf.so.32 python3 main.py
```

> 注意：`libprotobuf.so`（链接用）与 `libprotobuf.so.32`（运行时加载）是两个独立文件，不可互相替换。详见 `topic_py/readme.txt`。

---

## 依赖库

| 库 | 版本 | 用途 |
|----|------|------|
| Protocol Buffers | 3.x | 数据序列化/反序列化 |
| ZMQ (libzmq) | 4.3.6+ | 消息传输 |
| message | 自定义库 | 消息总线实现 |
| pthread | — | Linux 线程库 |

### 支持的操作系统和架构

| 操作系统 | 架构 |
|----------|------|
| Windows | x64 |
| Linux | x86 / x64、ARM / ARM64（统一使用 Ubuntu 20.04 ABI 基线库） |

---

## 参考文档

| 文档 | 路径 |
|------|------|
| SystemStateReader 完整 API | `topic_c++/API.md` |
| Python 接口 API | `topic_py/API.md` |
| Linux 运行库说明 | `topic_py/readme.txt` |
| Protobuf 消息定义 | `topic_c++/proto/*.proto` |
