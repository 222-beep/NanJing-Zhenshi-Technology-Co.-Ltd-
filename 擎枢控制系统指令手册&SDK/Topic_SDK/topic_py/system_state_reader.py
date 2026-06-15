# system_state_reader.py
"""
SystemStateReader —— Python 版本的便捷只读访问层，对应 C++ 的 system_state_reader.hpp

提供两种访问方式：

1. 快照方式 (Snapshot Mode) —— 高效批量读取
   >>> reader = SystemStateReader.snapshot_rt()
   >>> if reader.valid():
   >>>     pos = reader.joint_position(0, 2)   # 模型0, 关节2 的位置
   >>>     for m in range(reader.model_count()):
   >>>         for j in range(reader.joint_count(m)):
   >>>             print(reader.joint_position(m, j))

2. 直接获取方式 (Direct Mode) —— 模块级便捷函数
   每个函数内部自动获取快照，适合快速单字段访问
   >>> pos = get_joint_position(0, 2)          # 模型0, 关节2 的位置
   >>> if has_rt_data(): ...
"""
import struct
import sys
import ctypes
from platform_loader import get_topic_module

_topic = get_topic_module()

# ============================================================================
# ctypes 辅助：当 pybind11 无法将 std::string 解码为 UTF-8 str 时，
# 通过直接读取 C++ 对象内存来获取原始字节数据。
# ============================================================================

_ctypes_available = False
_ctypes_read_mem = None
_ctypes_get_cpp_ptr = None

def _init_ctypes_fallback():
    """初始化 ctypes fallback 机制（平台相关）"""
    global _ctypes_available, _ctypes_read_mem, _ctypes_get_cpp_ptr

    if _ctypes_available:
        return

    try:
        if sys.platform == 'win32':
            import ctypes.wintypes

            kernel32 = ctypes.windll.kernel32
            _ReadProcessMemory = kernel32.ReadProcessMemory
            _ReadProcessMemory.argtypes = [
                ctypes.wintypes.HANDLE, ctypes.wintypes.LPCVOID,
                ctypes.wintypes.LPVOID, ctypes.c_size_t,
                ctypes.POINTER(ctypes.c_size_t),
            ]
            _ReadProcessMemory.restype = ctypes.wintypes.BOOL
            _handle = kernel32.GetCurrentProcess()

            def _read_mem_win32(addr, size):
                buf = ctypes.create_string_buffer(size)
                bytes_read = ctypes.c_size_t(0)
                if _ReadProcessMemory(_handle, ctypes.c_void_p(addr), buf, size, ctypes.byref(bytes_read)):
                    return bytes(buf.raw[:bytes_read.value])
                return None

            _ctypes_read_mem = _read_mem_win32

        elif sys.platform == 'linux':
            import os

            # Linux: 通过 /proc/self/mem 读取进程内存
            # 需要先通过 ptrace 或使用 process_vm_readv
            # 尝试使用 ctypes 调用 process_vm_readv
            libc = ctypes.CDLL('libc.so.6', use_errno=True)

            class IOVec(ctypes.Structure):
                _fields_ = [
                    ('iov_base', ctypes.c_void_p),
                    ('iov_len', ctypes.c_size_t),
                ]

            _process_vm_readv = libc.process_vm_readv
            _process_vm_readv.argtypes = [
                ctypes.c_uint,            # pid (0 = self)
                ctypes.POINTER(IOVec),    # local iov
                ctypes.c_ulong,           # local iov count
                ctypes.POINTER(IOVec),    # remote iov
                ctypes.c_ulong,           # remote iov count
                ctypes.c_ulong,           # flags
            ]
            _process_vm_readv.restype = ctypes.c_ssize_t

            def _read_mem_linux(addr, size):
                buf = ctypes.create_string_buffer(size)
                local_iov = IOVec(ctypes.cast(buf, ctypes.c_void_p), size)
                remote_iov = IOVec(ctypes.c_void_p(addr), size)
                nread = _process_vm_readv(
                    0,  # self
                    ctypes.byref(local_iov), 1,
                    ctypes.byref(remote_iov), 1,
                    0,
                )
                if nread > 0:
                    return bytes(buf.raw[:nread])
                return None

            _ctypes_read_mem = _read_mem_linux

        else:
            _ctypes_available = False
            return

        # ---- 提取 pybind11 对象中的 C++ 指针 ----
        # pybind11 对象在 Python 对象头部 (PyObject_HEAD, 16 bytes) 之后
        # 存储 C++ 对象的指针或值。
        # 典型布局（64位）:
        #   offset  0: ob_refcnt (8 bytes)
        #   offset  8: ob_type   (8 bytes)
        #   offset 16: value_ptr / holder (8 bytes) ← C++ 对象指针
        CPP_PTR_OFFSET = 0x10  # pybind11 对象中 C++ 指针的偏移

        def _get_cpp_ptr(obj):
            """从 pybind11 Python 对象中获取底层 C++ 对象指针"""
            obj_addr = id(obj)
            data = _ctypes_read_mem(obj_addr, 24)
            if not data or len(data) < (CPP_PTR_OFFSET + 8):
                return None
            return struct.unpack('<Q', data[CPP_PTR_OFFSET:CPP_PTR_OFFSET + 8])[0]

        _ctypes_get_cpp_ptr = _get_cpp_ptr
        _ctypes_available = True

    except Exception:
        _ctypes_available = False


