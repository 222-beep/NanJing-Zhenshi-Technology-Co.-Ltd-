#ifndef SYSTEM_STATE_READER_HPP
#define SYSTEM_STATE_READER_HPP

#include "shared_data.hpp"
#include <iostream>
#include <memory>
#include <cstring>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>

// ============================================================================
// SystemStateReader — 在 SharedSystemState 之上封装的便捷只读访问层
//
// 用法：
//   auto snapshot = SystemStateReader::snapshotRt();   // 或 snapshotNrt()
//   if (snapshot) {
//       double pos = snapshot.getJointPosition(0, 2);
//       auto gripper = snapshot.parseSubsystemData<MyGripperStruct>(0);
//   }
// ============================================================================

class SystemStateReader {
public:
    // ---- 工厂：从全局状态获取快照 ----
    static SystemStateReader snapshotRt() {
        return SystemStateReader(getSystemState().getRt(), getSystemState().hasRt());
    }
    static SystemStateReader snapshotNrt() {
        return SystemStateReader(getSystemState().getNrt(), getSystemState().hasNrt());
    }

    // 有效性检查
    operator bool() const { return data_ != nullptr; }
    bool valid() const { return data_ != nullptr; }

    // 直接访问底层数据（需要时）
    const SystemStateData* raw() const { return data_.get(); }

    // ========================================================================
    // 顶层字段
    // ========================================================================
    int64_t headerTimestamp()    const { return data_->header_timestamp; }
    int64_t headerFrameId()      const { return data_->header_frame_id; }
    bool    isSystemRunning()    const { return data_->system_running_state; }

    // ---- RT only ----
    const std::string& systemInfo() const { return data_->system_info; }

    // ---- NRT only ----
    bool isSystemInit() const { return data_->system_is_init; }

    // ========================================================================
    // 控制器 (ControllerInfo) —— 主要来自 RT
    // ========================================================================
    const std::string& controllerName() const { return data_->controller.controller_name; }
    double controlCycle()              const { return data_->controller.control_cycle; }
    int64_t globalCount()              const { return data_->controller.global_count; }
    const std::string& masterInfo()    const { return data_->controller.master_info; }
    bool    isLinkUp()                 const { return data_->controller.is_link_up; }

    size_t ftValuesCount() const { return data_->controller.ftvalues.size(); }
    const std::vector<double>& ftValue(size_t sensor_idx) const {
        return data_->controller.ftvalues.at(sensor_idx);
    }
    // 单个传感器分量快捷访问
    double ftValueFx(size_t sensor_idx) const { return data_->controller.ftvalues.at(sensor_idx).at(0); }
    double ftValueFy(size_t sensor_idx) const { return data_->controller.ftvalues.at(sensor_idx).at(1); }
    double ftValueFz(size_t sensor_idx) const { return data_->controller.ftvalues.at(sensor_idx).at(2); }
    double ftValueMx(size_t sensor_idx) const { return data_->controller.ftvalues.at(sensor_idx).at(3); }
    double ftValueMy(size_t sensor_idx) const { return data_->controller.ftvalues.at(sensor_idx).at(4); }
    double ftValueMz(size_t sensor_idx) const { return data_->controller.ftvalues.at(sensor_idx).at(5); }

    // ========================================================================
    // 模型 (ModelsInfo) —— 元信息 + 数量
    // ========================================================================
    size_t modelCount() const { return data_->models.size(); }

    const ModelsInfo& modelInfo(size_t model_idx) const { return data_->models.at(model_idx); }

    const std::string& modelName(size_t model_idx) const { return data_->models.at(model_idx).model_name; }
    const std::string& modelType(size_t model_idx) const { return data_->models.at(model_idx).model_type; }

    // ---- NRT only ----
    bool isModelUsingSP(size_t model_idx)            const { return data_->models.at(model_idx).is_using_sp; }
    bool isModelCollisionDetection(size_t model_idx) const { return data_->models.at(model_idx).is_collision_detection; }
    int  modelTakePhoto(size_t model_idx)            const { return data_->models.at(model_idx).take_photo; }

    // ========================================================================
    // 关节 (JointInfo) —— 核心访问：通过模型号 + 关节号
    // ========================================================================

