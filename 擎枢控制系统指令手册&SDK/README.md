# 机器人控制 SDK 使用手册

本仓库包含多个机器人运动控制 SDK，每个 SDK 提供 **Python** 和 **C++** 两种语言的示例程序，通过 RPC 协议与机器人控制器通信。

---

## 目录

- [SDK 功能一览](#sdk-功能一览)
- [目录结构说明](#目录结构说明)
- [⚠️ 重要注意事项](#️-重要注意事项)
- [Python SDK 快速开始](#python-sdk-快速开始)
- [C++ SDK 编译指南](#c-sdk-编译指南)
- [各 SDK 使用说明](#各-sdk-使用说明)
- [常见问题](#常见问题)

---

## SDK 功能一览

| SDK 名称 | 功能描述 |
|----------|----------|
| `MoveAbsJ_SDK` | 关节绝对运动（单臂） |
| `MoveAbsJ_Double_SDK` | 关节绝对运动（双臂协同） |
| `MoveBlend_SDK` | 混合插补运动（直线 + 圆弧轨迹拼接） |
| `MoveS_SDK` | S 型曲线平滑轨迹运动 |
| `MoveSeriesToppJ_SDK` | 多段关节轨迹连续运动（ToppJ 算法） |
| `JogAnyJ_SDK` | 任意关节连续点动（单臂） |
| `JogAnyJ_Double_SDK` | 任意关节连续点动（双臂协同） |
| `JogC_SDK` | 关节/笛卡尔空间步进点动 |
| `SubLoop_SDK` | 子循环指令（用于重复执行动作） |
| `SyncAsync_SDK` | 同步 / 异步 RPC 发送方式对比示例 |
| `IOModule_SDK` | IO 模块控制（读取输入 / 设置输出 / 脉冲） |
| `DragInCST_SDK` | CST 模式零力拖动 |
| `ForcePositionHybridControl_SDK` | 力位混合控制（恒力 + 轨迹运动） |
| `Topic_SDK` | 系统状态实时订阅（ZMQ + Protobuf） |

---

## 目录结构说明

```
代码/
├── common/
│   └── rpc/
│       ├── c++/              # C++ 公共 RPC 文件（头文件 + 库文件，替换即更新）
│       │   ├── include/      # 公共头文件
│       │   └── lib/          # 公共库文件（win / linux）
│       └── python/           # Python 公共 RPC 文件（替换即更新）
│           ├── rpc_client.py # 共享 RPC 客户端模块（唯一一份，所有 SDK 共用）
│           ├── win/          # Windows 平台（基于 Python 3.10 编译）
│           ├── x86/          # Linux x86/x64 平台
│           └── arm/          # Linux ARM 平台
│
├── MoveAbsJ_SDK/
│   ├── MoveAbsJ_py/      # Python 示例（运行入口：main.py）
│   └── MoveAbsJ_c++/     # C++ 示例（需 CMake 编译）
│
├── MoveBlend_SDK/        # 结构同上，其余 SDK 类似
│   ...
└── Topic_SDK/
    ├── topic_py/         # Python 订阅示例
    └── topic_c++/        # C++ 订阅示例
```

---

## ⚠️ 重要注意事项

### 1. 必须保持完整目录结构

所有 Python SDK 的底层库文件（`rpc.pyd` / `rpc.so`）和共享客户端模块（`rpc_client.py`）统一存放在 **`common/rpc/python/`** 目录中。每个 SDK 的 `main.py` 通过相对路径自动定位到该目录。

> **注意**：底层库基于 **Python 3.10 ABI** 编译，其他版本（如 3.11、3.12）无法使用。

**更新 RPC 文件时只需替换 `common/rpc/c++/`（C++）或 `common/rpc/python/`（Python）下的文件即可，无需修改任何 SDK 代码。**

**如果只把单个 SDK 文件夹复制到其他地方单独运行，程序会报错找不到库文件。**

> 正确做法：**克隆/下载整个仓库**，保持目录层级不变，再在对应 SDK 的 py 目录下运行 `main.py`。

### 2. 运行前必须配置机器人 IP 地址

运行前需要**手动修改** `main.py`（Python）或 `main.cpp`（C++）中的 IP 变量：

- **Python SDK**：修改 `main.py` 顶部的 `ROBOT_IP` 变量：

  ```python
  ROBOT_IP = "192.168.2.199"   # ← 改成你的机器人实际 IP
  ```

- **C++ SDK**：修改 `main.cpp` 中的 `robot_ip` 变量：

  ```cpp
  std::string robot_ip = "192.168.2.199";  // ← 改成你的机器人实际 IP
  ```

**IP 不对将无法连接机器人**，程序会超时挂起。

### 3. Topic_SDK 独立于其他 SDK

`Topic_SDK` 使用独立的 ZMQ + Protobuf 库（存放在 `topic_py/lib/` 内部），与其他 SDK 的共享库无关，可独立使用。

---

## Python SDK 快速开始

### 环境要求

- Python **3.10.x**（底层库基于 3.10 ABI 编译，**其他版本不支持**）
- 操作系统：Windows x64 / Linux x86_64 / Linux ARM

> 下载地址：[Python 3.10.11](https://www.python.org/downloads/release/python-31011/)

### 运行步骤

以 `MoveAbsJ_SDK` 为例（其他 SDK 步骤完全相同）：

**运行前**请先修改 `main.py` 顶部的 `ROBOT_IP` 为你的机器人实际 IP。

**Windows：**

```bash
cd MoveAbsJ_SDK/MoveAbsJ_py
py -3.10 main.py
```

**Linux：**

```bash
cd MoveAbsJ_SDK/MoveAbsJ_py
python3.10 main.py
```

> 所有 Python SDK 结构一致，替换路径即可运行其他 SDK。

---

## C++ SDK 编译指南

### 环境要求

| 项目 | 要求 |
|------|------|
| CMake | >= 3.12 |
| C++ 标准 | C++17 |
| 编译器（Windows） | Visual Studio 2017+（推荐 2019 16.6+，见下方说明） |
| 编译器（Linux） | GCC 7.0+ |

> **Visual Studio 版本说明**
>
> | VS 版本 | MSVC_VERSION | 支持的编译选项 | 说明 |
> |---------|-------------|--------------|------|
> | VS 2019 16.6+ | >= 1926 | `/std:c++17 /Zc:preprocessor /utf-8` | **推荐**，完整支持 |
> | VS 2017 | < 1926 | `/std:c++17 /utf-8` | 可用，但不启用新预处理器 |
>
> `/Zc:preprocessor` 启用符合标准的新预处理器，解决 `__VA_ARGS__` 宏展开兼容问题。如果使用 VS 2017，CMake 会自动跳过此选项，编译不受影响。
>
> **源文件编码**：所有 `.cpp` / `.h` / `.hpp` 文件应以 **UTF-8** 编码保存（CMake 已自动添加 `/utf-8` 编译选项）。

### 编译步骤

编译前请先修改 `main.cpp` 中的 `robot_ip` 为你的机器人实际 IP。

**Windows：**

```bash
cd MoveAbsJ_SDK/MoveAbsJ_c++
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

编译完成后可执行文件位于 `build/Release/` 目录中。

**Linux：**

```bash
cd MoveAbsJ_SDK/MoveAbsJ_c++
mkdir build && cd build
cmake ..
make -j$(nproc)
```

> 所有 C++ SDK 结构一致，替换路径即可编译其他 SDK。

---

## 各 SDK 使用说明

### MoveAbsJ — 关节绝对运动（单臂）

发送绝对关节角度，机器人运动到目标关节位置。直接运行 `main.py`，程序会循环执行 `motion_cmds` 中的预设指令。

修改 `main.py` 中的 `init_cmds`（初始化指令 + 定义关节变量）和 `motion_cmds`（运动目标列表）以调整运动参数。

> C++ 版本采用**三段式 demo 结构**：
>
> - **示例 1**：`robot::send_rpcsy` 同步发送 `init_cmds`
> - **示例 2**：`robot::send_rpc_async` 异步发送 `motion_speedL_cmds`（默认注释）
> - **示例 3**：扩展返回值处理（`PointChooseIDMoveResp`，默认注释）
>
> 如需在程序里执行运动，把示例 1 的 `init_cmds` 换成 `motion_cmds`，或自行追加 `robot::send_rpcsy(*client, motion_cmds, ...)` 调用即可。

---

### MoveAbsJ_Double — 关节绝对运动（双臂协同）

双臂版关节绝对运动，通过 `||` 分隔符同时控制两台机械臂。结构同 MoveAbsJ，但 `init_cmds` 中需为双臂分别定义关节变量（如 `j0~j4` 和 `j11~j24`），`motion_cmds` 中使用 `||` 连接双臂指令：

```python
"{MoveAbsJ --jointtarget_var=j0||MoveAbsJ --jointtarget_var=j11}"
```

> 同时定义了 `motion_speedL_cmds`（双臂 SpeedL 在线规划指令），可用于异步发送。
>
> C++ 版本采用与 MoveAbsJ 一致的**三段式 demo 结构**（示例 1 同步 / 示例 2 异步 / 示例 3 扩展返回），如需实际执行双臂运动，把示例 1 的 `init_cmds` 换成 `motion_cmds` 即可。

---

### MoveBlend — 混合插补运动

支持直线段和圆弧段的平滑拼接运动。程序启动后出现交互菜单：

```
first_insert  - 添加起点（当前位置）
add_line      - 添加直线轨迹点
add_circle    - 添加圆弧轨迹点（需要中间点）
start         - 执行轨迹
clear_points  - 清除所有轨迹点
show_points   - 显示当前轨迹点
exit          - 退出程序
```

坐标格式为 `x,y,z,q1,q2,q3,q4`（单位：米，四元数姿态）。添加轨迹点时可指定 `zone`（过渡区）和 `speed`（运动速度）参数。

---

### MoveS — S 型曲线运动

平滑轨迹运动，适合对运动柔顺性有要求的场景。菜单操作：

```
start_moves    - 设置起点（当前位置）
add_moves      - 添加 MoveS 轨迹点（输入格式：x,y,z,q1,q2,q3,q4）
execute        - 执行轨迹
clear_points   - 清除所有轨迹点
show_points    - 显示当前轨迹点
exit           - 退出程序
```

---

### MoveSeriesToppJ — 多段关节轨迹运动

使用 ToppJ 时间最优算法规划多段关节轨迹。菜单操作：

```
1 - 设置起点（当前位置）
2 - 添加关节轨迹点（输入6个关节角，单位：度）
3 - 设置速度系数（0.1 ~ 1.0）
4 - 设置加速度系数（0.1 ~ 1.0）
5 - 执行轨迹
6 - 清空轨迹点
7 - 查看已添加的点
8 - 安全停止并退出
```

---

### JogAnyJ — 任意关节点动（单臂）

移动机器人到指定关节角度位置。菜单操作：

```
start   - 启动 JogAnyJ 控制（移动到全零位）
stop    - 停止运动
custom  - 手动输入目标关节角度（单位：弧度）和速度
exit    - 停止后退出
```

修改 `main.py` 中的 `NUM_JOINTS` 以匹配机器人轴数（默认 7）。

---

### JogAnyJ_Double — 任意关节点动（双臂协同）

双臂版 JogAnyJ，通过 `||` 分隔符同时控制两台机械臂。菜单操作：

```
start   - 启动双臂 JogAnyJ 控制（双臂到零位）
home    - 双臂 MoveAbsJ 回零
stop    - 停止运动
custom  - 输入自定义双臂关节位置（弧度）
exit    - 停止后退出
```

`custom` 模式下需分别输入左右臂的关节角度和运动速度。修改 `NUM_JOINTS_PER_ARM` 以匹配轴数（默认 7）。

---

### JogC — 步进点动

按步长控制机器人在关节或笛卡尔空间内步进运动。直接运行 `main.py`，程序会循环执行 `motion_cmds` 中预设的步进指令。

修改 `motion_cmds` 中的参数可调整方向、步长和速度：

```python
"{JogC --motion_type=0 --direction=1 --step=0.1 --coordinate=0 --speed=v100}"
#        关节模式=0       方向           步长(rad)   关节坐标系
```

> C++ 版本采用与 MoveAbsJ 一致的**三段式 demo 结构**（示例 1 同步 / 示例 2 异步 / 示例 3 扩展返回）。如需实际执行 JogC 步进，把示例 1 的 `init_cmds` 换成 `motion_cmds` 即可。

---

### SyncAsync — 同步/异步发送对比

演示两种 RPC 发送方式的区别，启动后出现交互菜单：

```
sync    - 运行同步发送（MoveAbsJ 关节运动，逐条等待响应）
async   - 运行异步发送（SpeedL 往返运动，快速下发不阻塞）
compare - 同步 vs 异步 对比（先跑同步再跑异步，可对比耗时）
clear   - 清除所有变量
exit    - 退出程序
```

- **同步（send_rpcsy）**：发送后等待机器人执行完毕再发下一条，适合需要确认执行结果的场景
- **异步（send_rpc_async）**：发送后不等待，立即发下一条，适合持续控制类指令（如 SpeedL）

> C++ 版本和 Python 版本的菜单结构完全一致，均可交互选择模式。如需切换预设的同步/异步指令列表，修改 `sync_cmds` / `async_cmds` 即可。

---

### SubLoop — 子循环指令

用于向机器人发送 SubLoop 格式的循环控制指令（双模型格式 `cmd1||cmd2`）。程序运行后进入交互模式，直接在控制台输入指令字符串并回车发送。

---

### IOModule — IO 模块控制

控制机器人的数字输入/输出模块。菜单操作：

```
getdi    - 读取数字输入（DI）
setdo    - 设置数字输出（DO）
dopulse  - 脉冲输出（DO 定时高低电平）
exit     - 退出程序
```

修改 `main.py` 中的指令参数（如 `DO0`、`do_value` 等）以适配实际 IO 通道。

---

### DragInCST — CST 拖动

切换机器人到 CST（力矩控制）模式并启动零力拖动。交互菜单：

```
start   - 切换 CST 模式并开始拖动
stop    - 停止拖动，切回 CSP 位置模式
```

---

### ForcePositionHybridControl — 力位混合控制

在运动过程中叠加力控制。修改 `main.py` 顶部的模式变量选择场景：

```python
DRAG_MODE = "free_drag"      # 六自由度零力拖动
# DRAG_MODE = "z_force"      # 仅 Z 方向恒力控制
# DRAG_MODE = "hybrid_move_z"  # 直线运动 + Z 方向力位混合
```

---

### Topic — 系统状态订阅

通过 ZMQ 消息总线订阅机器人实时（RT）和非实时（NRT）状态数据，Topic SDK 自带独立库文件，**不依赖** `common/rpc/`。

#### Python 版本

**第一步：修改发布者 IP**

打开 `Topic_SDK/topic_py/main.py`，修改：

```python
PUBLISHER_IP = "192.168.2.140"   # 改成机器人控制器的实际 IP
```

> 端口固定为 `19091`，无需修改。

**第二步：运行**

```bash
cd Topic_SDK/topic_py
python main.py
```

程序启动后自动订阅并循环输出：
- **RT 实时数据**（关节位置、力矩、末端姿态等）：每 **1 秒**打印一次
- **NRT 非实时数据**（工具、工件、关节限制等配置）：每 **5 秒**打印一次
- 按 `Ctrl+C` 退出

**Linux 已知问题**

若运行时报 `ImportError: undefined symbol: _ZN6google8protobuf...`，使用以下方式启动：

```bash
LD_PRELOAD=./lib/x86/libprotobuf.so.32 python3 main.py
# ARM 平台：
LD_PRELOAD=./lib/arm/libprotobuf.so.32 python3 main.py
```

#### C++ 版本

C++ 版本提供**两种订阅模式**，对应 `topic_c++/src/` 下的两个源文件：

| 文件 | 模式 | 说明 |
|------|------|------|
| `topic_sub_direct.cpp` | 直接订阅模式 | 通过回调函数实时接收每帧数据，延迟最低 |
| `topic_sub_snapshot.cpp` | 快照订阅模式 | 主动查询当前最新状态，适合定时轮询场景 |

编译前在 `CMakeLists.txt` 中选择编译哪个源文件，或参考 `Topic_SDK/README.md` 获取详细编译步骤。

修改 C++ 程序中的连接 IP：

```cpp
std::string remote_ip = "192.168.2.140";  // 改为实际 IP
```

---

## 常见问题

### Q: 运行提示 `Python 3.10 is required` 或模块加载失败

**原因**：底层动态库（`.pyd` / `.so`）基于 Python 3.10 ABI 编译，其他版本（如 3.11、3.12）无法加载。

**解决**：安装 Python 3.10.x，下载地址：[Python 3.10.11](https://www.python.org/downloads/release/python-31011/)。安装时勾选 **"Add Python to PATH"**。如果电脑上已安装多个 Python 版本，使用 `py -3.10 main.py`（Windows）启动器定位到 3.10。

---

### Q: 运行 `python main.py` 提示 `Platform directory not found`

**原因**：`common/rpc/python/` 目录不存在或路径结构被破坏。

**解决**：确保你是在**完整仓库目录**下运行，不要只复制单个 SDK 文件夹。

---

### Q: 程序卡住不动，没有输出

**原因**：机器人 IP 地址不对，连接超时。

**解决**：检查并修改 `main.py` 中的 `ROBOT_IP`，确保 PC 与机器人在同一局域网内，并可以 ping 通。

---

### Q: Windows 提示缺少 DLL

**原因**：Visual C++ 运行时库缺失。

**解决**：安装 [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)（2015~2022 合并包）。

---

### Q: 如何切换同步/异步模式

程序启动后出现交互菜单，输入对应命令即可：

```
sync    - 运行同步发送
async   - 运行异步发送
compare - 同步 vs 异步 对比
```

如需修改预设的指令内容，编辑 `main.py`（或 C++ 的 `main.cpp`）中的 `sync_cmds`（同步指令列表）和 `async_cmds`（异步指令列表）。

---

### Q: Linux 上运行提示 `cannot open shared object file`

**解决**：`rpc_client.py` 已自动配置 `LD_LIBRARY_PATH`，如果仍报错，请手动执行：

```bash
export LD_LIBRARY_PATH=<仓库根目录>/common/rpc/python/x86:$LD_LIBRARY_PATH  # x86
# 或
export LD_LIBRARY_PATH=<仓库根目录>/common/rpc/python/arm:$LD_LIBRARY_PATH  # ARM
```

---

### Q: 如何新增一个自定义指令

在对应 SDK 的 `main.py` 中找到 `your_cmds` 列表，添加指令字符串：

```python
your_cmds = [
    "{YourInstruction --param1=value1 --param2=value2}",
]
```

然后调用 `send_rpcsy(client, your_cmds, 5000, 0.5)` 发送即可。

---

> 如有问题，请在 GitHub Issues 中反馈。
