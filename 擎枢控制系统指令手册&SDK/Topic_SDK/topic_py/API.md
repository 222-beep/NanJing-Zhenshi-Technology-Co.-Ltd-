# Topic Python API 参考

本文档对应 `system_state_reader.py`，提供快照方式和直接获取方式两套只读 API。

## 默认上报频率

| 数据类型 | 底层默认上报频率 |
|----------|--------------------|
| RT（实时数据） | 250 Hz |
| NRT（非实时数据） | 2 Hz |

以上是底层 Topic 的默认上报频率。实际接收频率还会受到发布端配置、网络和客户端负载影响；Python API 的调用频率不会改变底层上报频率。

## 1. 初始化

```python
from platform_loader import get_topic_module
from system_state_reader import SystemStateReader

topic = get_topic_module()
topic.start_subscriber("192.168.11.11")
```

## 2. 两种访问方式

### 2.1 快照方式

适合一次读取多个字段，同一快照内的数据保持一致。

```python
rt = SystemStateReader.snapshot_rt()
nrt = SystemStateReader.snapshot_nrt()

if rt.valid():
    print(rt.joint_position(0, 0))
```

### 2.2 直接获取方式

模块级 `get_*()` 函数内部自动获取快照，适合读取少量字段。

```python
from system_state_reader import has_rt_data, get_joint_position

if has_rt_data():
    print(get_joint_position(0, 0))
```

参数约定：`m` 为模型索引，`j` 为关节索引，`i` 为当前类别内的元素索引，均从 0 开始。

## 3. 顶层系统信息

| 直接函数 | 快照方法 | 返回值 | 说明 |
|----------|----------|--------|------|
| `has_rt_data()` | `rt.valid()` | `bool` | 是否已有 RT 数据 |
| `has_nrt_data()` | `nrt.valid()` | `bool` | 是否已有 NRT 数据 |
| `get_header_timestamp()` | `header_timestamp()` | `int` | 时间戳 |
| `get_header_frame_id()` | `header_frame_id()` | `int` | 帧 ID |
| `is_system_running()` | `is_system_running()` | `bool` | 系统是否运行 |
| `get_system_info()` | `system_info()` | `str` | 系统信息（RT） |
| `is_system_init()` | `is_system_init()` | `bool` | 系统是否初始化（NRT） |

## 4. 控制器

| 直接函数 | 快照方法 | 返回值 | 说明 |
|----------|----------|--------|------|
| `get_controller_name()` | `controller_name()` | `str` | 控制器名称 |
| `get_control_cycle()` | `control_cycle()` | `float` | 控制周期 |
| `get_global_count()` | `global_count()` | `int` | 全局计数 |
| `get_master_info()` | `master_info()` | `str` | 主控信息 |
| `is_link_up()` | `is_link_up()` | `bool` | 链路状态 |
| `get_ftvalues_count()` | `ftvalues_count()` | `int` | 六维力传感器数量 |
| `get_ftvalue(i)` | `ftvalue(i)` | `list[float]` | `[fx, fy, fz, mx, my, mz]` |

快照还提供 `ftvalue_fx/fy/fz/mx/my/mz(i)` 分量访问方法。

## 5. 模型与关节

| 直接函数 | 快照方法 | 返回值 | 说明 |
|----------|----------|--------|------|
| `get_model_count_rt()` / `get_model_count_nrt()` | `model_count()` | `int` | 模型数量 |
| `get_model_name(m)` | `model_name(m)` | `str` | 模型名称 |
| `get_model_type(m)` | `model_type(m)` | `str` | 模型类型 |
| `get_joint_count(m)` | `joint_count(m)` | `int` | 关节数量 |
| `is_model_using_sp(m)` | `is_model_using_sp(m)` | `bool` | 是否启用奇异点检测（NRT） |
| `is_model_collision_detection(m)` | `is_model_collision_detection(m)` | `bool` | 是否启用碰撞检测（NRT） |
| `get_model_take_photo(m)` | `model_take_photo(m)` | `int` | 相机拍照状态（NRT） |

### 5.1 关节 RT 数据

以下直接函数均有去掉 `get_` 前缀的同名快照方法：

| 函数 | 返回值 | 说明 |
|------|--------|------|
| `get_joint_type(m, j)` | `str` | 关节类型 |
| `get_joint_position(m, j)` | `float` | 当前位置 |
| `get_joint_torque(m, j)` | `float` | 当前力矩 |
| `get_joint_is_enabled(m, j)` | `bool` | 是否使能 |
| `get_joint_mode(m, j)` | `int` | 控制模式 |
| `get_joint_error_code(m, j)` | `int` | 错误码 |
| `get_joint_digit_output(m, j)` | `int` | 数字输出 |
| `get_joint_digit_input(m, j)` | `int` | 数字输入 |
| `get_joint_sensor_torque(m, j)` | `float` | 传感器力矩 |
| `get_joint_velocity(m, j)` | `float` | 速度 |
| `get_joint_target_position(m, j)` | `float` | 目标位置 |

客户设备还提供模块级便捷函数 `get_model_gripper_state()`。它从同一个 RT 快照
读取 `model1/joint0`，并返回米、毫米和 Topic 时间戳。该函数是对通用关节 RT
数据的派生包装，不是新的底层 Topic 字段，也不同于 NRT 子系统中的外设夹爪接口。

```python
from system_state_reader import get_model_gripper_state

model_gripper = get_model_gripper_state()
if model_gripper is not None:
    print(model_gripper["position_m"])
    print(model_gripper["position_mm"])
```

