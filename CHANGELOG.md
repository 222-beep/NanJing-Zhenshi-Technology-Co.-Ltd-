# 更新日志 (CHANGELOG)

本文件记录本仓库的所有重大变更。

---

## 2026-08-21

### 同步 Gitee 更新（指令手册&SDK）

- 源仓库内容更新（无新提交记录）

## 2026-08-21

### 同步 Gitee 更新（指令手册&SDK）

- **新增** `RealSenseCamera_SDK`（SD-18 两路 RealSense HTTP MJPEG 图像获取，Python），SDK 示例模块由 17 个增至 **18 个**；Topic 编号相应调整为 SD-17
- **新增** 仓库根目录 `.gitignore`（忽略 `__pycache__`、`build/`、`*.obj` 等缓存与编译产物）
- **同步** `擎枢控制系统指令手册&SDK` 目录及模块 `README.md` 至 Gitee 最新版；根目录 `README.md` 同步模块数量与分类

## 2026-08-20

### 更新擎枢控制系统指令手册&SDK（新增 ReadPdo / ReadSdo EtherCAT 数据读取）

- **新增** `ReadPdo_SDK`（SD-15）与 `ReadSdo_SDK`（SD-16）EtherCAT 数据读取示例（C++ / Python 双语言），SDK 示例模块由 15 个增至 **17 个**
- **更新** `common/rpc/c++/include/message/resp_dto.h`：恢复/新增 `RespPdo` / `RespSdo` 响应类型
- **修复** `MoveSeriesToppJ_SDK` C2280 编译错误
- **新增** Topic 套接字状态订阅示例（`topic_sub_socket_state.cpp` / `topic_sub_socket_state.py`）
- **更新** `擎枢控制系统指令手册&SDK/README.md`（模块列表更新至 17 个）与根目录 `README.md`

---

## 2026-08-14

### 更新擎枢控制系统指令手册&SDK（Topic 库结构精简 + 状态读取层）

- **更新** `common/topic/` 公共库结构：
  - Linux 库按架构平铺：`c++/lib/linux/{arm,x86}/` 与 `python/lib/linux/{arm,x86}/`，移除 `20.04/22.04` 版本化目录及 `common/topic/shared/`
  - Python 扩展按 CPython ABI 分目录：`lib/linux/{arm,x86}/cp310/topic.so`、`lib/win/cp310/topic.pyd`
  - Windows 补充 `libzmq-v142-mt-4_3_6.dll`
- **新增** 系统状态只读访问层：C++ `system_state_reader.hpp` 与 Python `system_state_reader.py`（Direct / Snapshot 双模式）
- **新增** `Topic_SDK/topic_py/platform_loader.py` 平台检测与动态库自动加载
- **更新** protobuf 消息：`overall_system_nrtstate.proto` 增加 `matrix_variables`、`drag_in_cst_coef`、`inf_rngs` 字段，并同步重新生成 pb 文件；新增 `proto_generated/` 预生成兜底与 `message_struct/` 解析结构体示例
- **更新** `Topic_SDK/README.md`、`topic_c++/API.md`、`topic_py/API.md` 及根目录 `README.md`
- **删除** `common/rpc/python/__init__.py` 与仓库中的 `__pycache__` 缓存文件

---

## 2026-08-11

### 更新擎枢控制系统指令手册&SDK（新增 JogAnyC，RPC 库结构精简）

- **新增** `JogAnyC_SDK`（SD-09 笛卡尔空间任意位姿控制，C++ / Python 双语言），SDK 示例模块由 14 个增至 **15 个**
- **更新** `common/rpc/` 公共库结构：
  - `resp_dto.h` 移至 `c++/include/message/`（与 `rpc_client.h` 同层）
  - Linux 平台库精简：移除 `2004/2204` 版本化目录，C++ 库直挂 `lib/linux/{arm,x86}/libcpp_rpc.so`，Python 库为 `linux/{arm,x86}/cp310/rpc.so`
  - `rpc_client.py` 更新
- **新增** `JogAnyJ_SDK` 正弦点动示例（`main-sine.cpp` / `main-sine.py`）
- **更新** `擎枢控制系统指令手册&SDK/README.md`、`Topic_SDK/README.md` 与 `指令手册-合-v1.7.5.pdf`
- **更新** 根目录 `README.md`，同步模块数量（15 个）与分类

---

## 2026-07-22

### 更新擎枢控制系统最小上位机例程（基础 SDK 拆分与更新）

- **更新** 资源包为 Gitee 最新版本（`sdk_send_recevice_demo_c`、`sdk_send_recevice_demo_py`、`ros1_sdk`）：
  - **新增** `Base_SDK_c++.zip` — 基础 SDK（C++）：底层 RPC 客户端（`robot_command`）+ 状态订阅（`robot_state`，ZMQ + Protobuf）
  - **新增** `Base_SDK_py.zip` — 基础 SDK（Python）：RPC 客户端（`robot_command`）+ 状态订阅（`robot_state`）+ 运行时封装（`robot_runtime.py`）及示例 `main.py`
  - **更新** `Ros.zip` — ROS1 集成包更新至最新（示例统一置于 `ros/` 目录下）
