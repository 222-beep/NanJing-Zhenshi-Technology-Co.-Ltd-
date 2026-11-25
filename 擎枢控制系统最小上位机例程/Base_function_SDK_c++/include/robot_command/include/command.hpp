#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <future>
#include "cpp_rpc.hpp"
#include "arm_params.hpp"
#include "message/rpc_client.h"

// ====================== 命令参数结构体 ======================
/**
 * @brief 命令参数结构体，用于封装各种命令所需的参数
 */
struct CommandParams {
    std::string command_type;    // 命令类型，如 "Var", "SetTool", "Move_Absj" 等
    std::string var_type;        // 变量类型：tool, jointtarget, robottarget, wobj, clear 等
    std::string target_name;     // 目标点名称
    std::string motion_id;       // 电机ID
    std::string mid_target_name; // MoveBlend 特有参数，中间目标点名称
    std::string raw_command;     // 原始命令字符串，备用参数
    std::vector<double> position;     // [x, y, z]，三维位置向量，单位：米
    std::vector<double> quaternion;   // [qx, qy, qz, qw]，四元数姿态
    std::vector<double> joint_angles; // [j1..j10]，关节角度，单位：弧度
    std::vector<double> distance;     // 三指夹爪九个关节角度，单位：度
    std::vector<double> zone;         // 平滑参数，[dis, per]
    std::vector<double> vel;          // 旋量速度，[x,y,z,rx,ry,rz]，单位 rad/s
    std::vector<double> acc;          // 旋量加速度，单位 rad/s²
    std::vector<double> dec;          // 旋量减速度，单位 rad/s²
    std::vector<double> jerk;         // 旋量加加速度，单位 rad/s³
    std::vector<double> cf_coef;      // 关节速度前馈系数（阻尼），范围 0~1
    std::vector<double> vf_coef;      // 关节速度前馈系数，范围 0~1
    std::vector<int> force_direction; // 允许拖动的自由度，1 启用，0 禁用
    std::vector<int> force_target;    // 期望力/力矩设定 {Fx, Fy, Fz, Tx, Ty, Tz}，单位 N
    double ratio = 0.0;               // 笛卡尔加速度与速度比率，范围 1~10
    int speed = 0;                    // 运动速度（预定义档位），单位 0.00367 rad/s
    int last_count = 0;               // 指令存在时间，单位：毫秒
    int value = 0;                    // 碰撞检测灵敏度
    int direction = 0;                // 运动方向：1 正方向，-1 负方向
    int moving_type = 0;              // 笛卡尔运动方式 0~5（0~2 平动，3~5 旋转）
    int coordinate = 0;               // 参考坐标系：0-世界坐标系，1-工具坐标系
    int open_tg = 0;                  // DragFtInCartesian 使用：0 直接力控拖拽，1 配合 MoveBlend
};

/**
 * @brief 电流环拖动参数，默认值与 DragInCST_SDK 示例 main 对齐
 */
struct DragInCSTParams {
    std::vector<int> cbfSwitch = { 0 };          // 电流环反馈开关
    double cutoff_freq = 20.0;                   // 截止频率
    std::vector<double> gravity_cof = { 1.0 };   // 重力补偿系数
    int ping_pong = 100;                         // 乒乓周期
    double ping_pong_amp = 0.5;                  // 乒乓幅值
    std::vector<double> vel_limit = { 0.1 };     // 速度限制
    std::vector<double> zero_check = { 0.0001 }; // 零速检查阈值
};

/**
 * @brief 命令类，封装与机器人控制器的 RPC 通信及命令发送
 */
class Command
{
    // 请求 ID 生成器，用于异步调用
    class RequestIDGenerator {
        std::atomic<int64_t> next_id_{ 1 };
    public:
        int64_t next() {
            int64_t id = next_id_.fetch_add(1, std::memory_order_relaxed);
            if (id <= 0) {
                next_id_ = 1;
                id = 1;
            }
            return id;
        }
    };

public:
    /**
     * @brief 构造函数
     * @param server_ip   RPC 服务器 IP 地址，默认 "192.168.2.37"
     * @param server_port RPC 服务器端口，默认 5868
     * @param auto_connect 是否在构造时自动连接服务器
     */
    explicit Command(const std::string& server_ip = "192.168.2.37",
        int server_port = 5868, bool auto_connect = true);
    ~Command();

