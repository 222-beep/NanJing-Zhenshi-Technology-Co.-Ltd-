#ifndef SUB_HPP
#define SUB_HPP

#include "overall_system_nrtstate.pb.h"
#include "overall_system_rtstate.pb.h"
#include <array>
#include <string>
#include <vector>
#include <cstring>

typedef std::size_t Size;



// ---------- 其他数据结构 ----------
struct SlaveInfo {
    std::string slave_name;	// 从站名称
    int phy_id;				// 物理地址
    int alias;				// 逻辑别名
    int slave_state;		// 从站状态机
    bool is_online;			// 是否在线
    bool is_virtual;		// 是否为虚拟/仿真从站
    bool is_error;			// 是否存在错误
};

struct ControllerInfo {
    std::string controller_name;	//控制器名称
    double control_cycle;			//控制周期
    int64_t global_count;				//全局计数
    std::string master_info;		//主控信息
    bool is_link_up;				//与机器人的链路是否在线
    std::vector<std::vector<double>> ftvalues; //六维力传感器
};

//模型名称及类型
struct ModelsInfo {
    std::string model_name;
    std::string model_type;
    bool is_using_sp;               //模型是否开启奇异点检测
    bool is_collision_detection;    //模型是否开启碰撞检测
    int take_photo;                 //启动相机拍照 (NRT)

    // 以下字段用于快速定位该模型在 models_joints 中的范围
    size_t joint_start_idx = 0;     // 起始索引（包含）
    size_t joint_count = 0;         // 关节数量
    size_t tools_start_idx = 0;      // 在 models_tools 中的起始索引
    size_t tools_count = 0;          // 工具数量
    size_t wobjs_start_idx = 0;      // 在 models_wobjs 中的起始索引
    size_t wobjs_count = 0;          // 工件数量
    size_t loads_start_idx = 0;      // 在 models_loads 中的起始索引
    size_t loads_count = 0;          // 负载数量
    size_t teach_points_start_idx = 0; // 在 models_teach_points 中的起始索引
    size_t teach_points_count = 0;     // 示教点数量
    size_t io_start_idx = 0;         // 在 models_io 中的起始索引
    size_t io_count = 0;             // IO 数量
};

//模型运行状态信息
struct ModelInfo {
    int error_code;					// 错误码
    std::string error_msg;			// 错误描述
    int model_state;				// 模型状态
    double model_time_rate;			// 模型运行时间比例或速率
    std::string current_func_name;	// 当前正在执行的函数名
    std::string current_func_info;	// 当前函数的附加信息
    int func_count;					// 函数计数或已调用次数
    std::string info_msg;			// 系统提示信息
    std::vector<double> ee_pe321;	// pe321表示的末端位姿
};

//关节位置和扭矩等
struct JointInfo {
    std::string joint_type;			//关节类型
    double position;
    double torque;					//当前力矩
    bool is_enabled;				//是否上电使能
    int mode;						//控制模式
    int error_code;
    int digit_output;				//数字输出/输入状态
    int digit_input;
    double sensor_torque;			//关节传感器力矩,Nm (RT)
    double velocity;				//关节速度 (RT)
    double target_position;			//目标角度,rad (RT)
    double max_position;			//关节最大/最小位置限制
    double min_position;
    double max_vel;					//最大/最小速度限制
    double min_vel;
    double max_acc;					//最大/最小加速度限制
    double min_acc;
    double max_collision_torque;	//碰撞检测力矩阈值
};

//当前点信息
struct CurrentPointInfo {
    std::string point_name;				// 当前点位的名称
    std::string tool_name;				// 当前使用的工具名称
    std::string wobj_name;				// 当前工件坐标系名称
    std::vector<double> tool_data;		// 工具的 6D 数据
    std::vector<double> wobj_data;		// 工件坐标系的 6D 数据
    std::vector<double> robottarget;	// 当前笛卡尔目标位姿
    std::vector<double> jointtarget;	// 当前关节目标角度
};

struct ToolInfo {
    std::string tool_name;		// 工具名称
    std::vector<double> data;	// 工具数据
};