### 5.2 关节 NRT 限制

| 函数 | 返回值 | 说明 |
|------|--------|------|
| `get_joint_max_position(m, j)` / `get_joint_min_position(m, j)` | `float` | 位置上下限 |
| `get_joint_max_vel(m, j)` / `get_joint_min_vel(m, j)` | `float` | 速度上下限 |
| `get_joint_max_acc(m, j)` / `get_joint_min_acc(m, j)` | `float` | 加速度上下限 |
| `get_joint_max_collision_torque(m, j)` | `float` | 碰撞力矩阈值 |

快照方法为对应函数去掉 `get_` 前缀后的名称。

## 6. MatrixVariable（NRT）

| 直接函数 | 快照方法 | 返回值 | 说明 |
|----------|----------|--------|------|
| `get_matrix_variable_count(m)` | `matrix_variable_count(m)` | `int` | 变量数量 |
| `get_matrix_variable_name(m, i)` | `matrix_variable_name(m, i)` | `str` | 变量名称 |
| `get_matrix_variable_data(m, i)` | `matrix_variable_data(m, i)` | `list[float]` | 变量数组数据 |
| — | `matrix_variable_data_by_name(m, name)` | `list[float]` | 按名称读取；不存在时抛出 `KeyError` |

## 7. 拖动系数与干涉区（NRT）

| 直接函数 | 快照方法 | 返回值 | 说明 |
|----------|----------|--------|------|
| `get_drag_in_cst_coef(m)` | `drag_in_cst_coef(m)` | `list[float]` | 电流环拖动系数 |
| `get_inf_rng_count(m)` | `inf_rng_count(m)` | `int` | 区域数量 |
| `get_inf_rng(m, i)` | `inf_rng(m, i)` | `InfRngInfo` | 完整区域配置 |

`InfRngInfo` 字段：

| 字段 | 类型 | 说明 |
|------|------|------|
| `infrng_name` | `str` | 区域名称 |
| `infrng_action` | `str` | stop/pause |
| `infrng_isenable` | `int` | 是否启用 |
| `infrng_priority` | `int` | 优先级 |
| `infrng_zonetype` | `str` | interfere/safety |
| `infrng_diname` / `infrng_doname` | `str` | DI/DO 名称 |
| `infrng_shape` | `str` | box/sphere/cylinder |
| `infrng_center` | `list[float]` | 中心坐标 |
| `infrng_size` | `list[float]` | 尺寸参数 |
| `infrng_euler` | `list[float]` | 欧拉角 |
| `infrng_margin` | `float` | 安全边距 |
| `infrng_is_twopoint` | `bool` | 是否使用双点定义 |
| `infrng_point1` / `infrng_point2` | `list[float]` | 双点坐标 |

## 8. 当前点与模型运行状态（RT）

直接函数包括：

- `get_current_point_name(m)`、`get_current_tool_name(m)`、`get_current_wobj_name(m)`
- `get_current_robottarget(m)`、`get_current_jointtarget(m)`
- `get_model_error_code(m)`、`get_model_error_msg(m)`、`get_model_state(m)`
- `get_model_time_rate(m)`、`get_model_current_func_name(m)`、`get_model_ee_pe321(m)`

快照还提供 `has_current_point()`、`current_tool_data()`、`current_wobj_data()`、
`model_current_func_info()`、`model_func_count()` 和 `model_info_msg()`。

## 9. 工具、工件、负载、IO 和示教点（NRT 快照）

这些批量配置推荐使用快照方式：

- 工具：`tool_count/name/data(m, i)`
- 工件：`wobj_count/name/data(m, i)`
- 负载：`load_count/name/data(m, i)`
- IO：`io_count/name/data(m, i)`、`io_total_count()`
- 示教点：`teach_point_count/name/tool_name/wobj_name/tool_data/wobj_data/robottarget/jointtarget(m, i)`

## 10. 从站、子系统和接口（NRT）

| 类别 | 直接函数 | 快照方法 |
|------|----------|----------|
| 从站 | `get_slave_count/name/state/is_online` | `slave_count/name/phy_id/alias/state/is_online/is_virtual/is_error` |
| 子系统 | `get_subsystem_count/name/state/data_size`、`parse_subsystem_data` | `subsystem_count/name/id/state/raw_data/data_size`、`parse_subsystem_data` |
| 接口 | `get_interface_count/name/state` | `interface_count/name/id/state` |

## 11. 完整示例

- 直接获取方式：[`examples/topic_sub_direct.py`](examples/topic_sub_direct.py)
- 快照方式：[`examples/topic_sub_snapshot.py`](examples/topic_sub_snapshot.py)

## 12. model1 model 夹爪便捷接口

`system_state_reader.py` 提供面向客户设备的 model 夹爪状态便捷接口。它只读取
RT Topic 中的 `model1/joint0`，不包含机械臂关节角或末端位姿。`main.py` 已包含
调用示例。

```python
from system_state_reader import get_model_gripper_state

state = get_model_gripper_state()
if state is not None:
    print(state["position_m"])
    print(state["position_mm"])
    print(state["topic_timestamp"])
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `position_m` | `float` | model1 第 0 关节位置，单位 m |
| `position_mm` | `float` | 同一位置换算为 mm |
| `topic_timestamp` | `int` | Topic 消息头原始时间戳，单位由发布端定义 |

订阅尚未收到 RT 数据时返回 `None`；缺少 model1 或其第 0 关节时抛出
`RuntimeError`，用于尽早发现发布端模型配置不匹配。
