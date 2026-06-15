# SystemStateReader API 文档

## 概述

SystemStateReader 是 `SharedSystemState` 之上的只读访问层，提供两种调用方式：

| 方式 | 示例文件 | 特点 |
|------|----------|------|
| 自由函数（Direct） | `topic_sub_direct.cpp` | 一步到位，直接调函数，适合快速获取少量字段 |
| 快照（Snapshot） | `topic_sub_snapshot.cpp` | 先拿快照再逐字段访问，适合需要批量读取的场合 |

两种方式共享同一套字段和命名体系，自由函数底层也是基于快照实现。

---

## 快速开始

```cpp
#include "system_state_reader.hpp"

// 启动订阅
start_subscriber("192.168.2.216");

// ===== 方式一：自由函数（一步到位）=====
if (hasRtData()) {
    double pos = getJointPosition(0, 2);    // 模型0, 关节2 的位置
    std::cout << pos << std::endl;
}

// ===== 方式二：快照（取一次，多次读）=====
auto rt = SystemStateReader::snapshotRt();
if (rt.valid()) {
    double pos = rt.jointPosition(0, 2);    // 同上
    double vel = rt.jointVelocity(0, 2);    // 不重复获取快照
    std::cout << pos << ", " << vel << std::endl;
}
```

---

## 1. 顶层字段

### 自由函数

| 函数 | 返回值 | 数据来源 | 说明 |
|------|--------|----------|------|
| `hasRtData()` | `bool` | — | 是否有 RT 数据 |
| `hasNrtData()` | `bool` | — | 是否有 NRT 数据 |
| `getHeaderTimestamp()` | `int64_t` | RT | 消息时间戳 |
| `getHeaderFrameId()` | `int64_t` | RT | 帧 ID |
| `isSystemRunning()` | `bool` | RT | 系统是否运行中 |
| `getSystemInfo()` | `std::string` | RT | 系统启停等信息 |
| `isSystemInit()` | `bool` | NRT | 系统是否已完成初始化 |

### 快照方法

```cpp
auto rt = SystemStateReader::snapshotRt();
auto nrt = SystemStateReader::snapshotNrt();

// 快照上直接调用：
rt.headerTimestamp();      // int64_t
rt.headerFrameId();        // int64_t
rt.isSystemRunning();      // bool
rt.systemInfo();            // const std::string&

nrt.isSystemInit();        // bool
```

---

## 2. 控制器（Controller）

### 自由函数

| 函数 | 返回值 | 来源 | 说明 |
|------|--------|------|------|
| `getControllerName()` | `std::string` | RT | 控制器名称 |
| `getControlCycle()` | `double` | RT | 控制周期 |
| `getGlobalCount()` | `int64_t` | RT | 全局计数 |
| `getMasterInfo()` | `std::string` | RT | 主控信息 |
| `isLinkUp()` | `bool` | RT | 与机器人链路是否在线 |
| `getFtValuesCount()` | `size_t` | RT | 六维力传感器数量 |
| `getFtValue(idx)` | `std::vector<double>` | RT | 传感器数据 [fx,fy,fz,mx,my,mz] |

### 快照方法

```cpp
rt.controllerName();        // const std::string&
rt.controlCycle();          // double
rt.globalCount();           // int64_t
rt.masterInfo();            // const std::string&
rt.isLinkUp();              // bool
rt.ftValuesCount();         // size_t
rt.ftValue(idx);            // const std::vector<double>&
// 单个分量：
rt.ftValueFx(idx);          // double
rt.ftValueFy(idx);          // double
rt.ftValueFz(idx);          // double
rt.ftValueMx(idx);          // double
rt.ftValueMy(idx);          // double
rt.ftValueMz(idx);          // double
```

---

## 3. 模型（Model）

### 3.1 模型基本信息

| 自由函数 | 返回值 | 来源 | 说明 |
|----------|--------|------|------|
| `getModelCountRt()` | `size_t` | RT | 模型数量 |
| `getModelCountNrt()` | `size_t` | NRT | 模型数量 |
| `getModelName(m)` | `std::string` | RT/NRT | 模型名称 |
| `getModelType(m)` | `std::string` | RT/NRT | 模型类型 |
| `getJointCount(m)` | `size_t` | RT/NRT | 某模型的关节数量 |

### 3.2 模型 NRT 属性

| 自由函数 | 返回值 | 说明 |
|----------|--------|------|
| `isModelUsingSP(m)` | `bool` | 是否开启奇异点检测 |
| `isModelCollisionDetection(m)` | `bool` | 是否开启碰撞检测 |
| `getModelTakePhoto(m)` | `int` | 启动相机拍照 |

### 快照方法

```cpp
rt.modelCount();                // size_t
rt.modelName(m);                // const std::string&
rt.modelType(m);                // const std::string&
rt.jointCount(m);               // size_t

nrt.isModelUsingSP(m);          // bool
nrt.isModelCollisionDetection(m); // bool
nrt.modelTakePhoto(m);          // int
```

---

## 4. 关节（Joint）