struct WobjInfo {
    std::string wobj_name;		// 工件坐标系名称
    std::vector<double> data;	// 工件坐标系数据
};

struct LoadInfo {
    std::string load_name;		// 负载名称
    std::vector<double> data;	// 负载数据
};

struct IODataInfo {
    std::string io_name;		// IO名称
    double io_data;				// IO数据
};

//示教点信息
struct PointInfo {
    std::string point_name;
    std::string tool_name;
    std::string wobj_name;
    std::vector<double> tool_data;
    std::vector<double> wobj_data;
    std::vector<double> robottarget;
    std::vector<double> jointtarget;
};

struct SubsystemInfo {
    std::string subsystem_name;	// 子系统名称
    int id;						// 子系统编号
    int state;					// 运行状态
    std::string data;			// 上报的pb消息内容
};

struct InterfaceInfo {
    std::string interface_name;	// 接口名称
    int id;						// 接口编号
    int state;					// 接口状态
};

// ---------- 顶层数据结构 ----------
struct SystemStateData {

    //消息头和时间戳、系统运行状态
    int64_t header_timestamp;
    int64_t header_frame_id;
    bool system_running_state;
    std::string system_info;		// 控制系统开启停止等信息 (RT)
    bool system_is_init;			// codeit系统是否已经完成了初始化 (NRT)

    ControllerInfo controller;
    std::vector<ModelsInfo> models;
    std::vector<SlaveInfo> slaves;
    std::vector<JointInfo> models_joints;
    std::vector<ToolInfo> models_tools;
    std::vector<WobjInfo> models_wobjs;
    std::vector<LoadInfo> models_loads;
    std::vector<IODataInfo> models_io;
    std::vector<PointInfo> models_teach_points;
    std::vector<CurrentPointInfo> models_current_points;
    std::vector<ModelInfo> models_info;
    std::vector<SubsystemInfo> subsystems;
    std::vector<InterfaceInfo> interfaces;
};

// ---------- RT 转换函数 ----------
void display_rt(const overall_system_rtstate::SystemRtState& tt, SystemStateData& parm) {
    if (!tt.has_head() || !tt.has_controller()) return;

    // 清空或重置相关数据
    parm.models.clear();
    parm.models_joints.clear();
    parm.models_current_points.clear();
    parm.models_info.clear();

    parm.header_timestamp = tt.head().timestamp();
    parm.header_frame_id = tt.head().frame_id();
    parm.system_running_state = tt.system_running_state();
    parm.system_info = tt.system_info();

    parm.controller.controller_name = tt.controller().controller_name();
    parm.controller.control_cycle = tt.controller().control_cycle();
    parm.controller.global_count = tt.controller().global_count();
    parm.controller.master_info = tt.controller().master_info();
    parm.controller.is_link_up = tt.controller().is_link_up();

    for (int i = 0; i < tt.controller().ftvalues_size(); ++i) {
        auto& ft = tt.controller().ftvalues(i);
        std::vector<double> ft_data = { ft.fx(), ft.fy(), ft.fz(), ft.mx(), ft.my(), ft.mz() };
        parm.controller.ftvalues.push_back(std::move(ft_data));
    }

    // 遍历每个模型
    for (int i = 0; i < tt.model_size(); ++i) {
        auto& model = tt.model(i);
        // 记录该模型在 joints 数组中的起始位置
        size_t joint_start = parm.models_joints.size();

        ModelsInfo model_info;
        model_info.model_name = model.model_name();
        model_info.model_type = model.model_type();
        // 记录关节范围
        model_info.joint_start_idx = joint_start;
        model_info.joint_count = model.joint_size();
        parm.models.push_back(std::move(model_info));

        // 关节信息
        for (int j = 0; j < model.joint_size(); ++j) {
            auto& joint = model.joint(j);
            JointInfo joint_info;
            joint_info.joint_type = joint.joint_type();
            joint_info.position = joint.position();
            joint_info.torque = joint.torque();
            joint_info.is_enabled = joint.is_enabled();
            joint_info.mode = joint.mode();
            joint_info.error_code = joint.error_code();
            joint_info.digit_output = joint.digit_output();
            joint_info.digit_input = joint.digit_input();
            joint_info.sensor_torque = joint.sensor_torque();
            joint_info.velocity = joint.velocity();
            joint_info.target_position = joint.target_position();
            parm.models_joints.push_back(std::move(joint_info));
        }

        // 当前点信息
        if (model.has_current_point()) {
            auto& cur = model.current_point();
            CurrentPointInfo cur_info;
            cur_info.point_name = cur.point_name();
            cur_info.tool_name = cur.tool().tool_name();
            cur_info.wobj_name = cur.wobj().wobj_name();
            cur_info.tool_data.assign(cur.tool().data().begin(), cur.tool().data().end());
            cur_info.wobj_data.assign(cur.wobj().data().begin(), cur.wobj().data().end());
            cur_info.robottarget.assign(cur.robottarget().begin(), cur.robottarget().end());
            cur_info.jointtarget.assign(cur.jointtarget().begin(), cur.jointtarget().end());
            parm.models_current_points.push_back(std::move(cur_info));
        }

        // 保存每个模型的 ModelInfo 信息
        ModelInfo info_detail;
        info_detail.error_code = model.error_code();
        info_detail.error_msg = model.error_msg();
        info_detail.model_state = model.model_state();
        info_detail.model_time_rate = model.model_time_rate();
        info_detail.current_func_name = model.current_func_name();
        info_detail.current_func_info = model.current_func_info();
        info_detail.func_count = model.func_count();
        info_detail.info_msg = model.info_msg();
        info_detail.ee_pe321.assign(model.ee_pe321().begin(), model.ee_pe321().end());
        parm.models_info.push_back(std::move(info_detail));
    }
}

