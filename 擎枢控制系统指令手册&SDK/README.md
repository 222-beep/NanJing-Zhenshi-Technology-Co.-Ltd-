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
│   ├── include/          # C++ 公共头文件
│   ├── lib/              # C++ 公共库文件（win / linux）
│   └── py_lib/           # ⭐ Python 公共库（所有 Python SDK 共用）
│       ├── win/          # Windows 平台
│       ├── x86/          # Linux x86/x64 平台
│       └── arm/          # Linux ARM 平台
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

所有 Python SDK 的底层库文件统一存放在 **`common/py_lib/`** 目录中。每个 SDK 的 `arch.py` 通过相对路径自动定位到该目录。

**如果只把单个 SDK 文件夹复制到其他地方单独运行，程序会报错找不到库文件。**

> 正确做法：**克隆/下载整个仓库**，保持目录层级不变，再在对应 SDK 的 py 目录下运行 `main.py`。

### 2. 运行前必须修改机器人 IP 地址

每个 `main.py` 文件顶部都有一行：

```python
ROBOT_IP = "192.168.2.199"   # ← 改成你的机器人实际 IP
```

**IP 不对将无法连接机器人**，程序会超时挂起。

### 3. Topic_SDK 独立于其他 SDK

`Topic_SDK` 使用独立的 ZMQ + Protobuf 库（存放在 `topic_py/lib/` 内部），与其他 SDK 的共享库无关，可独立使用。

---

## Python SDK 快速开始

### 环境要求

- Python **3.7** 或以上版本
- 操作系统：Windows x64 / Linux x86_64 / Linux ARM

### 运行步骤

以 `MoveAbsJ_SDK` 为例（其他 SDK 步骤完全相同）：

**第一步：修改机器人 IP**

打开 `MoveAbsJ_SDK/MoveAbsJ_py/main.py`，找到并修改：

```python
ROBOT_IP = "192.168.2.199"   # 改成你的机器人 IP
```

**第二步：运行程序**

```bash
cd MoveAbsJ_SDK/MoveAbsJ_py
python main.py
```

> Windows 用户也可以直接在资源管理器中双击 `main.py`（前提是已关联 Python）。

---

## C++ SDK 编译指南

### 环境要求

| 项目 | 要求 |
|------|------|
| CMake | >= 3.12 |
| C++ 标准 | C++17 |
| 编译器（Windows） | MSVC（Visual Studio 2019+） |
| 编译器（Linux） | GCC 7.0+ |

### Windows 编译（以 MoveAbsJ 为例）

```bash
cd MoveAbsJ_SDK/MoveAbsJ_c++
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

编译完成后可执行文件位于 `build/Release/` 目录中。

### Linux 编译

```bash
cd MoveAbsJ_SDK/MoveAbsJ_c++
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## 各 SDK 使用说明

### MoveAbsJ — 关节绝对运动

发送绝对关节角度，机器人运动到目标关节位置。

```python
ROBOT_IP = "192.168.2.199"
```

修改 `main.py` 中的 `motion_cmds` 列表，调整目标关节角度后运行即可。

---

### MoveBlend — 混合插补运动

支持直线段和圆弧段的平滑拼接运动。程序启动后出现交互菜单：

```
first_insert  - 设置起点（当前位置）
add_line      - 添加直线轨迹点
add_circle    - 添加圆弧轨迹点
start         - 执行轨迹
clear_points  - 清除所有轨迹点
exit          - 退出程序
```

坐标格式为 `x,y,z,q1,q2,q3,q4`（单位：米，四元数姿态）。

---

### MoveS — S 型曲线运动

平滑轨迹运动，适合对运动柔顺性有要求的场景。菜单操作：

```
start_moves   - 设置起点
add_moves     - 添加轨迹点（输入格式：x,y,z,q1,q2,q3,q4）
execute       - 执行轨迹
clear_points  - 清空轨迹点
exit          - 退出
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

### JogAnyJ — 任意关节点动

移动机器人到指定关节角度位置。菜单操作：

```
start   - 移动到初始位置（全零位）
stop    - 停止运动
custom  - 手动输入目标关节角度（单位：弧度）和速度
exit    - 停止后退出
```

修改 `main.py` 中的 `NUM_JOINTS` 以匹配机器人轴数（默认 7）。

---

### JogC — 步进点动

按步长控制机器人在关节或笛卡尔空间内步进运动。直接运行 `main.py`，程序会循环执行 `motion_cmds` 中预设的步进指令。

修改 `motion_cmds` 中的参数可调整方向、步长和速度：

```python
"{JogC --motion_type=0 --direction=1 --step=0.1 --coordinate=0 --speed=v100}"
#        关节模式=0       方向           步长(rad)   关节坐标系
```

---

### SyncAsync — 同步/异步发送对比

演示两种 RPC 发送方式的区别：
- **同步（send_rpcsy）**：发送后等待机器人执行完毕再发下一条
- **异步（send_rpc_async）**：发送后不等待，立即发下一条

在 `main.py` 中通过注释/反注释切换：

```python
# arch.send_rpc_async(client, motion_cmds, 10000, 0.5)  # 异步
arch.send_rpcsy(client, motion_cmds, 10000, 0.5)        # 同步（当前激活）
```

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

通过 ZMQ 消息总线订阅机器人实时（RT）和非实时（NRT）状态数据，Topic SDK 自带独立库文件，**不依赖** `common/py_lib/`。

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

### Q: 运行 `python main.py` 提示 `Platform directory not found`

**原因**：`common/py_lib/` 目录不存在或路径结构被破坏。

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

在 `main.py` 的主循环中找到以下两行，注释其中一行：

```python
# arch.send_rpc_async(client, cmds, 10000, 0.5)  # 异步：发完不等响应
arch.send_rpcsy(client, cmds, 10000, 0.5)        # 同步：等响应后再发下一条
```

---

### Q: Linux 上运行提示 `cannot open shared object file`

**解决**：`arch.py` 已自动配置 `LD_LIBRARY_PATH`，如果仍报错，请手动执行：

```bash
export LD_LIBRARY_PATH=<仓库根目录>/common/py_lib/x86:$LD_LIBRARY_PATH  # x86
# 或
export LD_LIBRARY_PATH=<仓库根目录>/common/py_lib/arm:$LD_LIBRARY_PATH  # ARM
```

---

### Q: 如何新增一个自定义指令

在对应 SDK 的 `main.py` 中找到 `your_cmds` 列表，添加指令字符串：

```python
your_cmds = [
    "{YourInstruction --param1=value1 --param2=value2}",
]
```

然后调用 `arch.send_rpcsy(client, your_cmds, 5000, 0.5)` 发送即可。

---

> 如有问题，请在 GitHub Issues 中反馈。
