# Topic 系统状态订阅程序

这是一个 C++ 编写的系统状态数据订阅客户端，用于通过 ZMQ 消息总线实时接收和处理系统的实时(RT)和非实时(NRT)状态数据。

NRT 消息的底层默认上报频率为 **20 Hz**。RT 消息将在后续版本中以
**250 Hz** 作为默认上报频率；当前版本的实际接收频率以现场发布端配置和
`topic_frequency` 的测量结果为准，并可能受到网络和客户端负载影响。

可运行 `topic_frequency` 检测实际接收频率。统计在订阅回调中完成，每收到一条
消息计数一次，与客户程序调用 `get*()` 的频率无关。

## 项目功能

- **实时数据订阅**：接收实时系统状态，包括控制器信息、机器人关节位置/力矩等
- **非实时数据订阅**：接收系统配置信息，包括关节限制、工具、工件、负载、MatrixVariable、拖动系数和干涉区等
- **线程安全**：采用互斥锁保护共享数据，支持多线程安全访问
- **跨线程调用**：提供全局系统状态对象，支持跨线程数据共享

## 系统要求

### 编译环境

| 项目 | 要求 |
|------|------|
| **CMake** | >= 3.12 |
| **C++ 标准** | C++ 17 |
| **编译器 (Windows)** | MSVC (Visual Studio 2022) |
| **编译器 (Linux)** | GCC 7.0+ 或 Clang 5.0+ |

### 支持的操作系统和架构

- **Windows**：x64
- **Linux**：x86_64、AArch64

> **Linux 预编译库统一采用 Ubuntu 20.04 ABI 基线：**
>
> | 架构 | 编译基线 | 库目录 |
> |------|----------|--------|
> | x86_64 | Ubuntu 20.04 | `lib/linux/x86` |
> | AArch64 | Ubuntu 20.04 | `lib/linux/arm` |
>
> Ubuntu 20.04 基线库可在相同架构的 Ubuntu 20.04 及更新版本上使用，无需按发行版分别提供。

### 依赖库

| 库 | 版本 | 用途 |
|----|------|------|
| **Protocol Buffers** | 3.21.12 | 数据序列化/反序列化 |
| **ZMQ (libzmq)** | 4.3.6 | 消息传输 |
| **pthread** | - | Linux 线程库 |
| **message** | 自定义库 | 消息总线实现 |

### 已验证的编译环境

| 平台 | CMake | 编译器 | 架构 |
|------|-------|--------|------|
| **Windows** | 4.0.2 | MSVC (Visual Studio 2022) | x64 |
| **Linux (x86_64)** | 3.16.3 | GCC 9.4.0 | x86_64 |
| **Linux (AArch64)** | 3.16.3 | GCC 9.4.0 | AArch64 |

## 项目结构

```
topic_c++/
├── src/
│   ├── main.cpp                   # 主程序入口（精简示例）
│   ├── topic_frequency.cpp        # RT/NRT 实际接收频率检测示例
│   ├── topic_sub_direct.cpp       # 直接订阅模式完整示例
│   └── topic_sub_snapshot.cpp     # 快照订阅模式完整示例
├── include/
│   ├── system_state_reader.hpp    # 系统状态只读访问层
│   ├── shared_data.hpp            # 线程安全的共享数据类
│   ├── sub.hpp                    # 订阅器接口
│   ├── msg.hpp                    # 消息定义
│   ├── zmq.hpp                    # ZMQ 包装
│   └── message/
│       ├── TDMessageBus.h         # 消息总线头文件
│       └── BS_thread_pool.hpp     # 线程池实现
├── message_struct/
│   └── message_struct.h           # 用户自定义子系统消息结构体
├── proto/
│   ├── overall_system_rtstate.proto   # 实时状态消息定义
│   └── overall_system_nrtstate.proto  # 非实时状态消息定义
├── proto_generated/                    # 无 protoc 时使用的预生成 C++ 文件
├── lib/
│   ├── win/Release/               # Windows 库
│   │   ├── protoc.exe             # Protocol Buffer 编译器
│   │   ├── libprotobuf.dll
│   │   ├── libzmq-v142-mt-4_3_6.lib
│   │   └── message.lib
│   └── linux/
│       ├── x86/                    # Ubuntu 20.04 基线 x86_64 库
│       │   ├── libmessage.so
│       │   ├── libprotobuf.so / libprotobuf.so.32
│       │   └── libzmq.so / libzmq.so.5
│       └── arm/                    # Ubuntu 20.04 基线 AArch64 库
│           ├── libmessage.so
│           ├── libprotobuf.so / libprotobuf.so.32
│           └── libzmq.so / libzmq.so.5
└── CMakeLists.txt                 # CMake 构建脚本
```

