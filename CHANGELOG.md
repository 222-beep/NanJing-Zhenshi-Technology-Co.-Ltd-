# 更新日志 (CHANGELOG)

本文件记录本仓库的所有重大变更。

---

## 2026-06-29

### 更新擎枢控制系统最小上位机例程

- **删除** `topic+rpc_c++` 目录（原 Topic + RPC 通信示例）
- **新增** 以下资源压缩包：
  - `Base_function_SDK_c++.zip` — 高层指令封装 SDK（C++），在 Base_SDK 基础上将函数封装为指令接口，支持单臂/双臂/多臂指令发送，同时含 `robot_state` 状态订阅模块
  - `Base_SDk.zip` — 基础 SDK（C++ & Python 双语言），包含底层 RPC 客户端（`robot_command`）和状态订阅（`robot_state`，基于 ZMQ + Protobuf），提供基础通信能力
  - `Ros.zip` — ROS1 集成包，含 `robot_demo_system`、`user_receiver` 示例及 MuJoCo 仿真工作空间（`mujoco_ros1`、`topic_to_mujoco`）
  - `SDK使用手册.docx` — SDK 使用手册（Word 文档）
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