- **删除** `Base_SDk.zip`（原 C++ & Python 合并基础 SDK，已由 `Base_SDK_c++.zip` 与 `Base_SDK_py.zip` 拆分替代）
- **更新** 根目录 `README.md`，同步最小上位机例程资源包列表

---

## 2026-07-21

### 更新擎枢控制系统指令手册&SDK（RPC 公共库重构）

- **RPC 库重构**：`common/rpc/` 全面重构，影响全部 14 个 SDK 示例
  - **C++ API 入口变更**：由 `#include "robot.hpp"` 改为 `#include "rpc_client.h"`
  - **C++ include**：移除内置 `google/protobuf/` 头文件树、`zmq.h/hpp`、`tdsocket_global.h`、`robot.hpp`；新增 `task_pool.hpp`、`message/rpc_client.h`；JSON 工具（`easy_json.h`、`json.hpp`）移至 `util/reflection/`
  - **C++ lib**：移除 `robot_sdk`（dll/lib/so），仅保留 `cpp_rpc`；平台目录命名调整（`20.04→2004`、`22.04→2204`）
  - **Python lib**：移除 `robot_ext`（so/pyd），仅保留 `rpc`；平台目录命名同步调整
- **全部 14 个 SDK 示例更新**：各模块 `main.cpp`、`main.py`、`CMakeLists.txt` 均改用新版 RPC 客户端 API 与库引用路径
- **文档修正**：SDK README 模块计数由 15 个更正为 14 个（Topic 为独立通信库，不与 13 个标准 RPC SDK 重复计入）；同步更新根目录 `README.md`

---

## 2026-06-29

### SDK 使用手册格式更换

- **替换** `SDK使用手册.docx` 为 `SDK使用手册.pdf`（PDF 格式更便于分发和查看）

### 更新擎枢控制系统最小上位机例程

- **删除** `topic+rpc_c++` 目录（原 Topic + RPC 通信示例）
- **新增** 以下资源压缩包：
  - `Base_function_SDK_c++.zip` — 高层指令封装 SDK（C++），在 Base_SDK 基础上将函数封装为指令接口，支持单臂/双臂/多臂指令发送，同时含 `robot_state` 状态订阅模块
  - `Base_SDk.zip` — 基础 SDK（C++ & Python 双语言），包含底层 RPC 客户端（`robot_command`）和状态订阅（`robot_state`，基于 ZMQ + Protobuf），提供基础通信能力
  - `Ros.zip` — ROS1 集成包，含 `robot_demo_system`、`user_receiver` 示例及 MuJoCo 仿真工作空间（`mujoco_ros1`、`topic_to_mujoco`）
  - `SDK使用手册.pdf` — SDK 使用手册（PDF 文档）
- **更新** 根目录 `README.md`，补充各压缩包详细说明

---

## 2026-06-18

### 更新 SDK 至最新版本

- **更新** `擎枢控制系统指令手册&SDK` 文件夹为最新 SDK 文件
- **新增** `common/topic/` 目录（Topic 通信相关 C++ / Python 库及 ARM、x86 平台动态库）
- **更新** 指令手册 PDF 至 `v1.7.5` 版本

---

## 2026-06-15 ~ 2026-06-16

### 仓库重构（重大变更）

**将原有 4 个分支合并为 main 分支下的 3 个子目录：**

- **新增** `工智AI工艺包使用文档/` 目录 — 工智 AI 工艺包使用说明文档
- **新增** `擎枢控制系统指令手册&SDK/` 目录 — 包含 15 个 SDK 示例模块（C++ / Python）及指令手册
  - 涵盖：关节运动、笛卡尔运动、轨迹运动、点动控制、力控、IO 控制、通信模式、数据订阅
- **新增** `擎枢控制系统最小上位机例程/` 目录 — 最小化上位机通信示例
- **删除** 原有 3 个分支：`工智AI工艺包使用文档`、`擎枢控制系统指令手册&SDK`、`擎枢控制系统最小上位机例程`
- **新增** 根目录 `README.md` — 项目总体介绍、仓库结构、三大模块简介、参考文档索引
- **新增** 备份分支 `backup-before-reorg-20260615`（重构前完整备份）

---

## 2026-05-20

### SDK 整理与清理

- **删除** 旧版 `指令手册-合-v1.7.4.pdf` 和 `Web使用手册v1.7.2.pdf`
- **删除** 多余的 `MoveBlend_SDK.zip`、`MoveAbsJ_SDK.zip`
- **更新** `Web使用手册` 至 `v1.7.3` 版本

---

## 2026-03-18 ~ 2026-04-16

### 仓库初始建立

- **初始化** 仓库，上传各 SDK 模块（DragInCST、ForcePositionHybridControl、IOModule、JogAnyJ、JogC、MoveAbsJ、MoveBlend、MoveS、MoveSeriesToppJ、SubLoop、SyncAsync、Topic 等）
- **上传** `指令手册-合-v1.7.4.pdf`、`Web使用手册v1.7.2.pdf`
- **创建** 3 个独立分支分别存放文档、SDK 和上位机例程


