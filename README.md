# 擎枢控制系统

擎枢控制系统是南京臻识科技有限公司开发的机器人控制系统，本仓库包含配套的**使用文档**、**指令手册与 SDK** 以及**最小上位机例程**。

---

## 仓库结构

```
.
├── 工智AI工艺包使用文档/               # 工智AI工艺包使用说明
├── 擎枢控制系统指令手册&SDK/           # 指令手册与 15 个 SDK 示例（C++ / Python）
├── 擎枢控制系统最小上位机例程/         # 基于 Topic + RPC 的最小上位机例程
└── Web使用手册v1.7.3.pdf              # Web 端使用手册
```

---

## 三大模块简介

### 1. 工智AI工艺包使用文档

工智AI工艺包的使用说明文档，帮助用户快速上手工艺包功能。

### 2. 擎枢控制系统指令手册&SDK

包含**控制器指令手册**以及 **15 个 SDK 示例模块**，每个模块均提供 C++ 和 Python 双语言版本：

| 分类 | SDK | 功能 |
|------|-----|------|
| 关节运动 | MoveAbsJ、MoveAbsJ_Double | 单臂/双臂关节绝对位置运动 |
| 笛卡尔运动 | MoveBlend、MoveS | 笛卡尔空间混合轨迹 / S 曲线轨迹 |
| 轨迹运动 | MoveSeriesToppJ | 关节连续 Topp 轨迹 |
| 点动控制 | JogC、JogAnyJ、JogAnyJ_Double | 笛卡尔/关节空间点动 |
| 力控 | DragInCST、ForcePositionHybridControl | 拖动示教 / 力位混合控制 |
| IO 控制 | IOModule | DI / DO / 脉冲输出 |
| 通信模式 | SyncAsync、SubLoop | 同步异步对比 / 子循环控制 |
| 数据订阅 | Topic | 实时状态数据订阅（ZMQ + Protobuf） |

> 详细说明见 `擎枢控制系统指令手册&SDK/README.md`

### 3. 擎枢控制系统最小上位机例程

基于 Topic（数据订阅）+ RPC（远程调用）的最小化上位机示例，展示如何快速搭建与控制器通信的上位机程序。

---

## 环境要求

| 项目 | 要求 |
|------|------|
| C++ 标准 | C++17 |
| CMake | ≥ 3.12 |
| Python | 3.10 |
| 编译器 (Windows) | MSVC (Visual Studio 2015+) |
| 编译器 (Linux) | GCC 7.0+ |

---

## 参考文档

| 文档 | 位置 |
|------|------|
| SDK 详细说明 | `擎枢控制系统指令手册&SDK/README.md` |
| 指令手册 | `擎枢控制系统指令手册&SDK/指令手册-合-v1.7.5.pdf` |
| Web 使用手册 | `Web使用手册v1.7.3.pdf` |
| Topic SDK API | `擎枢控制系统指令手册&SDK/Topic_SDK/topic_c++/API.md` |

---

## 许可证

本项目采用 [Apache License 2.0](LICENSE) 开源许可证。