    // ---- RT 值 ----
    const JointInfo& joint(size_t model_idx, size_t joint_idx) const {
        return data_->models_joints.at(data_->models.at(model_idx).joint_start_idx + joint_idx);
    }

    const std::string& jointType(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).joint_type;
    }
    double jointPosition(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).position;
    }
    double jointTorque(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).torque;
    }
    bool   jointIsEnabled(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).is_enabled;
    }
    int    jointMode(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).mode;
    }
    int    jointErrorCode(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).error_code;
    }
    int    jointDigitOutput(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).digit_output;
    }
    int    jointDigitInput(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).digit_input;
    }
    double jointSensorTorque(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).sensor_torque;
    }
    double jointVelocity(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).velocity;
    }
    double jointTargetPosition(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).target_position;
    }

    // ---- NRT 限制值 ----
    double jointMaxPosition(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).max_position;
    }
    double jointMinPosition(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).min_position;
    }
    double jointMaxVel(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).max_vel;
    }
    double jointMinVel(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).min_vel;
    }
    double jointMaxAcc(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).max_acc;
    }
    double jointMinAcc(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).min_acc;
    }
    double jointMaxCollisionTorque(size_t model_idx, size_t joint_idx) const {
        return joint(model_idx, joint_idx).max_collision_torque;
    }

    // ---- 便捷：获取某模型下的关节数量 ----
    size_t jointCount(size_t model_idx) const {
        return data_->models.at(model_idx).joint_count;
    }

    // ========================================================================
    // 当前点 (CurrentPointInfo) —— RT
    // ========================================================================
    bool hasCurrentPoint(size_t model_idx) const {
        return model_idx < data_->models_current_points.size();
    }
    const CurrentPointInfo& currentPoint(size_t model_idx) const {
        return data_->models_current_points.at(model_idx);
    }
    const std::string& currentPointName(size_t model_idx) const {
        return data_->models_current_points.at(model_idx).point_name;
    }
    const std::string& currentToolName(size_t model_idx) const {
        return data_->models_current_points.at(model_idx).tool_name;
    }
    const std::string& currentWobjName(size_t model_idx) const {
        return data_->models_current_points.at(model_idx).wobj_name;
    }
    const std::vector<double>& currentToolData(size_t model_idx) const {
        return data_->models_current_points.at(model_idx).tool_data;
    }
    const std::vector<double>& currentWobjData(size_t model_idx) const {
        return data_->models_current_points.at(model_idx).wobj_data;
    }
    const std::vector<double>& currentRobottarget(size_t model_idx) const {
        return data_->models_current_points.at(model_idx).robottarget;
    }
    const std::vector<double>& currentJointtarget(size_t model_idx) const {
        return data_->models_current_points.at(model_idx).jointtarget;
    }

    // ========================================================================
    // 模型运行状态 (ModelInfo) —— RT
    // ========================================================================
    const ModelInfo& modelStateInfo(size_t model_idx) const {
        return data_->models_info.at(model_idx);
    }
    int    modelErrorCode(size_t model_idx)      const { return data_->models_info.at(model_idx).error_code; }
    const std::string& modelErrorMsg(size_t model_idx)      const { return data_->models_info.at(model_idx).error_msg; }
    int    modelState(size_t model_idx)          const { return data_->models_info.at(model_idx).model_state; }
    double modelTimeRate(size_t model_idx)       const { return data_->models_info.at(model_idx).model_time_rate; }
    const std::string& modelCurrentFuncName(size_t model_idx) const { return data_->models_info.at(model_idx).current_func_name; }
    const std::string& modelCurrentFuncInfo(size_t model_idx) const { return data_->models_info.at(model_idx).current_func_info; }
    int    modelFuncCount(size_t model_idx)      const { return data_->models_info.at(model_idx).func_count; }
    const std::string& modelInfoMsg(size_t model_idx)       const { return data_->models_info.at(model_idx).info_msg; }
    const std::vector<double>& modelEePe321(size_t model_idx) const { return data_->models_info.at(model_idx).ee_pe321; }

    // ========================================================================
    // 工具 (ToolInfo) —— NRT（按模型索引）
    // ========================================================================
    size_t toolCount(size_t model_idx) const {
        return data_->models.at(model_idx).tools_count;
    }
    const std::string& toolName(size_t model_idx, size_t tool_idx) const {
        return data_->models_tools.at(data_->models.at(model_idx).tools_start_idx + tool_idx).tool_name;
    }
    const std::vector<double>& toolData(size_t model_idx, size_t tool_idx) const {
        return data_->models_tools.at(data_->models.at(model_idx).tools_start_idx + tool_idx).data;
    }

    // ========================================================================
    // 工件 (WobjInfo) —— NRT（按模型索引）
    // ========================================================================
    size_t wobjCount(size_t model_idx) const {
        return data_->models.at(model_idx).wobjs_count;
    }
    const std::string& wobjName(size_t model_idx, size_t wobj_idx) const {
        return data_->models_wobjs.at(data_->models.at(model_idx).wobjs_start_idx + wobj_idx).wobj_name;
    }
    const std::vector<double>& wobjData(size_t model_idx, size_t wobj_idx) const {
        return data_->models_wobjs.at(data_->models.at(model_idx).wobjs_start_idx + wobj_idx).data;
    }

    // ========================================================================
    // 负载 (LoadInfo) —— NRT（按模型索引）
    // ========================================================================
    size_t loadCount(size_t model_idx) const {
        return data_->models.at(model_idx).loads_count;
    }
    const std::string& loadName(size_t model_idx, size_t load_idx) const {
        return data_->models_loads.at(data_->models.at(model_idx).loads_start_idx + load_idx).load_name;
    }
    const std::vector<double>& loadData(size_t model_idx, size_t load_idx) const {
        return data_->models_loads.at(data_->models.at(model_idx).loads_start_idx + load_idx).data;
    }

    // ========================================================================
    // IO (IODataInfo) —— NRT（按模型索引）
    // ========================================================================
    size_t ioCount(size_t model_idx) const {
        return data_->models.at(model_idx).io_count;
    }
    const std::string& ioName(size_t model_idx, size_t io_idx) const {
        return data_->models_io.at(data_->models.at(model_idx).io_start_idx + io_idx).io_name;
    }
    double ioData(size_t model_idx, size_t io_idx) const {
        return data_->models_io.at(data_->models.at(model_idx).io_start_idx + io_idx).io_data;
    }
    // 全局 IO 数量（兼容旧用法）
    size_t ioTotalCount() const { return data_->models_io.size(); }

    // ========================================================================
    // 示教点 (PointInfo) —— NRT（按模型索引）
    // ========================================================================
    size_t teachPointCount(size_t model_idx) const {
        return data_->models.at(model_idx).teach_points_count;
    }
    const PointInfo& teachPoint(size_t model_idx, size_t point_idx) const {
        return data_->models_teach_points.at(data_->models.at(model_idx).teach_points_start_idx + point_idx);
    }
    const std::string& teachPointName(size_t model_idx, size_t point_idx) const {
        return data_->models_teach_points.at(data_->models.at(model_idx).teach_points_start_idx + point_idx).point_name;
    }
    const std::string& teachPointToolName(size_t model_idx, size_t point_idx) const {
        return data_->models_teach_points.at(data_->models.at(model_idx).teach_points_start_idx + point_idx).tool_name;
    }
    const std::string& teachPointWobjName(size_t model_idx, size_t point_idx) const {
        return data_->models_teach_points.at(data_->models.at(model_idx).teach_points_start_idx + point_idx).wobj_name;
    }
    const std::vector<double>& teachPointToolData(size_t model_idx, size_t point_idx) const {
        return data_->models_teach_points.at(data_->models.at(model_idx).teach_points_start_idx + point_idx).tool_data;
    }
    const std::vector<double>& teachPointWobjData(size_t model_idx, size_t point_idx) const {
        return data_->models_teach_points.at(data_->models.at(model_idx).teach_points_start_idx + point_idx).wobj_data;
    }
    const std::vector<double>& teachPointRobottarget(size_t model_idx, size_t point_idx) const {
        return data_->models_teach_points.at(data_->models.at(model_idx).teach_points_start_idx + point_idx).robottarget;
    }
    const std::vector<double>& teachPointJointtarget(size_t model_idx, size_t point_idx) const {
        return data_->models_teach_points.at(data_->models.at(model_idx).teach_points_start_idx + point_idx).jointtarget;
    }

    // ========================================================================
    // 从站 (SlaveInfo) —— NRT
    // ========================================================================
    size_t slaveCount() const { return data_->slaves.size(); }
    const SlaveInfo& slave(size_t idx) const { return data_->slaves.at(idx); }
    const std::string& slaveName(size_t idx) const { return data_->slaves.at(idx).slave_name; }
    int  slavePhyId(size_t idx)   const { return data_->slaves.at(idx).phy_id; }
    int  slaveAlias(size_t idx)   const { return data_->slaves.at(idx).alias; }
    int  slaveState(size_t idx)   const { return data_->slaves.at(idx).slave_state; }
    bool slaveIsOnline(size_t idx)  const { return data_->slaves.at(idx).is_online; }
    bool slaveIsVirtual(size_t idx) const { return data_->slaves.at(idx).is_virtual; }
    bool slaveIsError(size_t idx)   const { return data_->slaves.at(idx).is_error; }

    // ========================================================================
    // 子系统 (SubsystemInfo) —— NRT
    // ========================================================================
    size_t subsystemCount() const { return data_->subsystems.size(); }
    const SubsystemInfo& subsystem(size_t idx) const { return data_->subsystems.at(idx); }
    const std::string& subsystemName(size_t idx)  const { return data_->subsystems.at(idx).subsystem_name; }
    int  subsystemId(size_t idx)    const { return data_->subsystems.at(idx).id; }
    int  subsystemState(size_t idx) const { return data_->subsystems.at(idx).state; }
    const std::string& subsystemRawData(size_t idx) const { return data_->subsystems.at(idx).data; }
    size_t subsystemDataSize(size_t idx) const { return data_->subsystems.at(idx).data.size(); }

    // -------- 子系统 data 解析接口 --------

    // 方式1：模板方法，将 data 按二进制解析为用户定义的 struct（POD/trivial 类型）
    //   要求 T 是 trivially copyable，且 data 大小 >= sizeof(T)
    template<typename T>
    T parseSubsystemData(size_t idx) const {
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be trivially copyable for binary parsing");
        const auto& raw = data_->subsystems.at(idx).data;
        if (raw.size() < sizeof(T)) {
            throw std::runtime_error(
                "Subsystem data too small: need " + std::to_string(sizeof(T)) +
                " bytes, got " + std::to_string(raw.size()));
        }
        T result;
        std::memcpy(&result, raw.data(), sizeof(T));
        return result;
    }

    // 方式2：通过回调/函数对象解析（用户传入自定义解析逻辑）
    //   回调签名为: T(const std::string& data)
    template<typename Callback>
    auto parseSubsystemDataWith(size_t idx, Callback&& parser) const
        -> decltype(parser(std::declval<const std::string&>()))
    {
        return parser(data_->subsystems.at(idx).data);
    }

    // ========================================================================
    // 接口 (InterfaceInfo) —— NRT
    // ========================================================================
    size_t interfaceCount() const { return data_->interfaces.size(); }
    const InterfaceInfo& interface(size_t idx) const { return data_->interfaces.at(idx); }
    const std::string& interfaceName(size_t idx)  const { return data_->interfaces.at(idx).interface_name; }
    int  interfaceId(size_t idx)    const { return data_->interfaces.at(idx).id; }
    int  interfaceState(size_t idx) const { return data_->interfaces.at(idx).state; }

