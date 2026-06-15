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
        "{Recover}"
    };

    std::vector<std::string> motion_cmds = {
        "{JogC --motion_type=0 --direction=1 --step=0.1 --coordinate=0 --speed=v100}",
        "{JogC --motion_type=0 --direction=-1 --step=0.1 --coordinate=0 --speed=v100}"
    };

    auto client = robot::create_client(robot_ip);

    robot::send_rpcsy(*client, init_cmds, 500, 100);

    while (1) {
        robot::send_rpcsy(*client, motion_cmds, 50000, 1000);
        // robot::send_rpc_async(*client, motion_cmds, 10000, 500);
    }

    return 0;
}