def _subsystem_raw_data_ctypes(sys_data_obj, idx):
    """
    通过 ctypes 直接读取 C++ SubsystemInfo::data 字段的原始字节。
    当 pybind11 无法将 std::string 解码为 UTF-8 str 时使用此 fallback。

    SubsystemInfo 内存布局 (MSVC x64):
      offset  0: std::string subsystem_name (32 bytes)
      offset 32: int id (4 bytes)
      offset 36: int state (4 bytes)
      offset 40: std::string data (32 bytes)

    MSVC std::string 布局 (32 bytes):
      offset  0: union { char _Buf[16];  (SSO)
                         char* _Ptr; }    (heap: capacity >= 16)
      offset 16: size   (8 bytes)
      offset 24: capacity (8 bytes)
    """
    _init_ctypes_fallback()
    if not _ctypes_available:
        return None

    cpp_addr = _ctypes_get_cpp_ptr(sys_data_obj)
    if not cpp_addr:
        return None

    # data 字段在 SubsystemInfo 中的偏移
    DATA_OFFSET = 0x28  # 40 bytes

    str_data = _ctypes_read_mem(cpp_addr + DATA_OFFSET, 32)
    if not str_data or len(str_data) < 32:
        return None

    size = struct.unpack('<Q', str_data[16:24])[0]
    capacity = struct.unpack('<Q', str_data[24:32])[0]

    if size == 0:
        return b''

    # 安全上限：防止读取异常大的 size（最大 1MB）
    if size > 1024 * 1024:
        return None

    if capacity >= 16:
        # 堆分配模式：从指针读取数据
        ptr = struct.unpack('<Q', str_data[0:8])[0]
        if ptr == 0 or ptr < 0x10000:
            return None
        return _ctypes_read_mem(ptr, size) or None
    else:
        # SSO 模式：数据内联存储在 string 对象的前 16 字节中
        return bytes(str_data[:size])