private:
    explicit SystemStateReader(std::shared_ptr<const SystemStateData> data, bool valid)
        : data_(valid ? std::move(data) : nullptr) {}

    std::shared_ptr<const SystemStateData> data_;
};

// ============================================================================
// 自由函数 —— 一步到位，无需手动获取快照
//
// 用法：
//   double pos = getJointPosition(0, 2);        // 模型0, 关节2 的位置
//   if (hasRtData()) { ... }                    // 检查是否有 RT 数据
//   auto gripper = parseSubsystemData<TwoFingerGripperStatus>(0);
// ============================================================================

inline bool hasRtData()  { return getSystemState().hasRt(); }
inline bool hasNrtData() { return getSystemState().hasNrt(); }

// ---- 顶层 ----
inline int64_t  getHeaderTimestamp()   { auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.headerTimestamp() : 0; }
inline int64_t  getHeaderFrameId()     { auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.headerFrameId() : 0; }
inline bool     isSystemRunning()      { auto s = SystemStateReader::snapshotRt(); return s.valid() && s.isSystemRunning(); }
inline std::string getSystemInfo()     { auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.systemInfo() : ""; }
inline bool     isSystemInit()         { auto s = SystemStateReader::snapshotNrt(); return s.valid() && s.isSystemInit(); }