## 编译指南

### 环境检查

**Windows**
```bash
cmake --version
cl              # 在 Visual Studio 开发者命令提示符中运行
```

**Linux**
```bash
cmake --version
gcc --version
```

### Windows 编译

#### 前置条件
- Visual Studio 2022 或更新版本
- CMake 3.12+

#### 编译步骤

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

编译结果位于 `build\Release`，包括 `topic.exe`、`topic_frequency.exe`、
`topic_sub_direct.exe` 和 `topic_sub_snapshot.exe`。

### Linux 编译 (x86_64)

#### 前置条件
- GCC 9.4.0+
- CMake 3.16.3+

#### 编译步骤

```bash
mkdir build
cd build
# 自动使用 lib/linux/x86 中的 Ubuntu 20.04 基线库
cmake ..
make -j$(nproc)
```

编译结果包括 `build/topic`、`build/topic_frequency`、`build/topic_sub_direct`
和 `build/topic_sub_snapshot`。

### Linux 编译 (AArch64)

#### 前置条件
- GCC 9.4.0+（AArch64 交叉编译器或本地编译）
- CMake 3.16.3+

#### 编译步骤

```bash
mkdir build
cd build
# 自动使用 lib/linux/arm 中的 Ubuntu 20.04 基线库
cmake ..
make -j$(nproc)
```

编译结果包括 `build/topic`、`build/topic_frequency`、`build/topic_sub_direct`
和 `build/topic_sub_snapshot`。

## 使用方法

### 基本使用

```bash
./topic
```

程序将默认连接到 `192.168.2.100` 并定时输出接收到的系统状态数据。

### 修改连接地址

编辑 `topic_c++/src/main.cpp` 中的以下代码：

```cpp
std::string remote_ip = "192.168.2.100";  // 改为你的目标主机 IP
start_subscriber(remote_ip);
```

## 数据说明

### 实时数据 (RT - Real-Time)

RT 消息将在后续版本中以 **250 Hz** 作为默认上报频率。当前版本请使用
`topic_frequency` 测量实际接收频率。

实时数据包含：
- **头部信息**：时间戳、帧 ID、运行状态
- **控制器信息**：名称、控制周期、全局计数、力传感器数值
- **模型数据**：多个机器人模型的关节实时信息
  - 关节位置、力矩、使能状态、模式、错误码
- **当前点信息**：末端工具、工件、机器人目标、关节目标
- **模型运行状态**：错误码、错误信息、运行状态、时间因子

### 非实时数据 (NRT - Non-Real-Time)

底层默认上报频率为 **20 Hz**。

非实时数据包含：
- **头部信息**：时间戳、帧 ID、运行状态
- **从设备信息**：EtherCAT 从设备列表及其状态
- **模型数据**：机器人模型的静态配置
  - 关节限制（位置、速度、加速度限制）
- **工具列表**：可用工具及其数据
- **工件列表**：可用工件及其数据
- **负载列表**：负载配置信息
- **示教点**：预编程的点位信息
- **MatrixVariable**：按模型提供变量名称及其数组数据
- **拖动系数**：电流环拖动系数 `drag_in_cst_coef`
- **干涉区/安全区**：区域动作、优先级、形状、中心、尺寸、姿态、边距和双点配置
- **子系统**：各子系统的状态
- **接口**：系统接口的配置