class SystemStateReader:
    """快照方式的系统状态只读访问器，对应 C++ 的 SystemStateReader"""

    def __init__(self, data: 'topic.SystemStateData', is_rt: bool = True):
        """
        :param data: SystemStateData 快照数据
        :param is_rt: True 表示 RT 数据快照，False 表示 NRT 数据快照
        """
        self._data = data
        self._is_rt = is_rt

    @staticmethod
    def snapshot_rt() -> 'SystemStateReader':
        """获取 RT 数据快照"""
        state = _topic.get_system_state()
        if state.has_rt():
            return SystemStateReader(state.get_rt(), is_rt=True)
        return SystemStateReader(None, is_rt=True)

    @staticmethod
    def snapshot_nrt() -> 'SystemStateReader':
        """获取 NRT 数据快照"""
        state = _topic.get_system_state()
        if state.has_nrt():
            return SystemStateReader(state.get_nrt(), is_rt=False)
        return SystemStateReader(None, is_rt=False)

    def valid(self) -> bool:
        """快照是否有效"""
        return self._data is not None

    def __bool__(self):
        return self.valid()

    @property
    def raw(self):
        """直接访问底层 SystemStateData 对象"""
        return self._data

    # ========================================================================
    # 顶层字段
    # ========================================================================
    def header_timestamp(self) -> int:
        return self._data.header_timestamp

    def header_frame_id(self) -> int:
        return self._data.header_frame_id

    def is_system_running(self) -> bool:
        return self._data.system_running_state

    def system_info(self) -> str:
        return self._data.system_info

    def is_system_init(self) -> bool:
        return self._data.system_is_init

    # ========================================================================
    # 控制器
    # ========================================================================
    def controller_name(self) -> str:
        return self._data.controller.controller_name

    def control_cycle(self) -> float:
        return self._data.controller.control_cycle

    def global_count(self) -> int:
        return self._data.controller.global_count

    def master_info(self) -> str:
        return self._data.controller.master_info

    def is_link_up(self) -> bool:
        return self._data.controller.is_link_up

    def ftvalues_count(self) -> int:
        return len(self._data.controller.ftvalues)

    def ftvalue(self, sensor_idx: int) -> list:
        return self._data.controller.ftvalues[sensor_idx]

    def ftvalue_fx(self, sensor_idx: int) -> float:
        return self._data.controller.ftvalues[sensor_idx][0]

    def ftvalue_fy(self, sensor_idx: int) -> float:
        return self._data.controller.ftvalues[sensor_idx][1]

    def ftvalue_fz(self, sensor_idx: int) -> float:
        return self._data.controller.ftvalues[sensor_idx][2]

    def ftvalue_mx(self, sensor_idx: int) -> float:
        return self._data.controller.ftvalues[sensor_idx][3]

    def ftvalue_my(self, sensor_idx: int) -> float:
        return self._data.controller.ftvalues[sensor_idx][4]

    def ftvalue_mz(self, sensor_idx: int) -> float:
        return self._data.controller.ftvalues[sensor_idx][5]

    # ========================================================================
    # 模型
    # ========================================================================
    def model_count(self) -> int:
        return len(self._data.models)

    def model(self, model_idx: int):
        """返回 ModelsInfo"""
        return self._data.models[model_idx]

    def model_name(self, model_idx: int) -> str:
        return self._data.models[model_idx].model_name

    def model_type(self, model_idx: int) -> str:
        return self._data.models[model_idx].model_type

    def is_model_using_sp(self, model_idx: int) -> bool:
        return self._data.models[model_idx].is_using_sp

    def is_model_collision_detection(self, model_idx: int) -> bool:
        return self._data.models[model_idx].is_collision_detection

    def model_take_photo(self, model_idx: int) -> int:
        return self._data.models[model_idx].take_photo

    # ========================================================================
    # 关节 —— 通过模型号 + 关节号访问
    # ========================================================================
    def _joint(self, model_idx: int, joint_idx: int):
        """内部方法：获取某个模型下某个关节的 JointInfo"""
        model = self._data.models[model_idx]
        return self._data.models_joints[model.joint_start_idx + joint_idx]

    def joint_count(self, model_idx: int) -> int:
        return self._data.models[model_idx].joint_count

    def joint_type(self, model_idx: int, joint_idx: int) -> str:
        return self._joint(model_idx, joint_idx).joint_type

    def joint_position(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).position

    def joint_torque(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).torque

    def joint_is_enabled(self, model_idx: int, joint_idx: int) -> bool:
        return self._joint(model_idx, joint_idx).is_enabled

    def joint_mode(self, model_idx: int, joint_idx: int) -> int:
        return self._joint(model_idx, joint_idx).mode

    def joint_error_code(self, model_idx: int, joint_idx: int) -> int:
        return self._joint(model_idx, joint_idx).error_code

    def joint_digit_output(self, model_idx: int, joint_idx: int) -> int:
        return self._joint(model_idx, joint_idx).digit_output

    def joint_digit_input(self, model_idx: int, joint_idx: int) -> int:
        return self._joint(model_idx, joint_idx).digit_input

    def joint_sensor_torque(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).sensor_torque

    def joint_velocity(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).velocity

    def joint_target_position(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).target_position

    # ---- 关节 NRT 限制值 ----
    def joint_max_position(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).max_position

    def joint_min_position(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).min_position

    def joint_max_vel(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).max_vel

    def joint_min_vel(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).min_vel

    def joint_max_acc(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).max_acc

    def joint_min_acc(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).min_acc

    def joint_max_collision_torque(self, model_idx: int, joint_idx: int) -> float:
        return self._joint(model_idx, joint_idx).max_collision_torque

    # ========================================================================
    # 当前点 (RT)
    # ========================================================================
    def has_current_point(self, model_idx: int) -> bool:
        return model_idx < len(self._data.models_current_points)

    def current_point_name(self, model_idx: int) -> str:
        return self._data.models_current_points[model_idx].point_name

    def current_tool_name(self, model_idx: int) -> str:
        return self._data.models_current_points[model_idx].tool_name

    def current_wobj_name(self, model_idx: int) -> str:
        return self._data.models_current_points[model_idx].wobj_name

    def current_tool_data(self, model_idx: int) -> list:
        return self._data.models_current_points[model_idx].tool_data

    def current_wobj_data(self, model_idx: int) -> list:
        return self._data.models_current_points[model_idx].wobj_data

    def current_robottarget(self, model_idx: int) -> list:
        return self._data.models_current_points[model_idx].robottarget

    def current_jointtarget(self, model_idx: int) -> list:
        return self._data.models_current_points[model_idx].jointtarget

    # ========================================================================
    # 模型运行状态 (RT)
    # ========================================================================
    def model_error_code(self, model_idx: int) -> int:
        return self._data.models_info[model_idx].error_code

    def model_error_msg(self, model_idx: int) -> str:
        return self._data.models_info[model_idx].error_msg

    def model_state(self, model_idx: int) -> int:
        return self._data.models_info[model_idx].model_state

    def model_time_rate(self, model_idx: int) -> float:
        return self._data.models_info[model_idx].model_time_rate

    def model_current_func_name(self, model_idx: int) -> str:
        return self._data.models_info[model_idx].current_func_name

    def model_current_func_info(self, model_idx: int) -> str:
        return self._data.models_info[model_idx].current_func_info

    def model_func_count(self, model_idx: int) -> int:
        return self._data.models_info[model_idx].func_count

    def model_info_msg(self, model_idx: int) -> str:
        return self._data.models_info[model_idx].info_msg

    def model_ee_pe321(self, model_idx: int) -> list:
        return self._data.models_info[model_idx].ee_pe321

    # ========================================================================
    # 工具 (NRT, 按模型索引)
    # ========================================================================
    def tool_count(self, model_idx: int) -> int:
        return self._data.models[model_idx].tools_count

    def tool_name(self, model_idx: int, tool_idx: int) -> str:
        model = self._data.models[model_idx]
        return self._data.models_tools[model.tools_start_idx + tool_idx].tool_name

    def tool_data(self, model_idx: int, tool_idx: int) -> list:
        model = self._data.models[model_idx]
        return self._data.models_tools[model.tools_start_idx + tool_idx].data

    # ========================================================================
    # 工件 (NRT, 按模型索引)
    # ========================================================================
    def wobj_count(self, model_idx: int) -> int:
        return self._data.models[model_idx].wobjs_count

    def wobj_name(self, model_idx: int, wobj_idx: int) -> str:
        model = self._data.models[model_idx]
        return self._data.models_wobjs[model.wobjs_start_idx + wobj_idx].wobj_name

    def wobj_data(self, model_idx: int, wobj_idx: int) -> list:
        model = self._data.models[model_idx]
        return self._data.models_wobjs[model.wobjs_start_idx + wobj_idx].data

    # ========================================================================
    # 负载 (NRT, 按模型索引)
    # ========================================================================
    def load_count(self, model_idx: int) -> int:
        return self._data.models[model_idx].loads_count

    def load_name(self, model_idx: int, load_idx: int) -> str:
        model = self._data.models[model_idx]
        return self._data.models_loads[model.loads_start_idx + load_idx].load_name

    def load_data(self, model_idx: int, load_idx: int) -> list:
        model = self._data.models[model_idx]
        return self._data.models_loads[model.loads_start_idx + load_idx].data

    # ========================================================================
    # IO (NRT, 按模型索引)
    # ========================================================================
    def io_count(self, model_idx: int) -> int:
        return self._data.models[model_idx].io_count

    def io_name(self, model_idx: int, io_idx: int) -> str:
        model = self._data.models[model_idx]
        return self._data.models_io[model.io_start_idx + io_idx].io_name

    def io_data(self, model_idx: int, io_idx: int) -> float:
        model = self._data.models[model_idx]
        return self._data.models_io[model.io_start_idx + io_idx].io_data

    def io_total_count(self) -> int:
        return len(self._data.models_io)

    # ========================================================================
    # 示教点 (NRT, 按模型索引)
    # ========================================================================
    def teach_point_count(self, model_idx: int) -> int:
        return self._data.models[model_idx].teach_points_count

    def teach_point_name(self, model_idx: int, point_idx: int) -> str:
        model = self._data.models[model_idx]
        return self._data.models_teach_points[model.teach_points_start_idx + point_idx].point_name

    def teach_point_tool_name(self, model_idx: int, point_idx: int) -> str:
        model = self._data.models[model_idx]
        return self._data.models_teach_points[model.teach_points_start_idx + point_idx].tool_name

    def teach_point_wobj_name(self, model_idx: int, point_idx: int) -> str:
        model = self._data.models[model_idx]
        return self._data.models_teach_points[model.teach_points_start_idx + point_idx].wobj_name

    def teach_point_tool_data(self, model_idx: int, point_idx: int) -> list:
        model = self._data.models[model_idx]
        return self._data.models_teach_points[model.teach_points_start_idx + point_idx].tool_data

    def teach_point_wobj_data(self, model_idx: int, point_idx: int) -> list:
        model = self._data.models[model_idx]
        return self._data.models_teach_points[model.teach_points_start_idx + point_idx].wobj_data

    def teach_point_robottarget(self, model_idx: int, point_idx: int) -> list:
        model = self._data.models[model_idx]
        return self._data.models_teach_points[model.teach_points_start_idx + point_idx].robottarget

    def teach_point_jointtarget(self, model_idx: int, point_idx: int) -> list:
        model = self._data.models[model_idx]
        return self._data.models_teach_points[model.teach_points_start_idx + point_idx].jointtarget

    # ========================================================================
    # 从站 (NRT)
    # ========================================================================
    def slave_count(self) -> int:
        return len(self._data.slaves)

    def slave_name(self, idx: int) -> str:
        return self._data.slaves[idx].slave_name

    def slave_phy_id(self, idx: int) -> int:
        return self._data.slaves[idx].phy_id

    def slave_alias(self, idx: int) -> int:
        return self._data.slaves[idx].alias

    def slave_state(self, idx: int) -> int:
        return self._data.slaves[idx].slave_state

    def slave_is_online(self, idx: int) -> bool:
        return self._data.slaves[idx].is_online

    def slave_is_virtual(self, idx: int) -> bool:
        return self._data.slaves[idx].is_virtual

    def slave_is_error(self, idx: int) -> bool:
        return self._data.slaves[idx].is_error

    # ========================================================================
    # 子系统 (NRT)
    # ========================================================================
    def subsystem_count(self) -> int:
        return len(self._data.subsystems)

    def subsystem_name(self, idx: int) -> str:
        return self._data.subsystems[idx].subsystem_name

    def subsystem_id(self, idx: int) -> int:
        return self._data.subsystems[idx].id

    def subsystem_state(self, idx: int) -> int:
        return self._data.subsystems[idx].state

    def subsystem_raw_data(self, idx: int) -> bytes:
        """返回子系统原始 data 字段（bytes），用户可以用 struct.unpack 自行解析"""
        sub = self._data.subsystems[idx]
        try:
            s = sub.data
            if isinstance(s, bytes):
                return s
            # C++ std::string 可能会被 pybind11 转换为 str，需要编码回 bytes
            if isinstance(s, str):
                return s.encode('latin-1')
            return bytes(s)
        except UnicodeDecodeError:
            # pybind11 无法将 std::string（二进制数据）解码为 UTF-8 str
            # 通过 ctypes 直接读取 C++ 对象内存获取原始字节
            raw = _subsystem_raw_data_ctypes(sub, idx)
            if raw is not None:
                return raw
            # ctypes fallback 也失败，抛出更具描述性的错误
            raise RuntimeError(
                f"Cannot access subsystem[{idx}].data: binary data contains "
                f"non-UTF-8 bytes and ctypes fallback also failed. "
                f"Consider recompiling the topic module with py::bytes return type."
            )

    def subsystem_data_size(self, idx: int) -> int:
        return len(self.subsystem_raw_data(idx))

    def parse_subsystem_data(self, idx: int, struct_format: str):
        """
        使用 struct 格式解析子系统 data 字段

        :param idx: 子系统索引
        :param struct_format: struct.unpack 格式字符串，如 '<6i' 表示 6 个 int
        :return: tuple of unpacked values
        """
        raw = self.subsystem_raw_data(idx)
        return struct.unpack(struct_format, raw[:struct.calcsize(struct_format)])

    # ========================================================================
    # 接口 (NRT)
    # ========================================================================
    def interface_count(self) -> int:
        return len(self._data.interfaces)

    def interface_name(self, idx: int) -> str:
        return self._data.interfaces[idx].interface_name

    def interface_id(self, idx: int) -> int:
        return self._data.interfaces[idx].id

    def interface_state(self, idx: int) -> int:
        return self._data.interfaces[idx].state


