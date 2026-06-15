# Topic 系统状态订阅程序

这是一个 C++ 编写的系统状态数据订阅客户端，用于通过 ZMQ 消息总线实时接收和处理系统的实时(RT)和非实时(NRT)状态数据。

## 项目功能

- **实时数据订阅**：接收实时系统状态，包括控制器信息、机器人关节位置/力矩等
- **非实时数据订阅**：接收系统配置信息，包括关节限制、工具、工件、负载等
- **线程安全**：采用互斥锁保护共享数据，支持多线程安全访问
- **跨线程调用**：提供全局系统状态对象，支持跨线程数据共享

## 系统要求

### 编译环境

| 项目 | 要求 |
|------|------|
| **CMake** | >= 3.12 |
| **C++ 标准** | C++ 17 |
| **编译器 (Windows)** | MSVC (Visual Studio 2015+) |
| **编译器 (Linux)** | GCC 7.0+ 或 Clang 5.0+ |

### 支持的操作系统和架构

- **Windows**：x86/x64
- **Linux**：x86/x64、ARM、ARM64

### 依赖库

| 库 | 版本 | 用途 |
|----|------|------|
| **Protocol Buffers** | 3.x | 数据序列化/反序列化 |
| **ZMQ (libzmq)** | 4.3.6+ | 消息传输 |
| **pthread** | - | Linux 线程库 |
| **message** | 自定义库 | 消息总线实现 |

### 已验证的编译环境

| 平台 | CMake | 编译器 | 架构 |
|------|-------|--------|------|
| **Windows** | 4.0.2 | MSVC (Visual Studio 2022) | x64 |
| **Linux (x86)** | 3.16.3 | GCC 9.4.0 | x86/x64 |
| **Linux (ARM)** | 3.22.1 | GCC 11.4.0 | ARM/ARM64 |

## 项目结构

```
topic_c++/
├── src/
│   └── topic_sub.cpp          # 主程序入口
├── include/
│   ├── shared_data.hpp        # 线程安全的共享数据类
│   ├── sub.hpp                # 订阅器接口
│   ├── msg.hpp                # 消息定义
│   ├── zmq.hpp                # ZMQ 包装
│   └── message/
│       ├── TDMessageBus.h     # 消息总线头文件
│       └── BS_thread_pool.hpp # 线程池实现
├── proto/
│   ├── overall_system_rtstate.proto   # 实时状态消息定义
│   └── overall_system_nrtstate.proto  # 非实时状态消息定义
├── lib/
│   ├── win/Release/            # Windows 库
│   │   ├── protoc.exe          # Protocol Buffer 编译器
│   │   ├── libprotobuf.dll
│   │   ├── libzmq-v142-mt-4_3_6.lib
│   │   └── message.lib
│   └── linux/
│       ├── x86/Release/        # Linux x86 库
│       └── arm/Release/        # Linux ARM 库
└── CMakeLists.txt             # CMake 构建脚本
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
- CMake 4.0.2+

#### 编译步骤

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

编译结果将生成在 `build\Release\topic.exe`

### Linux 编译 (x86)

#### 前置条件
- GCC 9.4.0+
- CMake 3.16.3+

#### 编译步骤

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

编译结果将生成在 `build/topic`

### Linux 编译 (ARM)

#### 前置条件
- GCC 11.4.0+（ARM 交叉编译器或本地编译）
- CMake 3.22.1+

#### 编译步骤

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

编译结果将生成在 `build/topic`

## 使用方法

### 基本使用

```bash
./topic
```

程序将默认连接到 `192.168.11.11` 并定时输出接收到的系统状态数据。

### 修改连接地址

编辑 `topic_c++/src/topic_sub.cpp` 中的以下代码：

```cpp
std::string remote_ip = "192.168.11.11";  // 改为你的目标主机 IP
start_subscriber(remote_ip);
```

## 数据说明

### 实时数据 (RT - Real-Time)

实时数据包含：
- **头部信息**：时间戳、帧 ID、运行状态
- **控制器信息**：名称、控制周期、全局计数、力传感器数值
- **模型数据**：多个机器人模型的关节实时信息
  - 关节位置、力矩、使能状态、模式、错误码
- **当前点信息**：末端工具、工件、机器人目标、关节目标
- **模型运行状态**：错误码、错误信息、运行状态、时间因子

### 非实时数据 (NRT - Non-Real-Time)

非实时数据包含：
- **头部信息**：时间戳、帧 ID、运行状态
- **从设备信息**：EtherCAT 从设备列表及其状态
- **模型数据**：机器人模型的静态配置
  - 关节限制（位置、速度、加速度限制）
- **工具列表**：可用工具及其数据
- **工件列表**：可用工件及其数据
- **负载列表**：负载配置信息
- **示教点**：预编程的点位信息
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

- 自动生成消息类和序列化/反序列化代码
- 支持 Windows 和 Linux 平台
- CMake 自动处理 `.proto` 文件的编译

## 性能考虑

- **更新频率**：主程序每 500ms 轮询一次获取数据
- **消息总线**：使用 ZMQ 实现高效的消息传输
- **内存管理**：采用智能指针和共享指针，自动管理内存

## 故障排除

| 问题 | 原因 | 解决方案 |
|------|------|--------|
| 编译失败：`protoc not found` | protoc 工具未找到 | 检查 `lib/win/Release/` 或 `lib/linux/` 中是否存在 protoc |
| 无法连接到目标主机 | IP 地址或网络不可达 | 检查目标 IP 地址和网络连接 |
| 收不到数据 | 消息总线未启动或订阅失败 | 检查 `start_subscriber()` 的返回值和日志 |
| Windows 链接失败 | 库文件路径错误 | 确保 `lib/win/Release/` 中存在所有所需的 `.lib` 文件 |

## 最近更新

-ver002_save: 提供topic_sub_direct.cpp（直接订阅模式）和topic_sub_snapshot.cpp（快照订阅模式）；子系统数据转移到用户端解析。
- ver001_save：子系统增加TwoFingerGripperStatus消息字段（该部分暂时写死，后续版本扩展支持在用户端进行多子系统消息解析）
- ov_system：中间版本，pb文件名称更改（已恢复，此版本不再维护）
