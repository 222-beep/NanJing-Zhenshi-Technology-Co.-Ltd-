#include "command.hpp"
#include "message/resp_dto.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <future>

using namespace std::chrono_literals;

// 提取命令类型
static std::string extractCommandType(const std::string& cmd_str) {
    if (cmd_str.empty() || cmd_str[0] != '{') return "";
    size_t start = 1;
    size_t end = cmd_str.find_first_of(" \t\n-}", start);
    if (end == std::string::npos) {
        end = cmd_str.find('}', start);
        if (end == std::string::npos) return "";
    }
    return cmd_str.substr(start, end - start);
}

// 生成带可选 --last_count 的指令，如 {Stop} 或 {Stop --last_count=10}
static std::string makeLastCountCommand(const std::string& name, int last_count) {
    if (last_count > 0) {
        return "{" + name + " --last_count=" + std::to_string(last_count) + "}";
    }
    return "{" + name + "}";
}

// vectorToString 实现
template<typename T>
std::string Command::vectorToString(const std::vector<T>& vec, const std::string& sep) const {
    if (vec.empty()) return "";
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) oss << sep;
        oss << vec[i];
    }
    return oss.str();
}

// 模板实例化，根据需要添加
template std::string Command::vectorToString<double>(const std::vector<double>&, const std::string&) const;
template std::string Command::vectorToString<int>(const std::vector<int>&, const std::string&) const;

Command::Command(const std::string& server_ip, int server_port, bool auto_connect)
    : server_ip_(server_ip), server_port_(server_port)
{
    // 初始化命令模板映射
    command_templates_ = {
        {"Enable", "{Enable}"},
        {"Disable", "{Disable}"},
        {"Clear", "{Clear}"},
        {"Recover", "{Recover}"},
        {"Start", "{Start}"},
        {"Stop", "{Stop}"},
        {"GetDefaultVfCf", "{GetDefaultVfCf}"},
        {"DeleteMotionRecord", "{DeleteMotionRecord}"},
        {"SwitchToCST", "{SwitchToCST --all --max_torque=1000}"},
        {"SwitchToCSP", "{SwitchToCSP --all --max_torque=1000}"},
        {"ResetFtSensor", "{ResetFtSensor}"},
        {"DeleteVar", "{DeleteVar --name={target_name}}"},
        {"SetTool", "{SetTool --tool_var={target_name}}"},
        {"SetWobj", "{SetWobj --wobj_var={target_name}}"},
        {"SetSystemConf", "{SetSystemConf --motion_id=3 --name=max_pos --value={value}}"},
        {"MoveAbsj", "{MoveAbsJ --jointtarget_var={target_name} --speed=v{speed}}"},
        {"JogJoint", "{JogJoint --speed=v{speed} --motion_id={motion_id} --increase_count={last_count} --direction={direction}}"},
        {"JogAnyJ3", "{JogAnyJ3 --joint_pos={{joint_angles}} --joint_vel={{vel}} --joint_acc={{acc}} --joint_dec={{dec}} --last_count={last_count}}"},
        {"JogCartesian", "{JogCartesian --moving_type={moving_type} --direction={direction} --speed=v{speed} --coordinate={coordinate} --increase_count={last_count}}"},
        {"SpeedL", "{SpeedL --vel={{{vel}}} --acc={{{acc}}} --jerk={{{jerk}}} --last_count={last_count} --coordinate={coordinate}}"},
        {"MoveThreeFingersEcatCTEK", "{MoveThreeFingersEcatCTEK --distance={{{distance}}} --interval={last_count}}"},
        {"DragFtInCartesian", "{DragFtInCartesian --force_target={{{force_target}}} --force_direction={{{force_direction}}} --open_tg={open_tg}}"},
        {"SetUsingSP", "{SetUsingSP --state={var_type}}"},
        {"SetCollisionDetection", "{SetCollisionDetection --state={var_type}}"},
    };

    if (auto_connect) {
        connect();
    }
}

Command::~Command()
{
    disconnect();
}