# ============================================================================
# 直接获取方式 (Direct Mode) — Python 层面的便捷函数
# 每个函数内部自动获取快照，适合快速单字段访问
# 这些函数对应 C++ system_state_reader.hpp 中的 free functions
# ============================================================================

def has_rt_data() -> bool:
    return _topic.get_system_state().has_rt()


def has_nrt_data() -> bool:
    return _topic.get_system_state().has_nrt()


# ---- 顶层 ----
def get_header_timestamp() -> int:
    s = SystemStateReader.snapshot_rt()
    return s.header_timestamp() if s else 0


def get_header_frame_id() -> int:
    s = SystemStateReader.snapshot_rt()
    return s.header_frame_id() if s else 0


def is_system_running() -> bool:
    s = SystemStateReader.snapshot_rt()
    return s.is_system_running() if s else False


def get_system_info() -> str:
    s = SystemStateReader.snapshot_rt()
    return s.system_info() if s else ""


def is_system_init() -> bool:
    s = SystemStateReader.snapshot_nrt()
    return s.is_system_init() if s else False


# ---- 控制器 ----
def get_controller_name() -> str:
    s = SystemStateReader.snapshot_rt()
    return s.controller_name() if s else ""


def get_control_cycle() -> float:
    s = SystemStateReader.snapshot_rt()
    return s.control_cycle() if s else 0.0


