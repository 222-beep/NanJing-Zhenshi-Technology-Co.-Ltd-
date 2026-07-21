# 机器人控制 SDK 示例集

本仓库包含 **14 个机器人控制 SDK 示例模块**，每个模块均提供 C++ 和 Python 双语言版本，通过 RPC（Remote Procedure Call）协议与机器人控制器通信，可实现关节运动、笛卡尔运动、力控、IO 控制、数据订阅等功能。

---

## 目录结构

```
代码/
├── README.md                              # 本文件
│
├── common/
│   ├── rpc/                                  # RPC 公共库（所有标准 SDK 的通信基础）
│   │   ├── c++/                              # C++ RPC 库
│   │   │   ├── include/                      #   头文件
│   │   │   │   ├── cpp_rpc.hpp               #     RPC 底层客户端
│   │   │   │   ├── msg.hpp                   #     消息类型
│   │   │   │   ├── resp_dto.h                #     响应数据类型定义
│   │   │   │   ├── util.hpp                  #     工具函数
│   │   │   │   ├── wepoll.h                  #     Windows epoll 模拟
│   │   │   │   ├── message/                  #     消息层封装
│   │   │   │   │   └── rpc_client.h          #       RPC 同步/异步发送（主入口）
│   │   │   │   └── util/reflection/          #     JSON 反射
│   │   │   │       ├── json.hpp              #         nlohmann/json
│   │   │   │       └── easy_json.h           #         简易 JSON 工具
│   │   │   └── lib/                          #   预编译库
│   │   │       ├── win/Release/              #     Windows: cpp_rpc.dll/.lib
│   │   │       └── linux/                    #     Linux
│   │   │           ├── x86/2004/Release/     #       x86 (Ubuntu 20.04): libcpp_rpc.so
│   │   │           └── arm/2204/Release/     #       ARM (Ubuntu 22.04): libcpp_rpc.so
│   │   └── python/                           # Python RPC 库
│   │       ├── rpc_client.py                 #   RPC 客户端封装（RpcClient, send_rpcsy, send_rpc_async）
│   │       ├── __init__.py
│   │       └── lib/                          #   平台动态库
│   │           ├── win/                      #     Windows: rpc.pyd
│   │           └── linux/                    #     Linux
│   │               ├── x86/2004/             #       x86 (Ubuntu 20.04): rpc.so
│   │               └── arm/2204/             #       ARM (Ubuntu 22.04): rpc.so
│   │
│   └── topic/                                # Topic 公共库（Topic SDK 的通信基础）
│       ├── c++/                              # C++ 专属
│       │   ├── include/                      #   头文件（protobuf、zmq、message 等）
│       │   └── lib/                          #   C++ 专属预编译库 + protoc 工具
│       │       ├── win/Release/              #     message.dll, libprotobuf.dll, protoc.exe 等
│       │       └── linux/                    #     libmessage.so + protobuf/zmq
│       │           ├── x86/20.04/Release/    #       x86 (Ubuntu 20.04)
│       │           └── arm/22.04/Release/    #       ARM (Ubuntu 22.04)
│       ├── python/                           # Python 专属
│       │   └── lib/                          #   topic.so/pyd + protobuf/zmq
│       │       ├── win/                      #     topic.pyd, libprotobuf.dll
│       │       └── linux/                    #
│       │           ├── x86/20.04/            #       x86 (Ubuntu 20.04): topic.so + libs
│       │           └── arm/22.04/            #       ARM (Ubuntu 22.04): topic.so
│       └── shared/lib/                       # C++/Python 共享的第三方依赖
│           ├── win/Release/                  #     libzmq-v142-mt-4_3_6.dll
│           └── linux/                        #     libprotobuf.so, libprotobuf.so.32, libzmq.so 等
│               ├── x86/20.04/Release/        #       x86 (Ubuntu 20.04)
│               └── arm/22.04/Release/        #       ARM (Ubuntu 22.04)
│
├── MoveAbsJ_SDK/                           # SD-01  单臂关节绝对运动
├── MoveAbsJ_Double_SDK/                    # SD-02  双臂关节绝对运动
├── MoveBlend_SDK/                          # SD-03  笛卡尔轨迹混合运动（直线+圆弧）
├── MoveS_SDK/                              # SD-04  S 曲线笛卡尔轨迹运动
├── MoveSeriesToppJ_SDK/                    # SD-05  关节连续 Topp 轨迹运动
├── JogC_SDK/                               # SD-06  笛卡尔空间点动
├── JogAnyJ_SDK/                            # SD-07  单臂任意关节位置控制
├── JogAnyJ_Double_SDK/                     # SD-08  双臂任意关节位置控制
├── DragInCST_SDK/                          # SD-09  CST 空间拖动
├── ForcePositionHybridControl_SDK/         # SD-10  力位混合控制
├── IOModule_SDK/                           # SD-11  IO 模块控制（DI/DO/脉冲）
├── SyncAsync_SDK/                          # SD-12  同步 vs 异步发送对比
├── SubLoop_SDK/                            # SD-13  子循环控制（双模型并行）
└── Topic_SDK/                              # SD-14  实时数据订阅（独立通信库）
```