// ---- 控制器 ----
inline std::string getControllerName() { auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.controllerName() : ""; }
inline double getControlCycle()        { auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.controlCycle() : 0.0; }
inline int64_t getGlobalCount()        { auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.globalCount() : 0; }
inline std::string getMasterInfo()     { auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.masterInfo() : ""; }
inline bool isLinkUp()                 { auto s = SystemStateReader::snapshotRt(); return s.valid() && s.isLinkUp(); }
inline size_t getFtValuesCount()       { auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.ftValuesCount() : 0; }
inline std::vector<double> getFtValue(size_t sensor_idx) {
    auto s = SystemStateReader::snapshotRt();
    return s.valid() ? s.ftValue(sensor_idx) : std::vector<double>();
}

// ---- 模型数量 ----
inline size_t getModelCountRt()  { auto s = SystemStateReader::snapshotRt();  return s.valid() ? s.modelCount() : 0; }
inline size_t getModelCountNrt() { auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.modelCount() : 0; }
inline std::string getModelName(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt();
    if (!s.valid()) { s = SystemStateReader::snapshotNrt(); }
    return s.valid() ? s.modelName(model_idx) : "";
}
inline std::string getModelType(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt();
    if (!s.valid()) { s = SystemStateReader::snapshotNrt(); }
    return s.valid() ? s.modelType(model_idx) : "";
}
inline size_t getJointCount(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt();
    if (!s.valid()) { s = SystemStateReader::snapshotNrt(); }
    return s.valid() ? s.jointCount(model_idx) : 0;
}