> **参数约定**：`m` = 模型索引（0 起始），`j` = 关节索引（0 起始，相对于该模型）

### 4.1 RT 运行时数据（自由函数）

| 函数 | 返回值 | 说明 |
|------|--------|------|
| `getJointType(m, j)` | `std::string` | 关节类型 |
| `getJointPosition(m, j)` | `double` | 角度 (rad) |
| `getJointTorque(m, j)` | `double` | 力矩 (Nm) |
| `getJointIsEnabled(m, j)` | `bool` | 是否使能 |
| `getJointMode(m, j)` | `int` | 伺服工作模式 |
| `getJointErrorCode(m, j)` | `int` | 伺服错误码 |
| `getJointDigitOutput(m, j)` | `int` | 数字 IO 输出 |
| `getJointDigitInput(m, j)` | `int` | 数字 IO 输入 |
| `getJointSensorTorque(m, j)` | `double` | 关节传感器力矩 (Nm) |
| `getJointVelocity(m, j)` | `double` | 关节速度 |
| `getJointTargetPosition(m, j)` | `double` | 目标角度 (rad) |

### 4.2 NRT 限制数据（自由函数）

| 函数 | 返回值 | 说明 |
|------|--------|------|
| `getJointMaxPosition(m, j)` | `double` | 最大位置限制 |
| `getJointMinPosition(m, j)` | `double` | 最小位置限制 |
| `getJointMaxVel(m, j)` | `double` | 最大速度限制 |
| `getJointMinVel(m, j)` | `double` | 最小速度限制 |
| `getJointMaxAcc(m, j)` | `double` | 最大加速度限制 |
| `getJointMinAcc(m, j)` | `double` | 最小加速度限制 |
| `getJointMaxCollisionTorque(m, j)` | `double` | 碰撞检测力矩阈值 |

### 快照方法

```cpp
// RT
rt.jointType(m, j);             // const std::string&
rt.jointPosition(m, j);         // double
rt.jointTorque(m, j);           // double
rt.jointIsEnabled(m, j);        // bool
rt.jointMode(m, j);             // int
rt.jointErrorCode(m, j);        // int
rt.jointDigitOutput(m, j);      // int
rt.jointDigitInput(m, j);       // int
rt.jointSensorTorque(m, j);     // double
rt.jointVelocity(m, j);         // double
rt.jointTargetPosition(m, j);   // double

// NRT
nrt.jointMaxPosition(m, j);     // double
nrt.jointMinPosition(m, j);     // double
nrt.jointMaxVel(m, j);          // double
nrt.jointMinVel(m, j);          // double
nrt.jointMaxAcc(m, j);          // double
nrt.jointMinAcc(m, j);          // double
nrt.jointMaxCollisionTorque(m, j); // double
```

---

## 5. 当前点（Current Point）

> 数据来源：RT，参数 `m` = 模型索引

| 自由函数 | 返回值 | 说明 |
|----------|--------|------|
| `getCurrentPointName(m)` | `std::string` | 当前点位名称 |
| `getCurrentToolName(m)` | `std::string` | 当前工具名称 |
| `getCurrentWobjName(m)` | `std::string` | 当前工件坐标系名称 |
| `getCurrentRobottarget(m)` | `std::vector<double>` | 笛卡尔目标位姿 |
| `getCurrentJointtarget(m)` | `std::vector<double>` | 关节目标角度 |

快照方法（命名一致，通过 `rt.xxx(m)` 调用）：

```cpp
rt.currentPointName(m);         // const std::string&
rt.currentToolName(m);          // const std::string&
rt.currentWobjName(m);          // const std::string&
rt.currentRobottarget(m);       // const std::vector<double>&
rt.currentJointtarget(m);       // const std::vector<double>&
// 附加（仅快照有）：
rt.hasCurrentPoint(m);          // bool
rt.currentToolData(m);          // const std::vector<double>&
rt.currentWobjData(m);          // const std::vector<double>&
```

---

## 6. 模型运行状态（Model Info）

> 数据来源：RT，参数 `m` = 模型索引

| 自由函数 | 返回值 | 说明 |
|----------|--------|------|
| `getModelErrorCode(m)` | `int` | 错误码 |
| `getModelErrorMsg(m)` | `std::string` | 错误描述 |
| `getModelState(m)` | `int` | 模型状态 |
| `getModelTimeRate(m)` | `double` | 时间比例/速率 |
| `getModelCurrentFuncName(m)` | `std::string` | 当前执行的函数名 |
| `getModelEePe321(m)` | `std::vector<double>` | PE321 末端位姿 |

快照额外字段：

```cpp
rt.modelCurrentFuncInfo(m);     // const std::string&  当前函数附加信息
rt.modelFuncCount(m);           // int                 函数调用计数
rt.modelInfoMsg(m);             // const std::string&  系统提示信息
```

---

## 7. 从站（Slave）

> 数据来源：NRT