    // ====================== 连接管理 ======================
    /**
     * @brief 连接到 RPC 服务器
     * @param ip  服务器 IP，为空则使用构造时指定的 IP
     * @param port 服务器端口，为 0 则使用构造时指定的端口
     * @return true-连接成功，false-连接失败
     */
    bool connect(const std::string& ip = "", int port = 0);

    /**
     * @brief 断开与服务器的连接
     */
    void disconnect();

    /**
     * @brief 检查当前是否已连接
     * @return true-已连接，false-未连接
     */
    bool is_connected() const;

    // ====================== 具体命令函数 ======================
    // ******** 基本命令 ******** //
    /**
     * @brief 使能机器人
     * @return true-发送成功，false-发送失败
     */
    bool Enable();

    /**
     * @brief 下使能机器人
     * @return true-发送成功，false-发送失败
     */
    bool Disable();

    /**
     * @brief 清除错误。清除异常等级为 ERROR 的异常，恢复系统到正常状态
     * @return true-发送成功，false-发送失败
     */
    bool Clear();

    /**
     * @brief 同步状态。与实机位置不匹配时使用此指令，常用于 Clear 之后
     * @return true-发送成功，false-发送失败
     */
    bool Recover();

    /**
     * @brief 将被暂停或停止的运动重新启动
     * @return true-发送成功，false-发送失败
     */
    bool Start();

    /**
     * @brief 将被暂停或停止的运动重新启动
     * @param last_count 指令存在时间（毫秒）；<=0 时发送不带参数的 {Start}
     * @return true-发送成功，false-发送失败
     */
    bool Start(int last_count);

    /**
     * @brief 在独立线程中发送 Start，不阻塞调用线程
     * @param last_count 指令存在时间（毫秒）；<=0 时发送不带参数的 {Start}
     * @param timeout_ms RPC 超时时间（毫秒）
     * @return future<bool>，true 表示指令已提交发送；需保持 Command 对象存活
     */
    std::future<bool> StartInThread(int last_count = 0, int timeout_ms = 10000);

    /**
     * @brief 停止运动，并跳出该指令。后续缓冲指令均不执行
     * @return true-发送成功，false-发送失败
     */
    bool Stop();

    /**
     * @brief 停止运动，并跳出该指令。后续缓冲指令均不执行
     * @param last_count 指令存在时间（毫秒）；<=0 时发送不带参数的 {Stop}
     * @return true-发送成功，false-发送失败
     */
    bool Stop(int last_count);

    /**
     * @brief 在独立线程中发送 Stop，不阻塞调用线程
     * @param last_count 指令存在时间（毫秒）；<=0 时发送不带参数的 {Stop}
     * @param timeout_ms RPC 超时时间（毫秒）
     * @return future<bool>，true 表示指令已提交发送；需保持 Command 对象存活
     */
    std::future<bool> StopInThread(int last_count = 0, int timeout_ms = 10000);

    /**
     * @brief 等待指定控制周期数，常用于 Stop 后等待停止生效
     * @param count 等待周期数
     * @return true-发送成功，false-发送失败
     */
    bool Sleep(int count);

    /**
     * @brief 获取默认的速度前馈系数（vf）和阻尼系数（cf）
     * @return true-发送成功，false-发送失败
     */
    bool GetDefaultVfCf();

    /**
     * @brief 删除已记录的运动轨迹数据
     * @return true-发送成功，false-发送失败
     */
    bool DeleteMotionRecord();

    /**
     * @brief 转换为转矩模式（CST）
     * @param all 是否切换全部轴，默认 true，与 DragInCST_SDK 示例对齐
     * @param max_torque 最大力矩限制，默认 1000
     * @return true-发送成功，false-发送失败
     */
    bool SwitchToCST(bool all = true, int max_torque = 1000);

    /**
     * @brief 转换为位置模式（CSP）
     * @param all 是否切换全部轴，默认 true，与 DragInCST_SDK 示例对齐
     * @param max_torque 最大力矩限制，默认 1000
     * @return true-发送成功，false-发送失败
     */
    bool SwitchToCSP(bool all = true, int max_torque = 1000);

    /**
     * @brief 力传感器复位/清零（仅实机有效，仿真无效）
     * @return true-发送成功，false-发送失败
     */
    bool ResetFtSensor();