def get_global_count() -> int:
    s = SystemStateReader.snapshot_rt()
    return s.global_count() if s else 0


def get_master_info() -> str:
    s = SystemStateReader.snapshot_rt()
    return s.master_info() if s else ""


def is_link_up() -> bool:
    s = SystemStateReader.snapshot_rt()
    return s.is_link_up() if s else False


def get_ftvalues_count() -> int:
    s = SystemStateReader.snapshot_rt()
    return s.ftvalues_count() if s else 0


def get_ftvalue(sensor_idx: int) -> list:
    s = SystemStateReader.snapshot_rt()
    return s.ftvalue(sensor_idx) if s else []


# ---- 模型 ----
def get_model_count_rt() -> int:
    s = SystemStateReader.snapshot_rt()
    return s.model_count() if s else 0


def get_model_count_nrt() -> int:
    s = SystemStateReader.snapshot_nrt()
    return s.model_count() if s else 0


def get_joint_count(model_idx: int) -> int:
    s = SystemStateReader.snapshot_rt()
    if not s:
        s = SystemStateReader.snapshot_nrt()
    return s.joint_count(model_idx) if s else 0


def get_model_name(model_idx: int) -> str:
    s = SystemStateReader.snapshot_rt()
    if not s:
        s = SystemStateReader.snapshot_nrt()
    return s.model_name(model_idx) if s else ""