---

## SDK 分类总览

### A 类：标准 RPC SDK（13 个）

依赖 `common/rpc/` 公共库，通过 RPC 协议（端口 **5868**）与机器人控制器通信。

| 编号 | SDK | 功能 | 交互方式 |
|------|-----|------|----------|
| SD-01 | MoveAbsJ | 关节空间绝对位置运动 | 循环运动 |
| SD-02 | MoveAbsJ_Double | 双臂关节空间绝对位置运动 | 循环运动 |
| SD-03 | MoveBlend | 笛卡尔空间混合轨迹（直线+圆弧） | 交互菜单 |
| SD-04 | MoveS | 笛卡尔空间 S 曲线轨迹 | 交互菜单 |
| SD-05 | MoveSeriesToppJ | 关节空间连续 Topp 轨迹 | 交互菜单 |
| SD-06 | JogC | 笛卡尔空间方向点动 | 循环运动 |
| SD-07 | JogAnyJ | 单臂任意关节位置控制 | 交互菜单 |
| SD-08 | JogAnyJ_Double | 双臂任意关节位置控制 | 交互菜单 |
| SD-09 | DragInCST | CST 空间拖动 | 交互启停 |
| SD-10 | ForcePositionHybridControl | 力位混合控制（零力/恒力/混合） | 持续型控制 |
| SD-11 | IOModule | IO 模块（GetDI / SetDO / DOPulse） | 交互菜单 |
| SD-12 | SyncAsync | 同步 vs 异步 RPC 性能对比 | 交互菜单 |
| SD-13 | SubLoop | 子循环控制（双模型并行执行） | 交互输入 |

### B 类：独立通信 SDK（1 个）

不依赖 `common/rpc/`，使用独立的 ZeroMQ + Protobuf 通信栈。

| 编号 | SDK | 功能 |
|------|-----|------|
| SD-14 | Topic | 实时数据订阅（关节数据、笛卡尔位姿、子系统状态等） |

---

## 编译与运行顺序

### 总原则

```
┌──────────────────────────────────────────────────┐
│ ① common/rpc/   公共 RPC 库（预编译，无需编译）     │
│    ├── C++:  头文件 + .lib/.dll/.so                │
│    └── Python: rpc_client.py + .pyd/.so            │
├──────────────────────────────────────────────────┤
│ ② Topic_SDK    独立编译（与 ① 无依赖关系）          │
│    依赖：common/topic/ 公共库                        │
├──────────────────────────────────────────────────┤
│ ③ 其余 13 个标准 RPC SDK                           │
│    均依赖 ① common/rpc/，彼此独立无先后顺序          │
│    可任意顺序编译                                    │
└──────────────────────────────────────────────────┘
```

> **重要**：`common/rpc/` 下的所有二进制文件（`.dll`、`.so`、`.pyd`）必须来自**同一版本的机器人控制器 SDK 发布包**，不可混用不同版本的库文件，否则可能导致通信异常。

---

## 系统要求

### 编译环境

| 项目 | 要求 |
|------|------|
| CMake | ≥ 3.12 |
| C++ 标准 | C++17 |
| 编译器 (Windows) | MSVC (Visual Studio 2015+)，需 `/Zc:preprocessor` |
| 编译器 (Linux) | GCC 7.0+ 或 Clang 5.0+ |

### Python 运行环境

| 项目 | 要求 |
|------|------|
| Python | 3.10 |
| 操作系统 | Windows 10/11、Ubuntu 20.04 / 22.04 或其他 Linux |
| 依赖库 | 预编译的动态库（已包含在项目中），无需额外安装 |

### 支持的操作系统和架构

| 操作系统 | 架构 |
|----------|------|
| Windows | x86 / x64 |
| Linux | x86 / x64、ARM / ARM64 |

### 依赖库

**A 类 — 标准 RPC SDK（13 个）：**