    // ******** 变量相关命令 ******** //
    /**
     * @brief 设置变量（通用函数）
     * @param var_type     变量类型：tool、jointtarget、robottarget、clear 等
     * @param target_name  目标点名称
     * @param position     三维位置向量 [x,y,z]，单位米，默认 {0,0,0}
     * @param quaternion   四元数姿态 [qx,qy,qz,qw]，默认 {0,0,0,1}
     * @param joint_angles 关节角度向量（长度10），单位弧度，默认空
     * @return true-发送成功，false-发送失败
     */
    bool setVar(const std::string& var_type, const std::string& target_name,
        const std::vector<double>& position = { 0.0, 0.0, 0.0 },
        const std::vector<double>& quaternion = { 0.0, 0.0, 0.0, 1.0 },
        const std::vector<double>& joint_angles = {});

    /**
     * @brief 添加当前点位为示教点
     * @param name       示教点名称
     * @robottarget      示教点位置
     * @return true-发送成功，false-发送失败
     */
    bool TeachPoint(const std::string& name, const std::vector<double>& robottarget);

    /**
     * @brief 设置关节目标变量。需先调用此指令再调用运动指令 MoveAbsj
     * @param target_name  目标名称
     * @param joint_angles 关节角度向量（长度10），单位弧度
     * @return true-发送成功，false-发送失败
     */
    bool setJointTargetVar(const std::string& target_name, const std::vector<double>& joint_angles);

    /**
     * @brief 设置机器人目标变量（笛卡尔空间）。需先调用此指令再调用运动指令
     * @param target_name 目标名称
     * @param position    三维位置 [x,y,z]，单位米，默认 {0.5,0,0.3}
     * @param quaternion  四元数姿态 [qx,qy,qz,qw]，默认 {0,0,0,1}
     * @return true-发送成功，false-发送失败
     */
    bool setRobotTargetVar(const std::string& target_name,
        const std::vector<double>& position = { 0.5, 0.0, 0.3 },
        const std::vector<double>& quaternion = { 0.0, 0.0, 0.0, 1.0 });

    /**
     * @brief 设置工具变量
     * @param target_name 工具名称
     * @param position    工具坐标系原点位置，默认 {0,0,0.295}
     * @param quaternion  工具坐标系姿态，默认 {0,0,0,1}
     * @return true-发送成功，false-发送失败
     */
    bool setToolVar(const std::string& target_name,
        const std::vector<double>& position = { 0.0, 0.0, 0.295 },
        const std::vector<double>& quaternion = { 0.0, 0.0, 0.0, 1.0 });

    /**
     * @brief 设置工件变量
     * @param target_name 工件名称
     * @param position    工件坐标系原点位置，默认 {0,0,0}
     * @param quaternion  工件坐标系姿态，默认 {0,0,0,1}
     * @return true-发送成功，false-发送失败
     */
    bool setWobjVar(const std::string& target_name,
        const std::vector<double>& position = { 0.0, 0.0, 0.0 },
        const std::vector<double>& quaternion = { 0.0, 0.0, 0.0, 1.0 });

    /**
     * @brief 删除通过 Var 指令定义的临时变量
     * @param var_name 要删除的变量名称
     * @return true-发送成功，false-发送失败
     */
    bool DeleteVar(const std::string& var_name);

    /**
     * @brief 清除所有通过 Var 指令定义的临时变量
     * @return true-发送成功，false-发送失败
     */
    bool ClearAllVar();

    /**
     * @brief 设置末端工具坐标系
     * @param tool_name 工具名称，必须是已定义的工具变量
     * @return true-发送成功，false-发送失败
     */
    bool SetTool(const std::string& tool_name);

    /**
     * @brief 设置末端工件坐标系
     * @param wobj_name 工件名称，必须是已定义的工件变量
     * @return true-发送成功，false-发送失败
     */
    bool SetWobj(const std::string& wobj_name);

    /**
     * @brief 设置碰撞检测灵敏度
     * @param value 灵敏度值
     * @return true-发送成功，false-发送失败
     */
    bool SetSystemConf(int value);

    // ******** 运动命令 ******** //
    /**
     * @brief 三指夹爪运动指令（阻塞式，仅实机有效）
     * @param distance 九个电机的关节角度，单位度，默认全0
     * @param interval 执行间隔（毫秒），默认 25ms
     * @return true-发送成功，false-发送失败
     */
    bool MoveThreeFingersEcatCTEK(const std::vector<double>& distance, int interval);

