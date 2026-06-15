#include "overall_system_nrtstate.pb.h"
#include "overall_system_rtstate.pb.h"
#include <array>

typedef std::size_t Size;

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
    int global_count;				//全局计数
    std::string master_info;		//主控信息
    bool is_link_up;				//与机器人的链路是否在线
    std::vector<std::vector<double>> ftvalues; //六维力传感器
};

//模型名称及类型
struct ModelsInfo {
    std::string model_name;  
    std::string model_type;  
	bool is_using_sp;               //模型是否开启奇异点检测
	bool is_collision_detection;     //模型是否开启碰撞检测
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
};

struct InterfaceInfo {
    std::string interface_name;	// 接口名称
    int id;						// 接口编号
	int state;					// 接口状态
};



struct SystemStateData {

	//消息头和时间戳、系统运行状态
	int header_timestamp;
	int header_frame_id;
    int system_running_state;

	ControllerInfo controller;
	std::vector<ModelsInfo> models;
    std::vector<SlaveInfo> slaves;
    std::vector<JointInfo> models_joints;
    std::vector<ToolInfo> models_tools;
    std::vector<WobjInfo> models_wobjs;
    std::vector<LoadInfo> models_loads;
    std::vector<PointInfo> models_teach_points;
	std::vector<CurrentPointInfo> models_current_points;
	std::vector<ModelInfo> models_info;
    std::vector<SubsystemInfo> subsystems;
    std::vector<InterfaceInfo> interfaces;
};