// ---------- NRT 转换函数 ----------
void display_nrt(const overall_system_nrtstate::SystemNrtState& tt, SystemStateData& parm) {
    if (!tt.has_head() || !tt.has_controller()) return;

    parm.slaves.clear();
    parm.models.clear();
    parm.models_joints.clear();
    parm.models_tools.clear();
    parm.models_wobjs.clear();
    parm.models_loads.clear();
    parm.models_io.clear();
    parm.models_teach_points.clear();
    parm.subsystems.clear();
    parm.interfaces.clear();

    parm.header_timestamp = tt.head().timestamp();
    parm.header_frame_id = tt.head().frame_id();
    parm.system_running_state = tt.system_running_state();
    parm.system_is_init = tt.system_is_init();

    // 从站信息
    for (int i = 0; i < tt.controller().slave_size(); ++i) {
        auto& slave = tt.controller().slave(i);
        SlaveInfo info;
        info.slave_name = slave.slave_name();
        info.phy_id = slave.phy_id();
        info.alias = slave.alias();
        info.slave_state = slave.slave_state();
        info.is_online = slave.is_online();
        info.is_virtual = slave.is_virtual();
        info.is_error = slave.is_error();
        parm.slaves.push_back(std::move(info));
    }

    // 遍历模型
    for (int i = 0; i < tt.model_size(); ++i) {
        auto& model = tt.model(i);

        // 记录该模型在 joints 数组中的起始位置
        size_t joint_start = parm.models_joints.size();

        // ==========记录 tools, wobjs, loads, teach_points 起始索引 ==========
        size_t tools_start = parm.models_tools.size();
        size_t wobjs_start = parm.models_wobjs.size();
        size_t loads_start = parm.models_loads.size();
        size_t teach_points_start = parm.models_teach_points.size();

        // 创建 ModelsInfo 并填写基本信息
        ModelsInfo model_info;
        model_info.model_name = model.model_name();
        model_info.model_type = model.model_type();
        model_info.is_using_sp = model.is_using_sp();
        model_info.is_collision_detection = model.is_collision_detection();
        model_info.take_photo = model.take_photo();
        model_info.joint_start_idx = joint_start;
        model_info.joint_count = model.joint_size();

        // 关节限制
        for (int j = 0; j < model.joint_size(); ++j) {
            auto& joint = model.joint(j);
            JointInfo j_info;
            j_info.max_position = joint.max_position();
            j_info.min_position = joint.min_position();
            j_info.max_vel = joint.max_vel();
            j_info.min_vel = joint.min_vel();
            j_info.max_acc = joint.max_acc();
            j_info.min_acc = joint.min_acc();
            j_info.max_collision_torque = joint.max_collision_torque();
            parm.models_joints.push_back(std::move(j_info));
        }

        // 工具
        for (int j = 0; j < model.tools_size(); ++j) {
            auto& tool = model.tools(j);
            ToolInfo t_info;
            t_info.tool_name = tool.tool_name();
            t_info.data.assign(tool.data().begin(), tool.data().end());
            parm.models_tools.push_back(std::move(t_info));
        }

        // 工件
        for (int j = 0; j < model.wobjs_size(); ++j) {
            auto& wobj = model.wobjs(j);
            WobjInfo w_info;
            w_info.wobj_name = wobj.wobj_name();
            w_info.data.assign(wobj.data().begin(), wobj.data().end());
            parm.models_wobjs.push_back(std::move(w_info));
        }

        // 负载
        for (int j = 0; j < model.loads_size(); ++j) {
            auto& load = model.loads(j);
            LoadInfo l_info;
            l_info.load_name = load.load_name();
            l_info.data.assign(load.data().begin(), load.data().end());
            parm.models_loads.push_back(std::move(l_info));
        }

        // IO
        size_t io_start = parm.models_io.size();
        for (int j = 0; j < model.io_size(); ++j) {
            auto& io = model.io(j);
            IODataInfo io_info;
            io_info.io_name = io.io_name();
            io_info.io_data = io.io_data();
            parm.models_io.push_back(std::move(io_info));
        }

        // 示教点
        for (int j = 0; j < model.teach_points_size(); ++j) {
            auto& point = model.teach_points(j);
            PointInfo p_info;
            p_info.point_name = point.point_name();
            p_info.tool_name = point.tool().tool_name();
            p_info.wobj_name = point.wobj().wobj_name();
            if (point.has_tool())
                p_info.tool_data.assign(point.tool().data().begin(), point.tool().data().end());
            if (point.has_wobj())
                p_info.wobj_data.assign(point.wobj().data().begin(), point.wobj().data().end());
            p_info.robottarget.assign(point.robottarget().begin(), point.robottarget().end());
            p_info.jointtarget.assign(point.jointtarget().begin(), point.jointtarget().end());
            parm.models_teach_points.push_back(std::move(p_info));
        }

        // 填充模型索引
        model_info.tools_start_idx = tools_start;
        model_info.tools_count = model.tools_size();
        model_info.wobjs_start_idx = wobjs_start;
        model_info.wobjs_count = model.wobjs_size();
        model_info.loads_start_idx = loads_start;
        model_info.loads_count = model.loads_size();
        model_info.teach_points_start_idx = teach_points_start;
        model_info.teach_points_count = model.teach_points_size();
        model_info.io_start_idx = io_start;
        model_info.io_count = model.io_size();

        // 将模型信息加入列表
        parm.models.push_back(std::move(model_info));
    }

    // 子系统
    for (int i = 0; i < tt.subsystem_size(); ++i) {
        auto& sub = tt.subsystem(i);
        SubsystemInfo info;
        info.subsystem_name = sub.subsystem_name();
        info.id = sub.id();
        info.state = sub.state();
        info.data = sub.data();   // 存储原始 bytes
        parm.subsystems.push_back(info);
    }

    // 接口
    for (int i = 0; i < tt.interface_size(); ++i) {
        auto& iface = tt.interface(i);
        InterfaceInfo info;
        info.interface_name = iface.interface_name();
        info.id = iface.id();
        info.state = iface.state();
        parm.interfaces.push_back(std::move(info));
    }
}

#endif // SUB_HPP