| 库 | 用途 |
|----|------|
| cpp_rpc | RPC 通信框架（自研） |
| nlohmann/json | JSON 序列化/反序列化 |
| pthread | Linux 线程库 |

**B 类 — Topic SDK（独立）：**

| 库 | 版本 | 用途 |
|----|------|------|
| Protocol Buffers | 3.x | 数据序列化/反序列化 |
| ZMQ (libzmq) | 4.3.6+ | 消息传输 |
| message | 自定义库 | 消息总线实现 |
| pthread | — | Linux 线程库 |

### 已验证的编译环境

| 平台 | CMake | 编译器 | 架构 |
|------|-------|--------|------|
| Windows | 4.0.2 | MSVC (Visual Studio 2022) | x64 |
| Linux (x86, Ubuntu 20.04) | 3.16.3 | GCC 9.4.0 | x86/x64 |
| Linux (ARM, Ubuntu 22.04) | 3.22.1 | GCC 11.4.0 | ARM/ARM64 |

---

## C++ 编译

### 标准 RPC SDK 编译

以 MoveAbsJ 为例，其余 12 个标准 SDK 编译方式完全一致。

**Windows：**

```powershell
cd MoveAbsJ_SDK\MoveAbsJ_c++
mkdir build; cd build
cmake .. -G "Visual Studio 18 2026" -A x64
cmake --build . --config Release
```

**Linux：**

```bash
cd MoveAbsJ_SDK/MoveAbsJ_c++
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release  # 自动检测 Ubuntu 版本；如需手动指定可加 -DLIB_UBUNTU_VERSION=2004 或 2204
make
```

编译完成后，Windows 平台会自动将 `cpp_rpc.dll` 复制到 exe 所在目录（通过 CMakeLists.txt 中的 POST_BUILD 命令），确保运行时能加载。

### Topic SDK 编译（独立）

Topic SDK 使用独立的 ZeroMQ + Protobuf 通信栈，**不依赖** `common/rpc/`，其公共库统一收口在 `common/topic/` 下。

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
cmake .. -DCMAKE_BUILD_TYPE=Release -DUBUNTU_VERSION=20.04  # 或 22.04，根据实际系统版本选择
make
```

编译完成后会自动复制依赖的 DLL（`libprotobuf.dll`、`libprotoc.dll`、`libzmq-v142-mt-4_3_6.dll`、`message.dll`）到 exe 目录。

---

## Python 运行

Python SDK 无需编译，所有模块开箱即用。

```bash
# 修改 main.py 中的 ROBOT_IP 为实际控制器 IP 后直接运行
python MoveAbsJ_SDK/MoveAbsJ_py/main.py
```

`rpc_client.py` 会自动检测操作系统、架构和 Ubuntu 版本，加载 `common/rpc/python/lib/` 下匹配当前平台的 `.pyd`/`.so` 动态库。Linux 下优先检测当前 Ubuntu 版本（20.04 / 22.04），检测失败时按 20.04 → 22.04 顺序 fallback。

---

## 公共库路径约定

### C++ SDK

每个标准 SDK 的 `CMakeLists.txt` 通过相对路径引用公共库，**禁止各 SDK 独立复制 include/ 和 lib/ 目录**：

```cmake
set(COMMON_DIR "${CMAKE_SOURCE_DIR}/../../common/rpc/c++")
include_directories("${COMMON_DIR}/include")
```

编译时自动根据平台和 Ubuntu 版本选择对应库文件：
- **Windows**: `common/rpc/c++/lib/win/Release/`
- **Linux x86**: `common/rpc/c++/lib/linux/x86/20.04/Release/`
- **Linux ARM**: `common/rpc/c++/lib/linux/arm/22.04/Release/`

Linux 平台可通过 CMake 参数指定 Ubuntu 版本（默认 `20.04`）：
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DUBUNTU_VERSION=22.04
```

### Topic SDK

Topic SDK 的 `CMakeLists.txt` 通过相对路径引用 `common/topic/` 下的公共库，**禁止各 SDK 独立复制**：

```cmake
set(TOPIC_COMMON_DIR "${CMAKE_SOURCE_DIR}/../../common/topic")
include_directories("${TOPIC_COMMON_DIR}/c++/include")
```