using namespace message_bus;
using namespace message_bus::detail;

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

	parm.controller.controller_name = tt.controller().controller_name();
	parm.controller.control_cycle = tt.controller().control_cycle();
	parm.controller.global_count = tt.controller().global_count();
	parm.controller.master_info = tt.controller().master_info();
	parm.controller.is_link_up = tt.controller().is_link_up();

        // 处理六维力传感器数据
    for (Size i = 0; i < tt.controller().ftvalues().size(); i++) {
        auto& ftvalue = tt.controller().ftvalues().at(i);
        std::vector<double> ft_data;
        ft_data.push_back(ftvalue.fx());
        ft_data.push_back(ftvalue.fy());
        ft_data.push_back(ftvalue.fz());
        ft_data.push_back(ftvalue.mx());
        ft_data.push_back(ftvalue.my());
        ft_data.push_back(ftvalue.mz());
        parm.controller.ftvalues.push_back(ft_data);
    }

	// 处理每个模型
	for (Size i = 0; i < tt.model().size(); i++) {
		auto& model = tt.model().at(i);

		// 添加模型基本信息
		ModelsInfo model_info;
		model_info.model_name = model.model_name();
		model_info.model_type = model.model_type();
		parm.models.push_back(model_info);

		// 处理关节信息
		for (Size j = 0; j < model.joint().size(); j++) {
			auto& joint = model.joint().at(j);
			JointInfo joint_info;
			joint_info.joint_type = joint.joint_type();
			joint_info.position = joint.position();
			joint_info.torque = joint.torque();
			joint_info.is_enabled = joint.is_enabled();
			joint_info.mode = joint.mode();
			joint_info.error_code = joint.error_code();
			joint_info.digit_output = joint.digit_output();
			joint_info.digit_input = joint.digit_input();
			parm.models_joints.push_back(joint_info);
		}

		// 处理当前点信息
		if (model.has_current_point()) {
			auto& current_point = model.current_point();
			CurrentPointInfo current_point_info;

			current_point_info.point_name = current_point.point_name();
			current_point_info.tool_name = current_point.tool().tool_name();
			current_point_info.wobj_name = current_point.wobj().wobj_name();

			// 复制工具数据
			current_point_info.tool_data.resize(current_point.tool().data().size());
			for (Size k = 0; k < current_point.tool().data().size(); k++) {
				current_point_info.tool_data[k] = current_point.tool().data().at(k);
			}

			// 复制工件坐标系数据
			current_point_info.wobj_data.resize(current_point.wobj().data().size());
			for (Size k = 0; k < current_point.wobj().data().size(); k++) {
				current_point_info.wobj_data[k] = current_point.wobj().data().at(k);
			}

			// 复制笛卡尔目标位姿
			current_point_info.robottarget.resize(current_point.robottarget().size());
			for (Size k = 0; k < current_point.robottarget().size(); k++) {
				current_point_info.robottarget[k] = current_point.robottarget().at(k);
			}

			// 复制关节目标角度
			current_point_info.jointtarget.resize(current_point.jointtarget().size());
			for (Size k = 0; k < current_point.jointtarget().size(); k++) {
				current_point_info.jointtarget[k] = current_point.jointtarget().at(k);
			}

			parm.models_current_points.push_back(current_point_info);
		}

		// 保存每个模型的 ModelInfo 信息
		ModelInfo model_info_detail;
		model_info_detail.error_code = model.error_code();
		model_info_detail.error_msg = model.error_msg();
		model_info_detail.model_state = model.model_state();
		model_info_detail.model_time_rate = model.model_time_rate();
		model_info_detail.current_func_name = model.current_func_name();
		model_info_detail.current_func_info = model.current_func_info();
		model_info_detail.func_count = model.func_count();

		parm.models_info.push_back(model_info_detail);
	}
}
void display_nrt(overall_system_nrtstate::SystemNrtState tt, SystemStateData& parm) {
    if (!tt.has_head() || !tt.has_controller()) return;

    // 清空所有相关数据
    parm.slaves.clear();
    parm.models.clear();
    parm.models_joints.clear();
    parm.models_tools.clear();
    parm.models_wobjs.clear();
    parm.models_loads.clear();
    parm.models_teach_points.clear();
    parm.subsystems.clear();
    parm.interfaces.clear();

    parm.header_timestamp = tt.head().timestamp();
    parm.header_frame_id = tt.head().frame_id();
    parm.system_running_state = tt.system_running_state();

    // 处理从站信息
    for (Size i = 0; i < tt.controller().slave().size(); i++) {
        auto& slave = tt.controller().slave().at(i);
        SlaveInfo slave_info;
        slave_info.slave_name = slave.slave_name();
        slave_info.phy_id = slave.phy_id();
        slave_info.alias = slave.alias();
        slave_info.slave_state = slave.slave_state();
        slave_info.is_online = slave.is_online();
        slave_info.is_virtual = slave.is_virtual();
        slave_info.is_error = slave.is_error();
        parm.slaves.push_back(slave_info);
    }

    // 处理每个模型
    for (Size i = 0; i < tt.model().size(); i++) {
        auto& model = tt.model().at(i);

        // 添加模型基本信息
        ModelsInfo model_info;
        model_info.model_name = model.model_name();
        model_info.model_type = model.model_type();
        model_info.is_using_sp = model.is_using_sp();
        model_info.is_collision_detection = model.is_collision_detection();
        parm.models.push_back(model_info);

        // 处理关节信息
        for (Size j = 0; j < model.joint().size(); j++) {
            auto& joint = model.joint().at(j);
            JointInfo joint_info;
            joint_info.max_position = joint.max_position();
            joint_info.min_position = joint.min_position();
            joint_info.max_vel = joint.max_vel();
            joint_info.min_vel = joint.min_vel();
            joint_info.max_acc = joint.max_acc();
            joint_info.min_acc = joint.min_acc();
            joint_info.max_collision_torque = joint.max_collision_torque();
            parm.models_joints.push_back(joint_info);
        }

        // 处理工具信息
        for (Size j = 0; j < model.tools().size(); j++) {
            auto& tool = model.tools().at(j);
            ToolInfo tool_info;
            tool_info.tool_name = tool.tool_name();

            tool_info.data.resize(tool.data().size());
            for (Size k = 0; k < tool.data().size(); k++) {
                tool_info.data[k] = tool.data().at(k);
            }
            parm.models_tools.push_back(tool_info);
        }

        // 处理工件坐标系信息
        for (Size j = 0; j < model.wobjs().size(); j++) {
            auto& wobj = model.wobjs().at(j);
            WobjInfo wobj_info;
            wobj_info.wobj_name = wobj.wobj_name();

            wobj_info.data.resize(wobj.data().size());
            for (Size k = 0; k < wobj.data().size(); k++) {
                wobj_info.data[k] = wobj.data().at(k);
            }
            parm.models_wobjs.push_back(wobj_info);
        }

        // 处理负载信息
        for (Size j = 0; j < model.loads().size(); j++) {
            auto& load = model.loads().at(j);
            LoadInfo load_info;
            load_info.load_name = load.load_name();

            load_info.data.resize(load.data().size());
            for (Size k = 0; k < load.data().size(); k++) {
                load_info.data[k] = load.data().at(k);
            }
            parm.models_loads.push_back(load_info);
        }

        // 处理示教点信息
        for (Size j = 0; j < model.teach_points().size(); j++) {
            auto& point = model.teach_points().at(j);
            PointInfo point_info;
            point_info.point_name = point.point_name();
            point_info.tool_name = point.tool().tool_name();
            point_info.wobj_name = point.wobj().wobj_name();

            if (point.has_tool()) {
                point_info.tool_data.resize(point.tool().data().size());
                for (Size k = 0; k < point.tool().data().size(); k++) {
                    point_info.tool_data[k] = point.tool().data().at(k);
                }
            }

            if (point.has_wobj()) {
                point_info.wobj_data.resize(point.wobj().data().size());
                for (Size k = 0; k < point.wobj().data().size(); k++) {
                    point_info.wobj_data[k] = point.wobj().data().at(k);
                }
            }

            point_info.robottarget.resize(point.robottarget().size());
            for (Size k = 0; k < point.robottarget().size(); k++) {
                point_info.robottarget[k] = point.robottarget().at(k);
            }

            point_info.jointtarget.resize(point.jointtarget().size());
            for (Size k = 0; k < point.jointtarget().size(); k++) {
                point_info.jointtarget[k] = point.jointtarget().at(k);
            }

            parm.models_teach_points.push_back(point_info);
        }
    }

    // 处理子系统信息
    for (Size i = 0; i < tt.subsystem().size(); i++) {
        auto& subsystem = tt.subsystem().at(i);
        SubsystemInfo subsystem_info;
        subsystem_info.subsystem_name = subsystem.subsystem_name();
        subsystem_info.id = subsystem.id();
        subsystem_info.state = subsystem.state();
        parm.subsystems.push_back(subsystem_info);
    }

    // 处理接口信息
    for (Size i = 0; i < tt.interface().size(); i++) {
        auto& interface1 = tt.interface().at(i);
        InterfaceInfo interface_info;
        interface_info.interface_name = interface1.interface_name();
        interface_info.id = interface1.id();
        interface_info.state = interface1.state();
        parm.interfaces.push_back(interface_info);
    }
    //std::cout << std::endl;
}	