// ---- 关节 RT 值（模型号, 关节号）----
inline double getJointPosition(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointPosition(model_idx, joint_idx) : 0.0;
}
inline double getJointTorque(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointTorque(model_idx, joint_idx) : 0.0;
}
inline bool getJointIsEnabled(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() && s.jointIsEnabled(model_idx, joint_idx);
}
inline int getJointMode(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointMode(model_idx, joint_idx) : 0;
}
inline int getJointErrorCode(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointErrorCode(model_idx, joint_idx) : 0;
}
inline int getJointDigitOutput(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointDigitOutput(model_idx, joint_idx) : 0;
}
inline int getJointDigitInput(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointDigitInput(model_idx, joint_idx) : 0;
}
inline double getJointSensorTorque(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointSensorTorque(model_idx, joint_idx) : 0.0;
}
inline double getJointVelocity(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointVelocity(model_idx, joint_idx) : 0.0;
}
inline double getJointTargetPosition(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointTargetPosition(model_idx, joint_idx) : 0.0;
}
inline std::string getJointType(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.jointType(model_idx, joint_idx) : "";
}

// ---- 关节 NRT 限制值（模型号, 关节号）----
inline double getJointMaxPosition(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.jointMaxPosition(model_idx, joint_idx) : 0.0;
}
inline double getJointMinPosition(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.jointMinPosition(model_idx, joint_idx) : 0.0;
}
inline double getJointMaxVel(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.jointMaxVel(model_idx, joint_idx) : 0.0;
}
inline double getJointMinVel(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.jointMinVel(model_idx, joint_idx) : 0.0;
}
inline double getJointMaxAcc(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.jointMaxAcc(model_idx, joint_idx) : 0.0;
}
inline double getJointMinAcc(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.jointMinAcc(model_idx, joint_idx) : 0.0;
}
inline double getJointMaxCollisionTorque(size_t model_idx, size_t joint_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.jointMaxCollisionTorque(model_idx, joint_idx) : 0.0;
}

