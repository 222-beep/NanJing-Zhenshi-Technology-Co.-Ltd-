#include "robot.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    const std::string robot_ip = "192.168.2.199";

    std::vector<std::string> init_cmds = {
    "{Clear}",
    "{Disable}",
    "{Mode}",
    "{SetMaxToq}",
    "{Recover}",
    "{SetRate}",
    "{Enable}",
    "{Var --clear}",
    "{Recover}",
    // 定义机械臂1的关节目标变量
    "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j3 --value={-0.1,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j4 --value={-0.2,0,0,0,0,0,0,0,0,0}}",
    // 定义机械臂2的关节目标变量
    "{Var --type=jointtarget --name=j11 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j21 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j22 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j23 --value={-0.1,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j24 --value={-0.2,0,0,0,0,0,0,0,0,0}}"
    };

    // 双臂运动指令列表 - 使用||分隔两个机械臂的指令
    std::vector<std::string> motion_cmds = {
    // 双臂同时运动到初始位置
    "{MoveAbsJ --jointtarget_var=j0||MoveAbsJ --jointtarget_var=j11}",
    // 双臂同时运动到不同位置
    "{MoveAbsJ --jointtarget_var=j1||MoveAbsJ --jointtarget_var=j21}",
    "{MoveAbsJ --jointtarget_var=j2||MoveAbsJ --jointtarget_var=j22}",                          
    "{MoveAbsJ --jointtarget_var=j3||MoveAbsJ --jointtarget_var=j23}",
    "{MoveAbsJ --jointtarget_var=j4||MoveAbsJ --jointtarget_var=j24}"
    };

    // 双臂在线规划指令
    std::vector<std::string> motion_speedL_cmds = {
    // 双臂同时进行在线规划运动
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000||SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000||SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
    "{Stop||Stop}",
    "{Start||Start}",
    };

    auto client = robot::create_client(robot_ip);

    robot::send_rpcsy(*client, init_cmds, 500, 100);

    while (1) {
        robot::send_rpcsy(*client, motion_cmds, 50000, 1000);
        // robot::send_rpc_async(*client, motion_speedL_cmds, 10000, 500);
    }

    return 0;
}