    /**
     * @brief 关节空间点动
     * @param motion_id  电机ID
     * @param speed      运动速度档位
     * @param direction  运动方向：1-正方向，-1-负方向
     * @param last_count 指令持续时间（毫秒）
     * @return true-发送成功，false-发送失败
     */
    bool JogJoint(const std::string& motion_id, int speed, int direction, int last_count);

    /**
     * @brief 笛卡尔点动（位置+姿态）
     * @param moving_type 运动方式 0~5（0~2 平动，3~5 旋转）
     * @param speed       运动速度档位
     * @param direction   运动方向：1-正，-1-负
     * @param coordinate  参考坐标系：0-世界，1-工具
     * @param last_count  指令持续时间（毫秒）
     * @return true-发送成功，false-发送失败
     */
    bool JogCartesian(int moving_type, int speed, int direction, int coordinate, int last_count);

    /**
     * @brief 任意关节空间点动（J3 系列）
     * @param joint_pos  期望关节位置（长度10）
     * @param joint_vel  旋量速度 [x,y,z,rx,ry,rz]
     * @param joint_acc  旋量加速度
     * @param joint_dec  旋量减速度
     * @param last_count 指令持续时间（毫秒）
     * @return true-发送成功，false-发送失败
     */
    bool JogAnyJ3(const std::vector<double>& joint_pos, const std::vector<double>& joint_vel,
        const std::vector<double>& joint_acc, const std::vector<double>& joint_dec, int last_count, int wait_time);

    /**
     * @brief 笛卡尔空间力控拖拽（仅实机有效，异步，需用 Stop 终止）
     * @param force_direction 允许拖动的自由度，1启用，0禁用（6维）
     * @param force_target    期望力/力矩设定 {Fx,Fy,Fz,Tx,Ty,Tz}，单位 N
     * @param open_tg         0-直接力控拖拽，1-配合 MoveBlend
     * @return true-发送成功，false-发送失败
     */
    bool DragFtInCartesian(const std::vector<int>& force_direction,
        const std::vector<int>& force_target, int open_tg = 0, int wait_time = 0);

    /**
     * @brief 电流环拖动，参数与 DragInCST_SDK 示例 main 对齐
     * @param params 拖动参数，默认使用示例中的 cbfSwitch/cutoff_freq/gravity_cof 等配置
     * @return true-发送成功，false-发送失败
     */
    bool DragInCST(const DragInCSTParams& params = {});

    /**
     * @brief 打开电流环拖动：GetDefaultVfCf → DeleteMotionRecord → SwitchToCST → DragInCST
     * @param params 拖动参数，默认使用 DragInCST_SDK 示例配置
     * @return true-发送成功，false-发送失败
     */
    bool SetDragInCSTOn(const DragInCSTParams& params = {});

    /**
     * @brief 关闭电流环拖动并恢复位置模式：
     *        Stop → Sleep → SwitchToCSP → Recover → Start
     * @return true-发送成功，false-发送失败
     */
    bool SetDragInCSTOff();

    /**
     * @brief 笛卡尔在线速度控制（直线运动，异步）
     * @param vel        旋量速度 [x,y,z,rx,ry,rz]，单位 rad/s
     * @param acc        旋量加速度，单位 rad/s²
     * @param jerk       旋量加加速度，单位 rad/s³
     * @param coordinate 参考坐标系：0-世界，1-工具
     * @param last_count 指令持续时间（毫秒）
     * @param wait_time  异步等待时间（毫秒）
     * @return true-发送成功，false-发送失败
     */
    bool SpeedL(const std::vector<double>& vel, const std::vector<double>& acc,
        const std::vector<double>& jerk, int coordinate, int last_count, int wait_time);

    /**
     * @brief 绝对关节运动命令，运动到 setJointTargetVar 指定的位置
     * @param target_name 目标点名称
     * @param speed       运动速度档位
     * @return true-发送成功，false-发送失败
     */
    bool MoveAbsj(const std::string& target_name, int speed);

    /**
     * @brief 设置是否使用奇异点求解（开启后可过奇异点）
     * @param state "on" 启用，"off" 禁用
     * @return true-发送成功，false-发送失败
     */
    bool setUsingSP(const std::string& state = "on");