| 自由函数 | 返回值 | 说明 |
|----------|--------|------|
| `getSlaveCount()` | `size_t` | 从站数量 |
| `getSlaveName(idx)` | `std::string` | 从站名称 |
| `getSlaveState(idx)` | `int` | 从站状态机 |
| `getSlaveIsOnline(idx)` | `bool` | 是否在线 |

快照额外字段：

```cpp
nrt.slavePhyId(idx);      // int    物理地址
nrt.slaveAlias(idx);      // int    逻辑别名
nrt.slaveIsVirtual(idx);  // bool   是否为虚拟从站
nrt.slaveIsError(idx);    // bool   是否存在错误
```

---

## 8. 子系统（Subsystem）

> 数据来源：NRT

| 自由函数 | 返回值 | 说明 |
|----------|--------|------|
| `getSubsystemCount()` | `size_t` | 子系统数量 |
| `getSubsystemName(idx)` | `std::string` | 子系统名称 |
| `getSubsystemState(idx)` | `int` | 运行状态 |
| `getSubsystemDataSize(idx)` | `size_t` | data 字段的字节大小 |

### 子系统的 data 字段解析

`data` 字段存储的是原始二进制数据（bytes），库不预设数据结构。用户根据协议自定义 POD 结构体，用 `memcpy` 解析。

```cpp
// 1. 定义协议结构体（1字节对齐）
#pragma pack(push, 1)
struct MyStruct {
    int field_a;
    double field_b;
    // ...
};
#pragma pack(pop)

// 2. 方式一：快照上解析
auto nrt = SystemStateReader::snapshotNrt();
if (nrt.valid() && nrt.subsystemDataSize(0) >= sizeof(MyStruct)) {
    MyStruct obj;
    std::memcpy(&obj, nrt.subsystemRawData(0).data(), sizeof(MyStruct));
    // 使用 obj.field_a ...
}

// 3. 方式二：自由函数 + 快照
if (getSubsystemDataSize(0) >= sizeof(MyStruct)) {
    auto nrt_snap = SystemStateReader::snapshotNrt();
    MyStruct obj;
    std::memcpy(&obj, nrt_snap.subsystemRawData(0).data(), sizeof(MyStruct));
}
```

> 参考 `topic_sub_direct.cpp` / `topic_sub_snapshot.cpp` 中的 `TwoFingerGripperStatus` 示例。

---

## 9. 接口（Interface）

> 数据来源：NRT

| 自由函数 | 返回值 | 说明 |
|----------|--------|------|
| `getInterfaceCount()` | `size_t` | 接口数量 |
| `getInterfaceName(idx)` | `std::string` | 接口名称 |
| `getInterfaceState(idx)` | `int` | 接口状态 |

快照额外字段：

```cpp
nrt.interfaceId(idx);    // int    接口编号
```

---

## 10. 工具 / 工件 / 负载 / 示教点 / IO（NRT，仅快照方式）

这些字段推荐通过快照方式访问，自由函数未封装。参数 `m` = 模型索引。

### 工具（Tool）

```cpp
nrt.toolCount(m);                // size_t
nrt.toolName(m, tool_idx);       // const std::string&
nrt.toolData(m, tool_idx);       // const std::vector<double>&
```

### 工件（Wobj）

```cpp
nrt.wobjCount(m);                // size_t
nrt.wobjName(m, wobj_idx);       // const std::string&
nrt.wobjData(m, wobj_idx);       // const std::vector<double>&
```

### 负载（Load）

```cpp
nrt.loadCount(m);                // size_t
nrt.loadName(m, load_idx);       // const std::string&
nrt.loadData(m, load_idx);       // const std::vector<double>&
```

### IO

```cpp
nrt.ioCount(m);                  // size_t
nrt.ioName(m, io_idx);           // const std::string&
nrt.ioData(m, io_idx);           // double
```

### 示教点（Teach Point）

```cpp
nrt.teachPointCount(m);                       // size_t
nrt.teachPointName(m, point_idx);             // const std::string&
nrt.teachPointToolName(m, point_idx);         // const std::string&
nrt.teachPointWobjName(m, point_idx);         // const std::string&
nrt.teachPointToolData(m, point_idx);         // const std::vector<double>&
nrt.teachPointWobjData(m, point_idx);         // const std::vector<double>&
nrt.teachPointRobottarget(m, point_idx);      // const std::vector<double>&
nrt.teachPointJointtarget(m, point_idx);      // const std::vector<double>&
```

---

## 数据来源速查

| 类别 | 来源 | 说明 |
|------|------|------|
| 传感器数据 (position, torque, velocity...) | RT | 高频、运行时 |
| 当前点 (current point) | RT | |
| 模型错误/状态 (error, state, func name) | RT | |
| 六维力 (ftvalues) | RT | |
| 关节限制 (max/min position/vel/acc) | NRT | 低频、配置 |
| 工具/工件/负载/示教点 | NRT | |
| 从站/子系统/接口 | NRT | |
| 系统初始化状态 | NRT | |

---

## 线程安全

所有读取操作均为线程安全。自由函数和快照方法内部采用 `shared_ptr` 快照机制，调用者拿到的是调用时刻的一致性数据副本，后续不受其他线程更新影响。