库文件按三层目录组织：
- **shared/lib/**：C++/Python 共用的第三方依赖（protobuf、zmq）
- **c++/lib/**：C++ 专属库（message）及编译链接用的 protobuf/zmq
- **python/lib/**：Python 专属扩展（topic.so/pyd）及运行时加载的 protobuf/zmq

编译时自动根据平台和 Ubuntu 版本选择对应库文件：
- **Windows**: `common/topic/{shared,c++}/lib/win/Release/`
- **Linux x86**: `common/topic/{shared,c++}/lib/linux/x86/20.04/Release/`
- **Linux ARM**: `common/topic/{shared,c++}/lib/linux/arm/22.04/Release/`

### Python SDK

每个标准 SDK 的 `main.py` 通过 `sys.path` 引用公共 RPC 模块：

```python
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python'))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async
```

Topic SDK 的 Python 版本由 `platform_loader.py` 自动检测操作系统、架构和 Ubuntu 版本，从 `common/topic/python/lib/` 和 `common/topic/shared/lib/` 加载匹配的动态库。

---

## 重要注意事项

### 1. ROBOT_IP 配置

每个 SDK 的 `main.py` / `main.cpp` 中都有一个 `ROBOT_IP` / `robot_ip` 变量，**运行前必须修改为实际机器人控制器的 IP 地址**。

### 2. 同步 vs 异步 RPC

| 模式 | C++ 函数 | Python 函数 | 适用场景 |
|------|----------|-------------|----------|
| 同步 | `send_rpcsy<RespDemo>()` | `send_rpcsy()` | 大多数运动指令，需等待响应确认 |
| 异步 | `send_rpcAsy()` | `send_rpc_async()` | SpeedL 等在线规划、持续型控制 |

- **同步 RPC**：逐条发送指令，每条等待控制器返回结果后再发下一条。适合 MoveAbsJ、JogC 等常规运动。
- **异步 RPC**：快速下发指令后立即返回，不阻塞等待。适合 SpeedL 在线规划、ForcePositionHybridControl 持续力控等场景。
- **SubLoop 特殊要求**：第一条指令**必须**使用异步发送，且 timeout 要足够大。

### 3. 超时参数单位

| 参数 | C++ | Python |
|------|-----|--------|
| `timeout_ms` | 毫秒 (ms) | 毫秒 (ms) |
| `sleep_time_ms` | 毫秒 (ms) | — |
| `sleep_s` / `wait_s` | — | 秒 (s) |

### 4. 指令集结构

RPC 指令字符串遵循控制器指令语法，大括号包裹：`{指令名 --参数=值}`。

双臂 SDK 使用 `||` 分隔左右臂指令：`{左臂指令||右臂指令}`。

### 5. 库文件来源一致性

`common/rpc/c++/lib/` 和 `common/rpc/python/` 下的所有二进制文件必须来自**同一版本的机器人控制器 SDK 发布包**，不可混用不同版本的库文件。

### 6. Topic SDK 特殊说明

Topic SDK 使用独立的 ZeroMQ + Protobuf 通信栈，**不依赖** `common/rpc/`，其公共库统一收口在 `common/topic/` 下。数据订阅端口固定为 **19091**。支持两种数据获取方式：
- **Direct 模式**：直接调用自由函数，适合快速获取少量字段
- **Snapshot 模式**：先获取快照再逐字段访问，适合批量读取

详细 API 文档见 `Topic_SDK/topic_c++/API.md`。

---

## 快速开始

1. **确认机器人控制器 IP**，在对应 SDK 的源码中修改 `ROBOT_IP` / `robot_ip`
2. **Python 直接运行**：`python <SDK目录>/<SDK名>_py/main.py`
3. **C++ 先编译再运行**：

```powershell
cd <SDK目录>/<SDK名>_c++
mkdir build; cd build
cmake .. ; cmake --build . --config Release
```

> 推荐新手从 **SyncAsync_SDK** 开始，它演示了同步与异步 RPC 的基本用法。

---

## 参考文档

| 文档 | 路径 |
|------|------|
| C++ 核心接口 | `common/rpc/c++/include/message/rpc_client.h` |
| Python 核心接口 | `common/rpc/python/rpc_client.py` |
| Topic 数据订阅 API | `Topic_SDK/topic_c++/API.md` |
| 力位混合控制说明 | `ForcePositionHybridControl_SDK/ForcePositionHybridControl_linux_c++例程使用说明-v1.5.2.pdf` |
| 力位混合控制 Python 说明 | `ForcePositionHybridControl_SDK/ForcePositionHybridControl_linux_python例程使用说明-v1.5.2.pdf` |
| 子循环控制 C++ 说明 | `SubLoop_SDK/SubLoop_linux_c++例程使用说明-v1.5.2.pdf` |
| 子循环控制 Python 说明 | `SubLoop_SDK/SubLoop_linux_python例程使用说明-v1.5.2.pdf` |