def get_model_type(model_idx: int) -> str:
    s = SystemStateReader.snapshot_rt()
    if not s:
        s = SystemStateReader.snapshot_nrt()
    return s.model_type(model_idx) if s else ""


# ---- 关节 RT 值 (model_idx, joint_idx) ----
def get_joint_type(model_idx: int, joint_idx: int) -> str:
    s = SystemStateReader.snapshot_rt()
    return s.joint_type(model_idx, joint_idx) if s else ""


def get_joint_position(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_rt()
    return s.joint_position(model_idx, joint_idx) if s else 0.0


def get_joint_torque(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_rt()
    return s.joint_torque(model_idx, joint_idx) if s else 0.0


def get_joint_is_enabled(model_idx: int, joint_idx: int) -> bool:
    s = SystemStateReader.snapshot_rt()
    return s.joint_is_enabled(model_idx, joint_idx) if s else False


def get_joint_mode(model_idx: int, joint_idx: int) -> int:
    s = SystemStateReader.snapshot_rt()
    return s.joint_mode(model_idx, joint_idx) if s else 0


def get_joint_error_code(model_idx: int, joint_idx: int) -> int:
    s = SystemStateReader.snapshot_rt()
    return s.joint_error_code(model_idx, joint_idx) if s else 0


def get_joint_digit_output(model_idx: int, joint_idx: int) -> int:
    s = SystemStateReader.snapshot_rt()
    return s.joint_digit_output(model_idx, joint_idx) if s else 0


def get_joint_digit_input(model_idx: int, joint_idx: int) -> int:
    s = SystemStateReader.snapshot_rt()
    return s.joint_digit_input(model_idx, joint_idx) if s else 0


def get_joint_sensor_torque(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_rt()
    return s.joint_sensor_torque(model_idx, joint_idx) if s else 0.0


def get_joint_velocity(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_rt()
    return s.joint_velocity(model_idx, joint_idx) if s else 0.0


def get_joint_target_position(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_rt()
    return s.joint_target_position(model_idx, joint_idx) if s else 0.0


# ---- 关节 NRT 限制值 (model_idx, joint_idx) ----
def get_joint_max_position(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_nrt()
    return s.joint_max_position(model_idx, joint_idx) if s else 0.0


def get_joint_min_position(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_nrt()
    return s.joint_min_position(model_idx, joint_idx) if s else 0.0


def get_joint_max_vel(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_nrt()
    return s.joint_max_vel(model_idx, joint_idx) if s else 0.0


def get_joint_min_vel(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_nrt()
    return s.joint_min_vel(model_idx, joint_idx) if s else 0.0


def get_joint_max_acc(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_nrt()
    return s.joint_max_acc(model_idx, joint_idx) if s else 0.0


def get_joint_min_acc(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_nrt()
    return s.joint_min_acc(model_idx, joint_idx) if s else 0.0


def get_joint_max_collision_torque(model_idx: int, joint_idx: int) -> float:
    s = SystemStateReader.snapshot_nrt()
    return s.joint_max_collision_torque(model_idx, joint_idx) if s else 0.0


# ---- 模型 NRT 属性 ----
def is_model_using_sp(model_idx: int) -> bool:
    s = SystemStateReader.snapshot_nrt()
    return s.is_model_using_sp(model_idx) if s else False


def is_model_collision_detection(model_idx: int) -> bool:
    s = SystemStateReader.snapshot_nrt()
    return s.is_model_collision_detection(model_idx) if s else False


def get_model_take_photo(model_idx: int) -> int:
    s = SystemStateReader.snapshot_nrt()
    return s.model_take_photo(model_idx) if s else 0


# ---- 当前点 (RT) ----
def get_current_point_name(model_idx: int) -> str:
    s = SystemStateReader.snapshot_rt()
    return s.current_point_name(model_idx) if s else ""


def get_current_tool_name(model_idx: int) -> str:
    s = SystemStateReader.snapshot_rt()
    return s.current_tool_name(model_idx) if s else ""


def get_current_wobj_name(model_idx: int) -> str:
    s = SystemStateReader.snapshot_rt()
    return s.current_wobj_name(model_idx) if s else ""


def get_current_robottarget(model_idx: int) -> list:
    s = SystemStateReader.snapshot_rt()
    return s.current_robottarget(model_idx) if s else []


def get_current_jointtarget(model_idx: int) -> list:
    s = SystemStateReader.snapshot_rt()
    return s.current_jointtarget(model_idx) if s else []


# ---- 模型运行状态 (RT) ----
def get_model_error_code(model_idx: int) -> int:
    s = SystemStateReader.snapshot_rt()
    return s.model_error_code(model_idx) if s else 0


def get_model_error_msg(model_idx: int) -> str:
    s = SystemStateReader.snapshot_rt()
    return s.model_error_msg(model_idx) if s else ""


def get_model_state(model_idx: int) -> int:
    s = SystemStateReader.snapshot_rt()
    return s.model_state(model_idx) if s else 0


def get_model_time_rate(model_idx: int) -> float:
    s = SystemStateReader.snapshot_rt()
    return s.model_time_rate(model_idx) if s else 0.0


def get_model_current_func_name(model_idx: int) -> str:
    s = SystemStateReader.snapshot_rt()
    return s.model_current_func_name(model_idx) if s else ""


def get_model_ee_pe321(model_idx: int) -> list:
    s = SystemStateReader.snapshot_rt()
    return s.model_ee_pe321(model_idx) if s else []


# ---- 从站 (NRT) ----
def get_slave_count() -> int:
    s = SystemStateReader.snapshot_nrt()
    return s.slave_count() if s else 0


def get_slave_name(idx: int) -> str:
    s = SystemStateReader.snapshot_nrt()
    return s.slave_name(idx) if s else ""


def get_slave_state(idx: int) -> int:
    s = SystemStateReader.snapshot_nrt()
    return s.slave_state(idx) if s else 0


def get_slave_is_online(idx: int) -> bool:
    s = SystemStateReader.snapshot_nrt()
    return s.slave_is_online(idx) if s else False


# ---- 子系统 (NRT) ----
def get_subsystem_count() -> int:
    s = SystemStateReader.snapshot_nrt()
    return s.subsystem_count() if s else 0


def get_subsystem_name(idx: int) -> str:
    s = SystemStateReader.snapshot_nrt()
    return s.subsystem_name(idx) if s else ""


def get_subsystem_state(idx: int) -> int:
    s = SystemStateReader.snapshot_nrt()
    return s.subsystem_state(idx) if s else 0


def get_subsystem_data_size(idx: int) -> int:
    s = SystemStateReader.snapshot_nrt()
    try:
        return s.subsystem_data_size(idx) if s else 0
    except (UnicodeDecodeError, RuntimeError):
        return 0


def parse_subsystem_data(idx: int, struct_format: str):
    """直接方式解析子系统 data 字段（内部自动获取 NRT 快照）"""
    s = SystemStateReader.snapshot_nrt()
    try:
        return s.parse_subsystem_data(idx, struct_format) if s else None
    except (UnicodeDecodeError, RuntimeError):
        return None


# ---- 接口 (NRT) ----
def get_interface_count() -> int:
    s = SystemStateReader.snapshot_nrt()
    return s.interface_count() if s else 0


def get_interface_name(idx: int) -> str:
    s = SystemStateReader.snapshot_nrt()
    return s.interface_name(idx) if s else ""


def get_interface_state(idx: int) -> int:
    s = SystemStateReader.snapshot_nrt()
    return s.interface_state(idx) if s else 0