void display_basic(const overall_system_rtstate::SystemRtState& tt, SystemStateData& parm) {
    if (!tt.has_head() || !tt.has_controller()) return;

    // 清空或重置相关数据
    parm.models.clear();
    parm.models_joints.clear();
    parm.models_current_points.clear();


    // 处理每个模型
    for (Size i = 0; i < tt.model().size(); i++) {
        auto& model = tt.model().at(i);

        // 添加模型基本信息
        ModelsInfo model_info;
        model_info.model_name = model.model_name();
        model_info.model_type = model.model_type();
        parm.models.push_back(model_info);

        // 处理关节信息
        for (Size j = 0; j < model.joint().size(); j++) {
            auto& joint = model.joint().at(j);
            JointInfo joint_info;
            joint_info.position = joint.position();
            joint_info.is_enabled = joint.is_enabled();
            parm.models_joints.push_back(joint_info);
        }

        // 处理当前点信息
        if (model.has_current_point()) {
            auto& current_point = model.current_point();
            CurrentPointInfo current_point_info;

            // 复制笛卡尔位姿
            current_point_info.robottarget.resize(current_point.robottarget().size());
            for (Size k = 0; k < current_point.robottarget().size(); k++) {
                current_point_info.robottarget[k] = current_point.robottarget().at(k);
            }

            // 复制关节角度
            current_point_info.jointtarget.resize(current_point.jointtarget().size());
            for (Size k = 0; k < current_point.jointtarget().size(); k++) {
                current_point_info.jointtarget[k] = current_point.jointtarget().at(k);
            }

            parm.models_current_points.push_back(current_point_info);
        }
    }
}