## 关键特性

### 1. 线程安全的数据共享

通过 `SharedSystemState` 类实现：
- 使用互斥锁保护 RT 和 NRT 数据
- 支持多线程安全读取
- 原子性数据更新

### 2. 跨线程访问

全局函数 `getSystemState()` 提供全局系统状态访问：

```cpp
auto rt = getSystemState().getRt();   // 获取实时状态
auto nrt = getSystemState().getNrt(); // 获取非实时状态
```

### 3. Protocol Buffer 支持

- 有 `protoc` 时自动生成消息类；无 `protoc` 时使用仓库内预生成代码
- 支持 Windows 和 Linux 平台
- CMake 自动处理 `.proto` 文件的编译

## 性能考虑

- **底层上报频率**：后续版本 RT 默认为 250 Hz；NRT 默认为 20 Hz
- **示例读取间隔**：示例程序的轮询或打印频率独立于底层上报频率，可按业务需要调整
- **消息总线**：使用 ZMQ 实现高效的消息传输
- **内存管理**：采用智能指针和共享指针，自动管理内存

## 频率检测

```bash
./topic_frequency
```

修改 `src/topic_frequency.cpp` 中的发布者 IP 后运行。程序默认统计 5 秒，输出
RT/NRT 实际接收频率、窗口消息数、累计消息数和最新发布端帧号。

输出中的 `publisher_timestamp_delta` 是发布端控制器原始计数差，单位随控制器采样
周期变化，不是固定单位的毫秒。`frame_delta` 与 `messages` 相同时，表示统计窗口内
未发现帧号跳变。

也可在业务代码中调用 `measureReceiveFrequency(duration_ms)`。发布端时间戳、帧号、
本地接收时间戳和累计接收数可通过 `getRtReceiveStats()`、`getNrtReceiveStats()` 获取。

## 故障排除

| 问题 | 原因 | 解决方案 |
|------|------|--------|
| 提示 `protoc not found` | Linux 未安装 protoc | 这是警告，CMake 会自动使用 `proto_generated/` 中的预生成文件 |
| 无法连接到目标主机 | IP 地址或网络不可达 | 检查目标 IP 地址和网络连接 |
| 收不到数据 | 消息总线未启动或订阅失败 | 检查订阅日志、发布端状态、目标 IP 和 19091 端口 |
| Windows 链接失败 | 库文件路径错误 | 确保 `lib/win/Release/` 中存在所有所需的 `.lib` 文件 |

## 最近更新

- **ver005**: 新增订阅回调频率统计与 `topic_frequency` 示例；NRT 默认频率更新为 20 Hz，并注明后续版本 RT 默认频率为 250 Hz；Linux 库统一采用 Ubuntu 20.04 ABI 基线
- **ver004**: 同步新版 `overall_system_nrtstate.proto`，新增 MatrixVariable、`drag_in_cst_coef` 和 InfRng；补充快照/直接访问 API 及两个全字段示例；旧版 PDF 归档至 `PDF-old/`
- **ver003**: 新增 `main.cpp`（精简示例，仅含 jointtarget/robottarget + 子系统消息）；子系统消息结构体统一提取到 `message_struct/message_struct.h`（含 `FingerGripperStatus`、`TwoFingerGripperStatus`、`TwoFingerGripperYSStatus`）；`print_vector` 提取到 `system_state_reader.hpp`
- **ver002_save**: 提供 `topic_sub_direct.cpp`（直接订阅模式）和 `topic_sub_snapshot.cpp`（快照订阅模式）；子系统数据转移到用户端解析
- **ver001_save**: 子系统增加 `TwoFingerGripperStatus` 消息字段（该部分暂时写死，后续版本扩展支持在用户端进行多子系统消息解析）
- **ov_system**: 中间版本，pb文件名称更改（已恢复，此版本不再维护）
