# 机器人控制 SDK 示例集

本仓库提供擎枢机器人控制系统的 **19 个 SDK 示例模块**，覆盖关节运动、笛卡尔运动、拖拽与力控、IO、EtherCAT PDO/SDO 读取、实时状态订阅及相机图像获取。除 RealSenseCamera 仅提供 Python 版本外，其余模块均提供 C++ 和 Python 示例。

## 运行前必读

- 运动示例会实际下发控制指令，部分程序启动后立即初始化、使能并运动。运行前必须核对机械臂型号、目标点位、坐标系、速度与工作空间，确保急停可用、人员远离运动区域。
- IO、ReadPdo、ReadSdo 示例也包含控制器初始化指令，不应当作完全无副作用的连接测试。首次连接建议从只读的 **Topic SDK** 开始。
- **Ctrl+C、关闭终端、RPC 超时或客户端退出，不等于机器人已经停止。** 并非所有示例都在退出时发送 `{Stop}`；停止结果需结合控制器状态确认，软件 Stop 不能替代硬件急停。
- 示例中的点位、从站编号、IO 名称和相机标识仅用于演示，不能直接套用于任意设备。头文件、Python 封装、动态库与控制器须使用匹配的 SDK 版本。

导航：[快速开始](#快速开始) · [模块索引](#模块索引) · [目录与公共库](#目录与公共库) · [C++ 构建](#c-构建) · [RPC 接口](#rpc-接口) · [专项说明](#专项说明) · [常见问题](#常见问题) · [参考文档](#参考文档)

## 快速开始

以下命令均在仓库根目录 `Hello_Demo_Example/` 执行，各模块独立使用，无需按顺序运行。

### 1. 准备环境

| 项目 | 要求或建议 |
|------|------------|
| Python | 64 位 CPython 3.10；现有扩展按 CPython 3.10 ABI 提供，不能直接用于其他 Python 版本 |
| C++ | C++17；建议 CMake 3.21+，以支持下文的 Visual Studio 2022 构建命令 |
| Windows | x64；C++ 建议 Visual Studio 2022，安装“使用 C++ 的桌面开发”组件；运行时需匹配的 Visual C++ 运行库 |
| Linux | x86_64 或 AArch64/ARM64；使用与预编译库 ABI 兼容的系统及编译器 |
| 依赖库 | RPC 保留 `common/rpc/`；新版 Topic 使用各自工程内的 `include/`、`lib/`。缺失文件应从匹配的 SDK 发布包补齐 |
| 相机示例 | 另需 `numpy`、`opencv-python`，以及控制器端已配置的两路 `RsCameraSensor` |

Linux 库目录中的 `x86` 表示当前提供的 x86_64 版本，`arm` 表示 ARM64 版本，不代表同时提供 32 位库。具体运行库要求见各平台的库环境说明。

### 2. 配置控制器地址

没有统一的全局 IP 配置文件，且当前示例的默认地址并不完全相同。运行前修改所选示例的源码：

| 示例 | 地址变量 | 当前默认地址 |
|------|----------|--------------|
| RPC、实时关节目标、相机 | `robot_ip` 或 `ROBOT_IP` | `192.168.11.11` |
| 新版 Topic C++ 主示例、频率检测 | `remote_ip` | `192.168.2.100` |
| 新版 Topic C++ 全字段 Direct / Snapshot | `remote_ip` | `192.168.2.216` |
| Topic Python 主示例、频率检测 | `PUBLISHER_IP` | `192.168.11.11` |

修改 C++ 地址后需要重新编译。各通信通道的用途如下，不能互相替代：

| 通道 | 端口 | 用途 |
|------|------|------|
| 普通 RPC | `5868` | 发送 `{指令名 --参数=值}` 字符串，接收执行响应 |
| 实时关节目标 | 默认 `5900`，可指定 `5901` | JogAnyJFast 的二进制关节目标帧；端口需与 `--index` 配套，不接受普通字符串指令 |
| Topic | `19091` | ZeroMQ + Protobuf 状态订阅，只读 |
| HTTP MJPEG | `6888` | RealSense 图像流；图像服务仍由 `5868` RPC 启停 |

### 3. 先运行只读订阅

以下使用新版独立 Python 工程 `Topic_SDK/topic.py/topic_all_py/`；其中 `topic.py` 是目录名。

Windows PowerShell：

```powershell
py -3.10 Topic_SDK/topic.py/topic_all_py/main.py
```

Linux：

```bash
python3.10 Topic_SDK/topic.py/topic_all_py/main.py
```

控制器发布数据后，示例打印关节目标、笛卡尔位姿和子系统等状态。Python 示例还包含 model 夹爪读取。按 Ctrl+C 结束本地订阅。

Windows 若未安装 `py` 启动器，可改用已确认是 CPython 3.10 的 `python` 或解释器完整路径。

### 4. 再选择控制示例

阅读并调整命令参数、确认安全条件后，再运行所需模块。例如单臂关节运动：

```powershell
# Windows：此示例会使能并驱动机器人运动
py -3.10 MoveAbsJ_SDK/MoveAbsJ_py/main.py
```

```bash
# Linux：此示例会使能并驱动机器人运动
python3.10 MoveAbsJ_SDK/MoveAbsJ_py/main.py
```

学习发送参数可阅读 `main_send_params`，学习同步阻塞期间发送 Stop 可阅读 `main_thread_start_stop`。这两个变体同样包含真实运动指令，并非只打印说明。

## 模块索引

按通信方式分为：16 个标准 RPC 模块、1 个 RPC + 实时关节目标模块、1 个独立 Topic 模块、1 个 RPC + HTTP 图像模块。

| 编号 | 模块 | 功能与特点 |
|------|------|------------|
| SD-01 | [MoveAbsJ](MoveAbsJ_SDK/) | 单臂关节绝对运动；含发送参数说明、独立线程 Start/Stop 变体 |
| SD-02 | [MoveAbsJ_Double](MoveAbsJ_Double_SDK/) | 双臂关节绝对运动 |
| SD-03 | [MoveBlend](MoveBlend_SDK/) | 笛卡尔直线、圆弧混合轨迹 |
| SD-04 | [MoveS](MoveS_SDK/) | 笛卡尔 S 曲线轨迹；含 EGM 偏移变体 |
| SD-05 | [MoveSeriesToppJ](MoveSeriesToppJ_SDK/) | 关节空间连续 Topp 轨迹 |
| SD-06 | [JogC](JogC_SDK/) | 笛卡尔空间方向点动 |
| SD-07 | [JogAnyJ](JogAnyJ_SDK/) | 单臂任意关节位置控制；含交互式正弦轨迹变体 |
| SD-08 | [JogAnyJ_Double](JogAnyJ_Double_SDK/) | 双臂任意关节位置控制 |
| SD-09 | [JogAnyC](JogAnyC_SDK/) | 笛卡尔空间任意位姿控制 |
| SD-10 | [DragInCST](DragInCST_SDK/) | CST 拖拽及退出后恢复 CSP 模式 |
| SD-11 | [ForcePositionHybridControl](ForcePositionHybridControl_SDK/) | 零力、恒力与力位混合控制 |
| SD-12 | [IOModule](IOModule_SDK/) | GetDI、SetDO、DOPulse |
| SD-13 | [SyncAsync](SyncAsync_SDK/) | 同步 MoveAbsJ 与异步 SpeedL 用法对比 |
| SD-14 | [SubLoop](SubLoop_SDK/) | 交互式双模型子循环控制 |
| SD-15 | [ReadPdo](ReadPdo_SDK/) | EtherCAT PDO 读取及 `pdo_value` 解析 |
| SD-16 | [ReadSdo](ReadSdo_SDK/) | EtherCAT SDO 读取及 `sdo_value` 解析 |
| SD-17 | [Topic](Topic_SDK/) | 独立状态订阅，支持 Direct、Snapshot、RT/NRT 接收频率检测 |
| SD-18 | [RealSenseCamera](RealSenseCamera_SDK/) | 两路 HTTP MJPEG 图像，仅 Python |
| SD-19 | [JogAnyJFast](JogAnyJFast_SDK/) | 经 RPC 进入快速运动模式，再经 `5900` / `5901` 持续发送关节目标 |

### 扩展示例入口

| 示例 | C++ 源码 | Python 源码 |
|------|----------|-------------|
| MoveAbsJ 发送参数说明 | [main_send_params.cpp](MoveAbsJ_SDK/MoveAbsJ_c++/src/main_send_params.cpp) | [main_send_params.py](MoveAbsJ_SDK/MoveAbsJ_py/main_send_params.py) |
| MoveAbsJ 独立线程 Start/Stop | [main_thread_start_stop.cpp](MoveAbsJ_SDK/MoveAbsJ_c++/src/main_thread_start_stop.cpp) | [main_thread_start_stop.py](MoveAbsJ_SDK/MoveAbsJ_py/main_thread_start_stop.py) |
| MoveS EGM 偏移 | [main_moves_egm_offset.cpp](MoveS_SDK/MoveS_c++/src/main_moves_egm_offset.cpp) | [main_moves_egm_offset.py](MoveS_SDK/MoveS_py/main_moves_egm_offset.py) |
| JogAnyJ 正弦轨迹 | [main-sine.cpp](JogAnyJ_SDK/JogAnyJ_c++/src/main-sine.cpp) | [main-sine.py](JogAnyJ_SDK/JogAnyJ_py/main-sine.py) |
| Topic 全字段 Direct | [topic_sub_direct.cpp](Topic_SDK/topic_c++/topic_c++/src/topic_sub_direct.cpp) | [topic_sub_direct.py](Topic_SDK/topic.py/topic_all_py/examples/topic_sub_direct.py) |
| Topic 全字段 Snapshot | [topic_sub_snapshot.cpp](Topic_SDK/topic_c++/topic_c++/src/topic_sub_snapshot.cpp) | [topic_sub_snapshot.py](Topic_SDK/topic.py/topic_all_py/examples/topic_sub_snapshot.py) |
| Topic 接收频率检测 | [topic_frequency.cpp](Topic_SDK/topic_c++/topic_c++/src/topic_frequency.cpp) | [topic_frequency.py](Topic_SDK/topic.py/topic_all_py/examples/topic_frequency.py) |

旧版 Python 的 [系统状态与错误码示例](Topic_SDK/topic_py/examples/topic_sub_socket_state.py) 仍保留在 `topic_py/`，继续使用 `common/topic/`；它不属于上表的新独立工程。

## 目录与公共库

```text
Hello_Demo_Example/
├── README.md
├── CHANGELOG.md
├── common/
│   ├── rpc/
│   │   ├── c++/                  # include/ 头文件、lib/ 预编译库
│   │   └── python/               # rpc_client.py、lib/ 扩展模块
│   └── topic/                    # 保留的旧版 Topic 公共依赖
├── MoveAbsJ_SDK/                  # 标准模块结构示例
│   ├── MoveAbsJ_c++/              # CMakeLists.txt、src/
│   └── MoveAbsJ_py/               # main.py 及变体
├── Topic_SDK/
│   ├── topic_c++/
│   │   ├── README.md             # 新版 C++ 说明
│   │   └── topic_c++/            # 实际 CMake 工程：src/、include/、lib/、proto/
│   ├── topic.py/
│   │   ├── README.md             # 新版 Python 说明
│   │   └── topic_all_py/         # main.py、examples/、lib/、API.md
│   └── topic_py/                 # 保留的旧版 Python 示例，使用 common/topic/
├── RealSenseCamera_SDK/           # RealSenseCamera_py/、独立说明
└── …                             # 其余模块见上表
```

各示例彼此独立，使用随附的预编译依赖。标准 RPC 模块继续共用 `common/rpc/`；新版 Topic C++ 和 Python 则各自携带依赖，不再引用 `common/topic/`。不要混用新旧工程的封装、头文件和二进制库。

下表的库路径相对于“基准目录”：

| 用途 | 基准目录 | Windows | Linux |
|------|----------|---------|-------|
| RPC C++ | `common/rpc/c++/` | `lib/win/Release/` | `lib/linux/<arch>/` |
| RPC Python | `common/rpc/python/` | `lib/win/rpc.pyd` | `lib/linux/<arch>/cp310/rpc.so` |
| 新版 Topic C++ | `Topic_SDK/topic_c++/topic_c++/` | `lib/win/Release/` | `lib/linux/<arch>/` |
| 新版 Topic Python | `Topic_SDK/topic.py/topic_all_py/` | `lib/win/cp310/topic.pyd` | `lib/linux/<arch>/cp310/topic.so` |

`<arch>` 为 `x86` 或 `arm`。Topic Python 的 Protobuf/ZeroMQ 依赖位于 `cp310/` 的上一级。标准 Python 示例通过 `sys.path` 引入公共 `rpc_client.py`；新版 Topic 由工程内的 `platform_loader.py` 加载同目录 `lib/` 下的扩展。

`Topic_SDK/topic_py/` 及 `common/topic/` 仍保留，旧版 Python 加载器继续引用后者。本 README 的 Topic 快速开始、构建和 API 链接均采用新版独立工程。

Linux 下文件名区分大小写：例如双臂 MoveAbsJ 的子目录实际为 `MoveAbsJ_double_c++`、`MoveAbsJ_double_py`。新版 Topic C++ 的路径包含两层 `topic_c++/`，Python 路径包含名为 `topic.py/` 的目录，请勿省略层级。

## C++ 构建

仓库没有顶层统一构建入口，`-S` 应指向具体模块的 C++ 目录。以下构建命令本身不会连接机器人；运行可执行文件才会执行示例。

### 标准 RPC 模块

以 MoveAbsJ 为例，Windows PowerShell：

```powershell
cmake -S "MoveAbsJ_SDK/MoveAbsJ_c++" -B "MoveAbsJ_SDK/MoveAbsJ_c++/build" -G "Visual Studio 17 2022" -A x64
cmake --build "MoveAbsJ_SDK/MoveAbsJ_c++/build" --config Release
# 仅在完成地址、点位和安全检查后运行
.\MoveAbsJ_SDK\MoveAbsJ_c++\build\Release\Hello_MoveAbsJ.exe
```

Linux：

```bash
cmake -S "MoveAbsJ_SDK/MoveAbsJ_c++" -B "MoveAbsJ_SDK/MoveAbsJ_c++/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "MoveAbsJ_SDK/MoveAbsJ_c++/build" --parallel
# 仅在完成地址、点位和安全检查后运行
./MoveAbsJ_SDK/MoveAbsJ_c++/build/Hello_MoveAbsJ
```

Windows 生成器名称需与实际安装的 Visual Studio 和 CMake 支持情况一致，可用 `cmake --help` 查看。更换生成器时使用新的构建目录。Windows 构建后会自动将 `cpp_rpc.dll` 复制到可执行文件目录。

MoveAbsJ 默认构建三个目标：`Hello_MoveAbsJ`、`Hello_MoveAbsJ_ThreadStop`、`Hello_MoveAbsJ_SendParams`。可通过 `--target` 选择其中一个。

其他模块采用相同的配置、构建方式，但可执行文件名以各自 `CMakeLists.txt` 的 `add_executable` 为准，例如 `Hello_MoveS_EgmOffset`、`Hello_JogAnyJ3_Sine`、`MoveSeriesToppJ2`、`subloop_test`。

### Topic 模块

新版 CMake 工程位于 `Topic_SDK/topic_c++/topic_c++/`，使用工程自己的 `include/` 和 `lib/`，不依赖根目录的 `common/`。下列命令使用预生成 Protobuf 文件，不要求本机安装 `protoc`。

Windows PowerShell：

```powershell
cmake -S "Topic_SDK/topic_c++/topic_c++" -B "Topic_SDK/topic_c++/topic_c++/build" -G "Visual Studio 17 2022" -A x64 -Dprotoc=OFF
cmake --build "Topic_SDK/topic_c++/topic_c++/build" --config Release
.\Topic_SDK\topic_c++\topic_c++\build\Release\topic.exe
```

Linux：

```bash
cmake -S "Topic_SDK/topic_c++/topic_c++" -B "Topic_SDK/topic_c++/topic_c++/build" -DCMAKE_BUILD_TYPE=Release -Dprotoc=OFF
cmake --build "Topic_SDK/topic_c++/topic_c++/build" --parallel
./Topic_SDK/topic_c++/topic_c++/build/topic
```

- 当前 CMake **不使用 `UBUNTU_VERSION` 参数**；Linux 库按 x86_64 / AArch64 架构选取，采用 Ubuntu 20.04 ABI 基线。
- `protoc` 选项默认开启；Windows 配置指向工程内 `lib/win/Release/protoc.exe`，Linux 查找系统工具。重新生成 `.proto` 前需确认工具存在且与随附 Protobuf 版本匹配，否则使用上面的 `-Dprotoc=OFF`。
- 默认同时构建四个目标：`topic`、`topic_frequency`、`topic_sub_direct`、`topic_sub_snapshot`。无需修改 CMake 源码即可选择示例，也可用 `--target` 单独构建；四个程序的发布者 IP 分别在各自源码中配置。
- Windows 构建后为各目标自动复制 `message.dll`、Protobuf 和 ZeroMQ 等依赖 DLL。

## RPC 接口

接口定义以 [C++ rpc_client.h](common/rpc/c++/include/message/rpc_client.h) 和 [Python rpc_client.py](common/rpc/python/rpc_client.py) 为准。

### 同步、异步与独立线程

| 方式 | C++ | Python | 返回语义 |
|------|-----|--------|----------|
| 同步 | `send_rpcsy<RespType>()` | `send_rpcsy()` | 逐条等待响应，返回汇总后的响应列表 |
| 异步 | `send_rpcAsy()` | `send_rpc_async()` | 不等待控制器响应；函数返回 `void` / `None`，结果经回调处理 |
| 独立线程 | `send_rpc_thread()` | `send_rpc_thread()` | 返回 `future<bool>` / `Future`，表示是否成功提交到底层异步发送接口 |

同步调用是否等待运动完成，取决于控制器该指令的响应时机。主线程被同步调用阻塞时，不能依赖该线程随后执行的 Stop 来中断当前等待，应从独立线程或其他控制入口发出。

MoveAbsJ 的线程变体先发送 Start 并等待发送任务返回，再在主线程同步发送运动，另一个线程延时 1 秒发送 Stop。**`.get()` / `.result()` 成功仅表示发送任务成功，不表示 Start/Stop 已执行完成，也不保证整组后续命令被取消。** 必须结合响应和实际状态判断。

异步接口仍会执行配置的发送延时，并非在任何参数下都立即返回；应用需要等待结果时，应保持连接和进程存活，并自行管理回调完成条件。

### 参数顺序与默认值

同步、异步接口的可选参数顺序均为：`间隔 → debug → 超时 → response_callback → timing_callback`；独立线程接口没有间隔参数。

以下为已有 `client`、`cmds` 时的调用片段，不是独立可运行程序：

```cpp
send_rpcsy<RespDemo>(client, cmds, 100);
send_rpcAsy(client, cmds, 0);
// 显式传入超时前，必须先传 debug
send_rpcsy<RespDemo>(client, cmds, 100, true, 5000);
```

```python
send_rpcsy(client, cmds, sleep_s=0.1)
send_rpc_async(client, cmds, wait_s=0)
send_rpcsy(client, cmds, sleep_s=0.1, debug=True, timeout_ms=5000)
```

| 参数 | C++ 当前默认值 | Python 当前默认值 |
|------|----------------|-------------------|
| 同步间隔 | `sleep_num = 0` ms | `sleep_s = 0` s |
| 异步间隔 | `wait_num = 0` ms | `wait_s = 0.5` s |
| 自动打印 | `debug = false` | `debug = False` |
| 同步 / 异步每条请求超时 | `outim_num = 864000000` ms | `timeout_ms = 864000000` ms |
| 独立线程请求超时 | `outim_num = 10000` ms | `timeout_ms = 864000000` ms |

- 普通示例省略超时实参，使用当前封装默认值。`864000000` ms 为 10 天，是有限等待，并不等于禁用超时。
- 发送间隔与响应超时不是同一概念。当前封装在每条命令处理后执行所配置的延时，**包含最后一条**；C++ 单位为 ms，Python 为 s。
- 表中为请求响应超时，不是连接超时，也不适用于 `5900` / `5901` 实时通道。超时不能作为运动已经停止的依据。

### 返回值、回调与耗时

| 内容 | C++ | Python |
|------|-----|--------|
| 通用响应字段 | `return_code`、`subcmd_index`、`return_message` | `code`、`index`、`message` |
| 扩展响应 | `RespPdo::pdo_value`、`RespSdo::sdo_value` 等模板类型 | 通用响应不保留扩展字段，需解析原始 JSON |
| 同步响应回调 | `(status, vector<RespType>, seq, cmd)` | `(status, resp_list, seq_id, cmd)` |
| 异步响应回调 | `(status, core::Msg, seq, cmd)` | `(status, resp_list, seq_id, cmd)` |
| 独立线程响应回调 | `(status, core::Msg, seq, cmd)` | `(status, raw_response, seq_id, cmd)`，响应体为字符串 |

同步返回的是各条请求响应的平铺列表，不能假设每条命令只有一个元素。需要关联发送命令时，优先使用回调携带的 `seq` / `seq_id` 和 `cmd`；`subcmd_index` / `index` 不应当作整批请求的唯一编号。

`status` 是通信/解析状态，`return_code` / `code` 是控制器返回的业务码，两者不能混为一谈。发送成功或收到响应都不等于运动已到位；业务码应结合 `return_message` 和匹配版本的指令手册判断。

C++ 和 Python 均支持 `timing_callback(RpcTimingInfo)`，包含消息序号、born/return 时间戳、`header_delta_ms` 和 `client_rtt_ms`。响应头时间是否有效应检查 `server_time_valid`；不要将头部时间差无条件当作精确的服务端处理耗时。

## 专项说明

### 初始化与停止

多数标准 RPC 示例采用以下初始化顺序，之后同步或异步下发业务指令：

```text
{Clear} → {Disable} → {Recover} → {Mode} → {Enable}
        → [笛卡尔运动：{SetUsingSP --state=on}]
        → [按需清理、定义 Var] → {Start}
```

- `{SetUsingSP --state=on}` 开启最优求解器，用于笛卡尔空间运动；纯关节运动不需要。SyncAsync 同时演示 SpeedL，因此也包含该指令。
- MoveS 当前使用 `--robottarget_value` 内联轨迹点；MoveAbsJ 主示例通过 `Var` 定义关节目标变量。
- 双臂指令以 `||` 分隔左右模型，例如 `{MoveAbsJ||MoveAbsJ}`。
- DragInCST 正常退出顺序为 `{Stop --last_count=10}` → `{Sleep --count=100}` → `{SwitchToCSP}` → `{Recover}` → `{Start}`。
- Stop 仅在部分示例中显式发送，例如线程中断、力控退出、拖拽退出、正弦轨迹交互和快速关节目标示例。异常退出时仍须独立确认机器人状态。
- SubLoop 是交互式示例，初始化和首条 SubLoop 发送调用当前为注释状态；应先阅读专用说明。首条 SubLoop 指令使用异步发送，响应超时应覆盖其执行周期。

### JogAnyJFast 实时关节目标

[JogAnyJFast_SDK](JogAnyJFast_SDK/) 先经 `5868` 初始化并异步发送 `JogAnyJFast`，再连接指定实时端口持续发送目标，正常结束时经 `5868` 发送 Stop。

- C++ 使用 `RealtimeChannel(robot_ip, realtime_port)` / `send_realtime_joint_target`，Python 使用 `RealtimeRpcClient(ROBOT_IP, realtime_port)`，构造时可指定端口。
- 当前示例为 `--index=0`、`realtime_port=5900`；源码约定 `--index=1` 对应 `5901`。切换时需同时修改指令与端口，并确认控制器对应通道已启用。
- 关节目标为 5 组机械臂 × 每组 10 个关节，发送单位为 rad；示例中的角度数组会先从度转换为弧度。
- 当前示例设置 2000 帧、每帧后延时 1 ms。它不是硬实时周期保证，实际频率受系统调度、发送耗时和 Python 运行时影响。当前 `--last_count` 在 C++ 中为 `5000`、Python 中为 `10000`，并非发送帧数。
- 目标需要持续刷新。停止发送不代表机械臂会继续运行到最后一个目标，发送成功也不代表运动完成。
- C++ 已改为每发送一次 `JogAnyJFast` 命令递增 `jog_cmd_count`，同次命令的目标帧使用 `target.count = jog_cmd_count`，不是逐帧递增。但随附 `FastJointTargetParam` 仍只定义 `joint_target_pos`，此处仍存在接口不一致，需先对齐示例与库接口再构建；不要自行给通信帧追加字段。

### ReadPdo / ReadSdo 扩展字段

C++ 使用 `send_rpcsy<RespPdo>()` / `send_rpcsy<RespSdo>()`，从返回的 `vector` 中读取扩展字段。Python 示例使用 `client.call_await_raw_msg()` 获取完整响应消息，再对 `response_msg.toString()` 做 JSON 解析，保留 `pdo_value` / `sdo_value`。

两类指令的 `size` 单位不同：当前 ReadPdo 示例按 bit 指定，ReadSdo 按字节指定。运行前核对从站、索引、子索引及数据宽度。

### Topic 状态订阅

Topic 与 RPC 可独立使用：RPC 下发控制，Topic 获取状态。它区分 RT（关节、位姿、模型运行状态等）与 NRT（子系统、接口、配置等）数据，读取前分别判断对应数据是否已到达。

- **Direct**：每次取字段时内部获取快照，适合单字段读取。
- **Snapshot**：先获取并检查有效快照，再从同一快照批量读取，适合需要同一批状态的场景。
- 新版 NRT 访问层包含 MatrixVariable、`drag_in_cst_coef` 电流环拖动系数和 InfRng 干涉区/安全区字段。字段含义及访问方式见对应语言的 API 文档。
- Python 的 `get_model_gripper_state()` 默认读取 `model1/joint0` 的 RT model 夹爪状态，返回米、毫米和 Topic 时间戳；该映射须与实际设备一致。它不同于 NRT 中的 `TwoFingerGripperYS` 外设夹爪。
- 子系统 `data` 是原始二进制数据，解析前必须确认子系统类型和协议；仅凭字节长度相同不能认定为同一结构体。

#### 接收频率检测

新版提供 `topic_frequency` / `examples/topic_frequency.py`，默认统计 5 秒，输出 RT/NRT 实际接收频率、窗口消息数、累计接收数、发布端帧号及时间戳差。计数来自订阅回调，**不是业务代码调用 `get*()` 或打印的频率**。

按前文构建 C++，并修改各频率检测示例的 IP 后运行：

```powershell
# Windows，任选一种语言
.\Topic_SDK\topic_c++\topic_c++\build\Release\topic_frequency.exe
py -3.10 Topic_SDK/topic.py/topic_all_py/examples/topic_frequency.py
```

```bash
# Linux，任选一种语言
./Topic_SDK/topic_c++/topic_c++/build/topic_frequency
python3.10 Topic_SDK/topic.py/topic_all_py/examples/topic_frequency.py
```

- 随附新版说明将 NRT 默认上报频率标为 **20 Hz**；RT **250 Hz 是后续版本的默认值说明**，不能当作当前实测结果。实际接收频率以发布端配置和现场测量为准。
- C++ 可调用 `measureReceiveFrequency(duration_ms)`，Python 可调用 `measure_receive_frequency(duration_seconds)`；两者默认均为 5 秒，但参数单位不同。
- `publisher_timestamp_delta` 是发布端控制器原始计数差，单位取决于控制器采样周期，不是固定的毫秒；本地接收时间戳 `local_receive_timestamp_us` 才明确以微秒表示。

完整说明见 [新版 Topic C++ README](Topic_SDK/topic_c++/README.md)、[C++ API](Topic_SDK/topic_c++/topic_c++/API.md)、[新版 Topic Python README](Topic_SDK/topic.py/README.md) 和 [Python API](Topic_SDK/topic.py/topic_all_py/API.md)。各语言目录的 `PDF-old/` 为历史文档归档。

### RealSense HTTP 图像

按 [RealSenseCamera README](RealSenseCamera_SDK/README.md) 的 Linux 环境要求配置两路 `RsCameraSensor`，修改 IP 和相机标识后运行：

```bash
python3.10 -m pip install -r RealSenseCamera_SDK/RealSenseCamera_py/requirements.txt
python3.10 RealSenseCamera_SDK/RealSenseCamera_py/main.py
```

程序通过 `ShowImageCameraSensor` 启动图像服务，读取：

```text
http://<robot-ip>:6888/RsCameraSensor/0/0/color
http://<robot-ip>:6888/RsCameraSensor/1/0/color
```

输出图像为 `RGB/HWC/numpy.uint8`，同时提供本机接收时间戳和帧序号；每路只保留最新帧，两路图像不保证严格同步。按 `q`、Esc 或 Ctrl+C 退出，清理时在 RPC 连接有效的情况下发送 `StopShowImageCameraSensor`。无图形桌面时可将 `SHOW_PREVIEW` 设为 `False`。

## 常见问题

| 现象 | 优先检查 |
|------|----------|
| `Connection failed` 或 Topic 一直无数据 | IP、网络路由、防火墙、对应端口和控制器服务是否启用；RPC 连通不代表 Topic 已发布数据 |
| `No module named 'rpc'/'topic'`、ABI 不支持 | 是否为 64 位 CPython 3.10、目录是否完整、CPU 架构是否与库匹配；不要用同名 pip 包替代随附扩展 |
| `DLL load failed` / 找不到 DLL | 匹配的 Visual C++ 运行库及随附 DLL 是否齐全；C++ 构建后的依赖 DLL 应与 exe 一起保留 |
| `GLIBC_*`、`GLIBCXX_*`、Protobuf 符号错误 | 系统 ABI 与实际加载的 `.so` 是否匹配；检查库环境说明及 `ldd` 输出 |
| 缺少 `CallAwaitRawMsg`、`CallAsyncRawMsg` 或 `RealtimeChannel` | Python 封装与 `.pyd/.so` 版本不一致，需配套更新，不能只替换 `rpc_client.py` |
| Topic 构建目录没有 `CMakeLists.txt` | 新版工程在 `Topic_SDK/topic_c++/topic_c++/`，`-S` 路径不能只写外层目录 |
| Topic 频率统计接口不存在 | 确认使用 `topic.py/topic_all_py/` 的新版封装和配套扩展，而非 `topic_py/` 或 `common/topic/` 中的旧库 |
| 连接后看不到发送/响应日志 | 默认 `debug=False` / `false`，可显式开启或使用响应回调；无日志不能证明没有下发命令 |
| 同步发送长时间等待 | 检查指令是否持续运行、响应时机及当前请求超时；缩短超时不会自动停止运动 |

## 参考文档

- [更新日志](CHANGELOG.md)：历史版本变更；当前行为以随附源码和接口定义为准。
- [指令手册 v1.7.5](指令手册-合-v1.7.5.pdf)
- RPC：[C++ 封装](common/rpc/c++/include/message/rpc_client.h)、[C++ 响应类型](common/rpc/c++/include/message/resp_dto.h)、[Python 封装](common/rpc/python/rpc_client.py)
- Linux RPC 环境：[C++ x86_64](common/rpc/c++/lib/linux/x86/README.md)、[C++ ARM64](common/rpc/c++/lib/linux/arm/README.md)、[Python x86_64](common/rpc/python/lib/linux/x86/cp310/README.md)、[Python ARM64](common/rpc/python/lib/linux/arm/cp310/README.md)
- 力位混合控制：[C++ 说明](ForcePositionHybridControl_SDK/ForcePositionHybridControl_linux_c++例程使用说明-v1.5.2.pdf)、[Python 说明](ForcePositionHybridControl_SDK/ForcePositionHybridControl_linux_python例程使用说明-v1.5.2.pdf)
- 子循环控制：[C++ 说明](SubLoop_SDK/SubLoop_linux_c++例程使用说明-v1.5.2.pdf)、[Python 说明](SubLoop_SDK/SubLoop_linux_python例程使用说明-v1.5.2.pdf)