bool Command::connect(const std::string& ip, int port)
{
    std::lock_guard<std::mutex> lock(client_mutex_);

    std::string target_ip = ip.empty() ? server_ip_ : ip;
    int target_port = (port <= 0) ? server_port_ : port;

    if (client_) {
        try {
            client_.reset();
        }
        catch (...) {}
    }

    try {
        client_ = std::make_unique<cpp_rpc::CPPClient>(target_ip, target_port, 2000, nullptr);
        
        core::Msg test_msg("{Clear}");
        test_msg.setMsgID(10001);
        test_msg.setMsgSeqID(requestIDGenerator.next());
        auto result = client_->CallAwait<RespDemo>(test_msg, 10000);

        if (result.first >= 0) {
            std::cout << "Connected to " << target_ip << ":" << target_port << std::endl;
            connected_.store(true);
            server_ip_ = target_ip;
            server_port_ = target_port;
            return true;
        }
        else {
            std::cerr << "Connection test failed with code: " << result.first << std::endl;
            client_.reset();
            connected_.store(false);
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Connection exception: " << e.what() << std::endl;
        client_.reset();
        connected_.store(false);
        return false;
    }
}

void Command::disconnect()
{
    std::lock_guard<std::mutex> lock(client_mutex_);
    if (client_) {
        try {
            client_.reset();
        }
        catch (...) {
            client_.reset();
        }
    }
    connected_.store(false);
    std::cout << "Disconnected." << std::endl;
}

bool Command::is_connected() const
{
    return connected_.load() && client_ != nullptr;
}

std::string Command::formatString(const std::string& fmt, const CommandParams& params) const
{
    std::string result = fmt;

    auto replace = [&](const std::string& key, const std::string& value) {
        size_t pos = 0;
        std::string placeholder = "{" + key + "}";
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    };

    replace("command_type", params.command_type);
    replace("var_type", params.var_type);
    replace("target_name", params.target_name);
    replace("motion_id", params.motion_id);
    replace("mid_target_name", params.mid_target_name);
    replace("speed", std::to_string(params.speed));
    replace("last_count", std::to_string(params.last_count));
    replace("value", std::to_string(params.value));
    replace("direction", std::to_string(params.direction));
    replace("moving_type", std::to_string(params.moving_type));
    replace("coordinate", std::to_string(params.coordinate));
    replace("ratio", std::to_string(params.ratio));
    replace("open_tg", std::to_string(params.open_tg));

    replace("position", vectorToString(params.position));
    replace("quaternion", vectorToString(params.quaternion));
    replace("joint_angles", vectorToString(params.joint_angles));
    replace("distance", vectorToString(params.distance));
    replace("zone", vectorToString(params.zone));
    replace("vel", vectorToString(params.vel));
    replace("acc", vectorToString(params.acc));
    replace("dec", vectorToString(params.dec));
    replace("jerk", vectorToString(params.jerk));
    replace("cf_coef", vectorToString(params.cf_coef));
    replace("vf_coef", vectorToString(params.vf_coef));
    replace("force_direction", vectorToString(params.force_direction));
    replace("force_target", vectorToString(params.force_target));

    return result;
}

std::string Command::convertCommand(const CommandParams& params) const
{
    // 特殊处理 Var
    if (params.command_type == "Var") {
        std::ostringstream oss;
        if (params.var_type == "tool" || params.var_type == "wobj" || params.var_type == "robottarget") {
            std::vector<double> pos = params.position;
            std::vector<double> quat = params.quaternion;
            if (pos.size() < 3) pos.resize(3, 0.0);
            if (quat.size() < 4) quat.resize(4, 0.0);
            std::vector<double> combined;
            combined.reserve(7);
            combined.insert(combined.end(), pos.begin(), pos.begin() + 3);
            combined.insert(combined.end(), quat.begin(), quat.begin() + 4);
            oss << "{Var --type=" << params.var_type << " --name=" << params.target_name
                << " --value={" << vectorToString(combined) << "}}";
        }
        else if (params.var_type == "jointtarget") {
            oss << "{Var --type=jointtarget --name=" << params.target_name
                << " --value={" << vectorToString(params.joint_angles) << "}}";
        }
        else if (params.var_type == "clear") {
            oss << "{Var --clear}";
        }
        else {
            oss << "{Var}";
        }
        return oss.str();
    }

    // 特殊处理 MoveBlend
    if (params.command_type == "MoveBlend") {
        std::ostringstream oss;
        if (params.var_type == "first_insert") {
            oss << "{MoveBlend --type=first_insert}";
        }
        else if (params.var_type == "insert_line") {
            oss << "{MoveBlend --type=insert_line --robottarget_var=" << params.target_name;
            if (params.speed > 0) oss << " --speed=v" << params.speed;
            if (!params.zone.empty()) {
                oss << " --zone={" << params.zone[0] << "," << params.zone[1] << "}";
            }
            if (params.ratio > 0) oss << " --ratio=" << params.ratio;
            oss << "}";
        }
        else if (params.var_type == "insert_circle") {
            oss << "{MoveBlend --type=insert_circle --mid_robottarget_var=" << params.mid_target_name
                << " --robottarget_var=" << params.target_name;
            if (params.speed > 0) oss << " --speed=v" << params.speed;
            if (!params.zone.empty()) {
                oss << " --zone={" << params.zone[0] << "," << params.zone[1] << "}";
            }
            if (params.ratio > 0) oss << " --ratio=" << params.ratio;
            oss << "}";
        }
        else if (params.var_type == "start") {
            oss << "{MoveBlend --type=start}";
        }
        return oss.str();
    }

    // 普通命令
    auto it = command_templates_.find(params.command_type);
    if (it != command_templates_.end()) {
        return formatString(it->second, params);
    }

    // 备选：raw_command
    if (!params.raw_command.empty()) {
        return params.raw_command;
    }

    std::cerr << "Unknown command type: " << params.command_type << std::endl;
    return "";
}


bool Command::sendSync(const std::vector<std::string>& cmds, int timeout_ms)
{
    for (const auto& cmd : cmds) {
        if (!client_ || !connected_.load()) {
            std::cerr << "Not connected, attempting to reconnect..." << std::endl;
            if (!connect(server_ip_, server_port_)) {
                std::cerr << "Reconnection failed." << std::endl;
                return false;
            }
        }

        try {
            core::Msg msg(cmd);
            msg.setMsgID(10001);
            msg.setMsgSeqID(requestIDGenerator.next());

            std::cout << "***Sending (sync): " << cmd << " (timeout: " << timeout_ms << "ms)" << std::endl;

            auto result = client_->CallAwait<RespDemo>(msg, timeout_ms);

            if (result.first == 0 && !result.second.empty()) {
                bool all_ok = true;
                for (const auto& r : result.second) {
                    if (r.return_code < 0) {
                        all_ok = false;
                    }
                    std::cout << "subcmd_index:" << r.subcmd_index << std::endl;
                    std::cout << "return_code:" << r.return_code << std::endl;
                    std::cout << "return_message:" << r.return_message << std::endl;
                }
                std::cout << "************************************" << std::endl;
                if (!all_ok) return false;
            }
            else {
                std::cerr << "RPC call failed with code " << result.first << std::endl;
                if (result.first == cpp_rpc::RPC_DISCONNECTED_CODE || !client_->IsConnected()) {
                    connected_.store(false);
                }
                return false;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during RPC call: " << e.what() << std::endl;
            connected_.store(false);
            return false;
        }
    }
    return true;
}

bool Command::sendAsync(const std::vector<std::string>& cmds, int timeout_ms, int wait_time)
{
    if (!client_ || !connected_.load()) {
        std::cerr << "Not connected, attempting to reconnect..." << std::endl;
        if (!connect(server_ip_, server_port_)) {
            std::cerr << "Reconnection failed." << std::endl;
            return false;
        }
    }

    bool all_sent = true;
    for (const auto& cmd : cmds) {
        try {
            core::Msg msg(cmd);
            msg.setMsgID(10001);
            msg.setMsgSeqID(requestIDGenerator.next());
            std::cout << "***Sending async command: " << cmd << std::endl;

            const bool sent = client_->CallAsync<RespDemo>(msg, timeout_ms,
                [cmd](int ret, std::vector<RespDemo> res) {
                    if (ret < 0) {
                        std::cerr << "Async command failed with code " << ret << ": " << cmd << std::endl;
                    }
                    else {
                        for (const auto& r : res) {
                            std::cout << "asubcmd_index:" << r.subcmd_index << std::endl;
                            std::cout << "areturn_code:" << r.return_code << std::endl;
                            std::cout << "areturn_message:" << r.return_message << std::endl;
                        }
                    }
                    std::cout << "************************************" << std::endl;
                });
            if (!sent) {
                std::cerr << "Failed to send async command: " << cmd << std::endl;
                all_sent = false;
                continue;
            }

            // 等待下一条异步指令，避免连续发送时相互覆盖。
            delay_ms(wait_time);
        }
        catch (const std::exception& e) {
            std::cerr << "Exception sending async command: " << e.what() << std::endl;
            all_sent = false;
        }
    }
    return all_sent;
}

bool Command::sendCommand(const CommandParams& params, int timeout_ms, int wait_time)
{
    std::string cmd_str = convertCommand(params);
    if (cmd_str.empty()) {
        std::cerr << "Command conversion failed." << std::endl;
        return false;
    }

    std::string cmd_type = extractCommandType(cmd_str);
    if (cmd_type == "DragFtInCartesian" || cmd_type == "SpeedL" || cmd_type == "JogAnyJ3") {
        return sendAsync({ cmd_str }, timeout_ms, wait_time);
    }
    else {
        return sendSync({ cmd_str }, timeout_ms);
    }
}


// ====================== 具体命令实现 ======================
bool Command::Enable()
{
    std::cout << "====== Sending Enable command ======" << std::endl;
    CommandParams params;
    params.command_type = "Enable";
    return sendCommand(params, 5000);
}

bool Command::Disable()
{
    std::cout << "====== Sending Disable command ======" << std::endl;
    CommandParams params;
    params.command_type = "Disable";
    return sendCommand(params, 5000);
}

bool Command::Clear()
{
    std::cout << "====== Sending Clear command ======" << std::endl;
    CommandParams params;
    params.command_type = "Clear";
    return sendCommand(params, 5000);
}

bool Command::Recover()
{
    std::cout << "====== Sending Recover command ======" << std::endl;
    CommandParams params;
    params.command_type = "Recover";
    return sendCommand(params, 5000);
}

bool Command::Start()
{
    return Start(0);
}

bool Command::Start(int last_count)
{
    std::cout << "====== Sending Start command ======" << std::endl;
    return sendSync({ makeLastCountCommand("Start", last_count) }, 5000);
}

std::future<bool> Command::StartInThread(int last_count, int timeout_ms)
{
    std::lock_guard<std::mutex> lock(client_mutex_);
    if (!client_) {
        std::cerr << "Not connected, Start not sent." << std::endl;
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }
    return send_rpc_thread(*client_, makeLastCountCommand("Start", last_count), false, timeout_ms);
}

bool Command::Stop()
{
    return Stop(0);
}

bool Command::Stop(int last_count)
{
    std::cout << "====== Sending Stop command ======" << std::endl;
    return sendSync({ makeLastCountCommand("Stop", last_count) }, 5000);
}

std::future<bool> Command::StopInThread(int last_count, int timeout_ms)
{
    std::lock_guard<std::mutex> lock(client_mutex_);
    if (!client_) {
        std::cerr << "Not connected, Stop not sent." << std::endl;
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }
    return send_rpc_thread(*client_, makeLastCountCommand("Stop", last_count), false, timeout_ms);
}

bool Command::Sleep(int count)
{
    std::cout << "====== Sending Sleep command ======" << std::endl;
    return sendSync({ "{Sleep --count=" + std::to_string(count) + "}" }, 5000);
}

bool Command::GetDefaultVfCf()
{
    std::cout << "====== Sending GetDefaultVfCf command ======" << std::endl;
    CommandParams params;
    params.command_type = "GetDefaultVfCf";
    return sendCommand(params, 5000);
}

bool Command::DeleteMotionRecord()
{
    std::cout << "====== Sending DeleteMotionRecord command ======" << std::endl;
    CommandParams params;
    params.command_type = "DeleteMotionRecord";
    return sendCommand(params, 5000);
}

bool Command::SwitchToCST(bool all, int max_torque)
{
    std::cout << "====== Sending SwitchToCST command ======" << std::endl;
    std::string cmd = "{SwitchToCST";
    if (all) cmd += " --all";
    cmd += " --max_torque=" + std::to_string(max_torque) + "}";
    return sendSync({ cmd }, 5000);
}

bool Command::SwitchToCSP(bool all, int max_torque)
{
    std::cout << "====== Sending SwitchToCSP command ======" << std::endl;
    std::string cmd = "{SwitchToCSP";
    if (all) cmd += " --all";
    cmd += " --max_torque=" + std::to_string(max_torque) + "}";
    return sendSync({ cmd }, 5000);
}

bool Command::ResetFtSensor()
{
    std::cout << "====== Sending ResetFtSensor command ======" << std::endl;
    CommandParams params;
    params.command_type = "ResetFtSensor";
    return sendCommand(params, 5000);
}

bool Command::setVar(const std::string& var_type, const std::string& target_name,
    const std::vector<double>& position,
    const std::vector<double>& quaternion,
    const std::vector<double>& joint_angles)
{
    std::cout << "====== Sending Var command ======" << std::endl;
    std::cout << "Type: " << var_type << ", Name: " << target_name << std::endl;
    CommandParams params;
    params.command_type = "Var";
    params.var_type = var_type;
    params.target_name = target_name;
    params.position = position;
    params.quaternion = quaternion;
    params.joint_angles = joint_angles;
    return sendCommand(params, 5000);
}

bool Command::setJointTargetVar(const std::string& target_name, const std::vector<double>& joint_angles)
{
    std::vector<double> angles = joint_angles;
    if (angles.size() < 10) {
        std::cerr << "Joint angles less than 10, auto-filling..." << std::endl;
        angles.resize(10, 0.0);
    }
    return setVar("jointtarget", target_name, {}, {}, angles);
}

bool Command::setRobotTargetVar(const std::string& target_name,
    const std::vector<double>& position,
    const std::vector<double>& quaternion)
{
    return setVar("robottarget", target_name, position, quaternion);
}

bool Command::setToolVar(const std::string& target_name,
    const std::vector<double>& position,
    const std::vector<double>& quaternion)
{
    return setVar("tool", target_name, position, quaternion);
}

bool Command::setWobjVar(const std::string& target_name,
    const std::vector<double>& position,
    const std::vector<double>& quaternion)
{
    return setVar("wobj", target_name, position, quaternion);
}

bool Command::TeachPoint(const std::string& name, const std::vector<double>& robottarget)
{
    std::cout << "====== Sending TeachPoint command ======" << std::endl;

    std::string cmd1 = "{TeachPoint --name=" + name + "}";
    if (!sendSync({ cmd1 }, 5000)) {
        std::cerr << "TeachPoint first insert failed, set aborted" << std::endl;
        return false;
    }

    std::string cmd2 = "{EditPoint --name=" + name + " --end_pq={" + vectorToString(robottarget) + "} --joint_pos={0,0,0,0,0,0,0,0,0,0} --new_name=" + name + "}";
    if (!sendSync({ cmd2 }, 5000)) {
        std::cerr << "TeachPoint redefine insert failed, set aborted" << std::endl;
        return false;
    }

    std::cout << "TeachPoint set completed" << std::endl;
    return true;
}

bool Command::DeleteVar(const std::string& var_name)
{
    std::cout << "====== Sending DeleteVar command ======" << std::endl;
    std::cerr << "Deleting variable: " << var_name << std::endl;
    CommandParams params;
    params.command_type = "DeleteVar";
    params.target_name = var_name;
    return sendCommand(params, 5000);
}

bool Command::ClearAllVar()
{
    std::cout << "====== Sending ClearAllVar command ======" << std::endl;
    std::cerr << "Delete all variables" << std::endl;
    CommandParams params;
    params.command_type = "Var";
    params.var_type = "clear";
    return sendCommand(params, 5000);
}

bool Command::SetTool(const std::string& tool_name)
{
    std::cout << "====== Sending SetTool command ======" << std::endl;
    CommandParams params;
    params.command_type = "SetTool";
    params.target_name = tool_name;
    return sendCommand(params, 5000);
}

bool Command::SetWobj(const std::string& wobj_name)
{
    std::cout << "====== Sending SetWobj command ======" << std::endl;
    CommandParams params;
    params.command_type = "SetWobj";
    params.target_name = wobj_name;
    return sendCommand(params, 5000);
}

bool Command::SetSystemConf(int value)
{
    std::cout << "====== Sending SetSystemConf command ======" << std::endl;
    std::cout << "value: " << value << std::endl;
    CommandParams params;
    params.command_type = "SetSystemConf";
    params.value = value;
    return sendCommand(params, 5000);
}

bool Command::MoveThreeFingersEcatCTEK(const std::vector<double>& distance, int interval)
{
    std::cout << "====== Sending MoveThreeFingersEcatCTEK command ======" << std::endl;
    CommandParams params;
    params.command_type = "MoveThreeFingersEcatCTEK";
    params.distance = distance;
    params.last_count = interval;
    return sendCommand(params, 5000);
}

bool Command::JogJoint(const std::string& motion_id, int speed, int direction, int last_count)
{
    std::cout << "====== Sending JogJoint command ======" << std::endl;
    std::cout << "motion_id: " << motion_id << ", speed: " << speed
        << ", direction: " << direction << ", last_count: " << last_count << std::endl;
    CommandParams params;
    params.command_type = "JogJoint";
    params.motion_id = motion_id;
    params.speed = speed;
    params.direction = direction;
    params.last_count = last_count;
    return sendCommand(params, 5000);
}

bool Command::JogAnyJ3(const std::vector<double>& joint_pos, const std::vector<double>& joint_vel,
                       const std::vector<double>& joint_acc, const std::vector<double>& joint_dec, int last_count, int wait_time)
{
    std::cout << "====== Sending JogAnyJ3 command ======" << std::endl;
    CommandParams params;
    params.command_type = "JogAnyJ3";
    params.joint_angles = joint_pos;
    params.vel = joint_vel;
    params.acc = joint_acc;
    params.dec = joint_dec;
    params.last_count = last_count;
    return sendCommand(params, 5000, wait_time);
}

bool Command::JogCartesian(int moving_type, int speed, int direction, int coordinate, int last_count)
{
    std::cout << "====== Sending JogCartesian command ======" << std::endl;
    std::cout << "moving_type: " << moving_type << ", speed: " << speed
        << ", direction: " << direction << ", coordinate: " << coordinate
        << ", last_count: " << last_count << std::endl;
    CommandParams params;
    params.command_type = "JogCartesian";
    params.moving_type = moving_type;
    params.speed = speed;
    params.direction = direction;
    params.coordinate = coordinate;
    params.last_count = last_count;
    return sendCommand(params, 5000);
}

bool Command::DragFtInCartesian(const std::vector<int>& force_direction,
    const std::vector<int>& force_target, int open_tg, int wait_time)
{
    std::cout << "====== Sending DragFtInCartesian command ======" << std::endl;
    CommandParams params;
    params.command_type = "DragFtInCartesian";
    params.force_direction = force_direction;
    params.force_target = force_target;
    params.open_tg = open_tg;
    return sendCommand(params, 86400000, wait_time);
}

bool Command::DragInCST(const DragInCSTParams& params)
{
    std::cout << "====== Sending DragInCST command ======" << std::endl;
    std::ostringstream oss;
    oss << "{DragInCST --cbfSwitch={" << vectorToString(params.cbfSwitch)
        << "} --cutoff_freq=" << params.cutoff_freq
        << " --gravity_cof={" << vectorToString(params.gravity_cof)
        << "} --ping_pong=" << params.ping_pong
        << " --ping_pong_amp=" << params.ping_pong_amp
        << " --vel_limit={" << vectorToString(params.vel_limit)
        << "} --zero_check={" << vectorToString(params.zero_check) << "}}";
    // DragInCST 只需同步发送一次，控制器会持续保持拖拽状态。
    return sendSync({ oss.str() }, 10000);
}

bool Command::SetDragInCSTOn(const DragInCSTParams& params)
{
    std::cout << "====== Sending SetDragInCSTOn command ======" << std::endl;
    if (!GetDefaultVfCf()) {
        std::cerr << "GetDefaultVfCf failed, sequence aborted" << std::endl;
        return false;
    }
    if (!DeleteMotionRecord()) {
        std::cerr << "DeleteMotionRecord failed, sequence aborted" << std::endl;
        return false;
    }
    if (!SwitchToCST()) {
        std::cerr << "SwitchToCST failed, sequence aborted" << std::endl;
        return false;
    }
    if (!DragInCST(params)) {
        std::cerr << "DragInCST failed, sequence aborted" << std::endl;
        return false;
    }
    std::cout << "SetDragInCSTOn sequence completed" << std::endl;
    return true;
}

bool Command::SetDragInCSTOff()
{
    std::cout << "====== Sending SetDragInCSTOff command ======" << std::endl;
    if (!Stop(10)) {
        std::cerr << "Stop failed, sequence aborted" << std::endl;
        return false;
    }
    if (!Sleep(20)) {
        std::cerr << "Sleep failed, sequence aborted" << std::endl;
        return false;
    }
    if (!SwitchToCSP()) {
        std::cerr << "SwitchToCSP failed, sequence aborted" << std::endl;
        return false;
    }
    if (!Recover()) {
        std::cerr << "Recover failed, sequence aborted" << std::endl;
        return false;
    }
    if (!Start(50)) {
        std::cerr << "Start failed, sequence aborted" << std::endl;
        return false;
    }
    std::cout << "SetDragInCSTOff sequence completed" << std::endl;
    return true;
}

bool Command::SpeedL(const std::vector<double>& vel, const std::vector<double>& acc,
    const std::vector<double>& jerk, int coordinate, int last_count, int wait_time)
{
    std::cout << "====== Sending SpeedL command ======" << std::endl;
    CommandParams params;
    params.command_type = "SpeedL";
    params.vel = vel;
    params.acc = acc;
    params.jerk = jerk;
    params.coordinate = coordinate;
    params.last_count = last_count;
    return sendCommand(params, 30000, wait_time);
}

bool Command::MoveAbsj(const std::string& target_name, int speed)
{
    std::cout << "====== Sending MoveAbsj command ======" << std::endl;
    CommandParams params;
    params.command_type = "MoveAbsj";
    params.target_name = target_name;
    params.speed = speed;
    return sendCommand(params, 60000);
}

bool Command::setUsingSP(const std::string& state)
{
    std::cout << "====== Sending SetUsingSP command ======" << std::endl;
    std::cout << "Set Singularity point solving: " << state << std::endl;
    CommandParams params;
    params.command_type = "SetUsingSP";
    params.var_type = state;
    return sendCommand(params, 5000);
}

bool Command::setCollisionDetection(const std::string& state)
{
    std::cout << "====== Sending setCollisionDetection command ======" << std::endl;
    std::cout << "Set collision detection: " << state << std::endl;
    CommandParams params;
    params.command_type = "SetCollisionDetection";
    params.var_type = state;
    return sendCommand(params, 5000);
}

bool Command::MoveBlendFirstInsert()
{
    std::cout << "====== Sending MoveBlend first insert command ======" << std::endl;
    CommandParams params;
    params.command_type = "MoveBlend";
    params.var_type = "first_insert";
    return sendCommand(params, 5000);
}

bool Command::MoveBlendInsertLine(const std::string& target_name, int speed,
    const std::vector<double>& zone, double ratio)
{
    std::cout << "====== Sending MoveBlend line insert command ======" << std::endl;
    CommandParams params;
    params.command_type = "MoveBlend";
    params.target_name = target_name;
    params.var_type = "insert_line";
    params.speed = speed;
    params.zone = zone;
    params.ratio = ratio;
    return sendCommand(params, 5000);
}

bool Command::MoveBlendInsertCircle(const std::string& mid_target_name, const std::string& target_name,
    int speed, const std::vector<double>& zone, double ratio)
{
    std::cout << "====== Sending MoveBlend circle insert command ======" << std::endl;
    CommandParams params;
    params.command_type = "MoveBlend";
    params.target_name = target_name;
    params.var_type = "insert_circle";
    params.mid_target_name = mid_target_name;
    params.speed = speed;
    params.zone = zone;
    params.ratio = ratio;
    return sendCommand(params, 5000);
}

bool Command::MoveBlendStart()
{
    std::cout << "====== Sending MoveBlend start command ======" << std::endl;
    CommandParams params;
    params.command_type = "MoveBlend";
    params.var_type = "start";
    return sendCommand(params, 60000);
}

bool Command::MoveBlendLine(const std::string& target_name, int speed,
    const std::vector<double>& zone, double ratio)
{
    std::cout << "====== Executing MoveBlend sequence ======" << std::endl;
    if (!MoveBlendFirstInsert()) {
        std::cerr << "MoveBlend first insert failed, sequence aborted" << std::endl;
        return false;
    }
    if (!MoveBlendInsertLine(target_name, speed, zone, ratio)) {
        std::cerr << "MoveBlend line insert failed, sequence aborted" << std::endl;
        return false;
    }
    if (!MoveBlendStart()) {
        std::cerr << "MoveBlend start failed, sequence aborted" << std::endl;
        return false;
    }
    std::cout << "MoveBlend sequence completed" << std::endl;
    return true;
}