    /**
     * @brief 设置是否使用碰撞检测
     * @param state "on" 启用，"off" 禁用
     * @return true-发送成功，false-发送失败
     */
    bool setCollisionDetection(const std::string& state = "off");

    /**
     * @brief MoveBlend 序列首次插入命令
     * @return true-发送成功，false-发送失败
     */
    bool MoveBlendFirstInsert();

    /**
     * @brief 向混合运动序列中插入直线段
     * @param target_name 目标点名称
     * @param speed       运动速度档位
     * @param zone        平滑参数 [笛卡尔转弯区大小(mm), 百分比]
     * @param ratio       加速度/速度比率，范围 1~10
     * @return true-发送成功，false-发送失败
     */
    bool MoveBlendInsertLine(const std::string& target_name, int speed,
        const std::vector<double>& zone, double ratio);

    /**
     * @brief 向混合运动序列中插入圆弧段
     * @param mid_target_name 中间目标点名称
     * @param target_name     终点目标点名称
     * @param speed           运动速度档位
     * @param zone            平滑参数
     * @param ratio           加速度/速度比率
     * @return true-发送成功，false-发送失败
     */
    bool MoveBlendInsertCircle(const std::string& mid_target_name, const std::string& target_name,
        int speed, const std::vector<double>& zone, double ratio);

    /**
     * @brief 开始执行混合运动序列
     * @return true-发送成功，false-发送失败
     */
    bool MoveBlendStart();

    /**
     * @brief 执行一次完整的 MoveBlend 直线序列（插入 + 开始）
     * @param target_name 目标点名称
     * @param speed       运动速度档位
     * @param zone        平滑参数
     * @param ratio       加速度/速度比率
     * @return true-执行成功，false-执行失败
     */
    bool MoveBlendLine(const std::string& target_name, int speed,
        const std::vector<double>& zone, double ratio);

    /**
     * @brief 同步发送命令（阻塞等待完成）
     * @param cmds       命令字符串列表
     * @param timeout_ms 超时时间（毫秒）
     * @return true-发送成功，false-发送失败
     */
    bool sendSync(const std::vector<std::string>& cmds, int timeout_ms);

    /**
     * @brief 异步发送命令（不等待完成）
     * @param cmds       命令字符串列表
     * @param timeout_ms 超时时间（毫秒）
     * @return true-发送成功，false-发送失败
     */
    bool sendAsync(const std::vector<std::string>& cmds, int timeout_ms, int wait_time);

    cpp_rpc::CPPClient& getClient() { return *client_; }

protected:
    /**
     * @brief 发送命令的核心函数
     * @param params     命令参数结构体
     * @param timeout_ms 超时时间（毫秒）
     * @return true-发送成功，false-发送失败
     */
    bool sendCommand(const CommandParams& params, int timeout_ms = 10000, int wait_time = 0);

    /**
     * @brief 将 CommandParams 转换为命令字符串
     * @param params 命令参数
     * @return 格式化的命令字符串
     */
    std::string convertCommand(const CommandParams& params) const;

    /**
     * @brief 将 vector 转换为逗号分隔的字符串
     * @tparam T 元素类型
     * @param vec 输入 vector
     * @param sep 分隔符，默认为 ","
     * @return 拼接后的字符串
     */
    template<typename T>
    std::string vectorToString(const std::vector<T>& vec, const std::string& sep = ",") const;

    /**
     * @brief 格式化字符串模板替换
     * @param fmt    包含占位符的模板字符串
     * @param params 命令参数
     * @return 替换后的命令字符串
     */
    std::string formatString(const std::string& fmt, const CommandParams& params) const;

    // RPC 客户端相关
    std::unique_ptr<cpp_rpc::CPPClient> client_;   ///< RPC 客户端对象
    mutable std::mutex client_mutex_;               ///< 保护 client_ 的互斥锁
    std::atomic<bool> connected_{ false };          ///< 连接状态标志
    std::string server_ip_;                         ///< 服务器 IP 地址
    int server_port_;                               ///< 服务器端口
    RequestIDGenerator requestIDGenerator;          ///< 请求 ID 生成器

    // 命令模板映射
    std::unordered_map<std::string, std::string> command_templates_;  ///< 命令格式模板
};

#endif // COMMAND_HPP