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
        "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}"
    };

    std::vector<std::string> motion_cmds = {
        "{MoveAbsJ --jointtarget_var=j0}",
        "{MoveAbsJ --jointtarget_var=j1}",
        "{MoveAbsJ --jointtarget_var=j2}"
    };

    std::vector<std::string> motion_speedL_cmds = {
        "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
        "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
        "{Stop}",
        "{Start}"
    };

    auto client = robot::create_client(robot_ip);

    robot::send_rpcsy(*client, init_cmds, 500, 100);

    while (1) {
        robot::send_rpcsy(*client, motion_cmds, 50000, 1000);
        // robot::send_rpc_async(*client, motion_speedL_cmds, 10000, 500);
    }

    return 0;
}