// ---- 模型 NRT 属性 ----
inline bool isModelUsingSP(size_t model_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() && s.isModelUsingSP(model_idx);
}
inline bool isModelCollisionDetection(size_t model_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() && s.isModelCollisionDetection(model_idx);
}
inline int getModelTakePhoto(size_t model_idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.modelTakePhoto(model_idx) : 0;
}

// ---- 当前点 (RT) ----
inline std::string getCurrentPointName(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.currentPointName(model_idx) : "";
}
inline std::string getCurrentToolName(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.currentToolName(model_idx) : "";
}
inline std::string getCurrentWobjName(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.currentWobjName(model_idx) : "";
}
inline std::vector<double> getCurrentRobottarget(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.currentRobottarget(model_idx) : std::vector<double>();
}
inline std::vector<double> getCurrentJointtarget(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.currentJointtarget(model_idx) : std::vector<double>();
}

// ---- 模型运行状态 (RT) ----
inline int getModelErrorCode(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.modelErrorCode(model_idx) : 0;
}
inline std::string getModelErrorMsg(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.modelErrorMsg(model_idx) : "";
}
inline int getModelState(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.modelState(model_idx) : 0;
}
inline double getModelTimeRate(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.modelTimeRate(model_idx) : 0.0;
}
inline std::string getModelCurrentFuncName(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.modelCurrentFuncName(model_idx) : "";
}
inline std::vector<double> getModelEePe321(size_t model_idx) {
    auto s = SystemStateReader::snapshotRt(); return s.valid() ? s.modelEePe321(model_idx) : std::vector<double>();
}

// ---- 从站 (NRT) ----
inline size_t getSlaveCount() { auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.slaveCount() : 0; }
inline std::string getSlaveName(size_t idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.slaveName(idx) : "";
}
inline int getSlaveState(size_t idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.slaveState(idx) : 0;
}
inline bool getSlaveIsOnline(size_t idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() && s.slaveIsOnline(idx);
}

// ---- 子系统 (NRT) ----
inline size_t getSubsystemCount() { auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.subsystemCount() : 0; }
inline std::string getSubsystemName(size_t idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.subsystemName(idx) : "";
}
inline int getSubsystemState(size_t idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.subsystemState(idx) : 0;
}
inline size_t getSubsystemDataSize(size_t idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.subsystemDataSize(idx) : 0;
}
// 子系统 data 解析（POD 结构体）
template<typename T>
T parseSubsystemData(size_t idx) {
    auto s = SystemStateReader::snapshotNrt();
    if (!s.valid()) throw std::runtime_error("No NRT data available");
    return s.parseSubsystemData<T>(idx);
}

// 子系统 data 解析（回调方式）
template<typename Callback>
auto parseSubsystemDataWith(size_t idx, Callback&& parser)
    -> decltype(parser(std::declval<const std::string&>()))
{
    auto s = SystemStateReader::snapshotNrt();
    if (!s.valid()) throw std::runtime_error("No NRT data available");
    return s.parseSubsystemDataWith(idx, std::forward<Callback>(parser));
}

// ---- 接口 (NRT) ----
inline size_t getInterfaceCount() { auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.interfaceCount() : 0; }
inline std::string getInterfaceName(size_t idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.interfaceName(idx) : "";
}
inline int getInterfaceState(size_t idx) {
    auto s = SystemStateReader::snapshotNrt(); return s.valid() ? s.interfaceState(idx) : 0;
}

// ============================================================================
// 辅助函数
// ============================================================================

// 打印 vector<double> 的内容，max_print 控制最大打印长度
inline void print_vector(const std::vector<double>& vec, size_t max_print = ~size_t(0)) {
    std::cout << "[";
    for (size_t i = 0; i < vec.size() && i < max_print; ++i) {
        std::cout << vec[i] << (i + 1 < vec.size() ? ", " : "");
    }
    if (vec.size() > max_print) std::cout << "...";
    std::cout << "]";
}

#endif // SYSTEM_STATE_READER_HPP
