#include "rpc_client.h"
#include <iostream>
#include <vector>
#include <string>

int main() {
    const std::string robot_ip = "192.168.2.199";
    std::string input;

    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Enable}",
    };

    std::vector<std::string> Dra_sta = {
        "{SwitchToCST}",  
        "{DragInCST --cf_coef={0,0,0,0,0,0,0} --vf_coef={0,0,0,0,0,0,0} --vel_limit={0.3,0.3,0.3,0.3,0.3,0.3,0.3} --ping_pong_amp=0 --zero_check=0.004}"
    };

    std::vector<std::string> Dra_stp = {
        "{Stop --last_count=10}", 
        "{SwitchToCSP}",
        "{Recover}",
        "{Start}"
    };

    std::vector<std::string> your_cmds = {
        //添加你自己的指令
    };

    cpp_rpc::CPPClient client(robot_ip, 5868);

    //init
    send_rpcsy<RespDemo>(client, init_cmds, 100, 500);

    std::cout << "Start dragging in CST? (start/stop): ";
    std::cin >> input; 

    // main motion
    while (1) {
        if (input == "start") {
            std::cout << "Start DragInCST!!!" << std::endl;
            send_rpcsy<RespDemo>(client, Dra_sta, 1000, 5000);
            std::cout << "Stop dragging in CST? (stop)" << std::endl;
            std::cin >> input;
        }
        else if (input == "stop") {
            send_rpcsy<RespDemo>(client, Dra_stp, 1000, 5000);
            std::cout << "Dragging stopped!!" << std::endl;
            std::cout << std::endl;
            std::cout << "Start dragging in CST? (start/stop):" << std::endl;
            std::cin >> input;
        }
        else {
            std::cout << "Start dragging in CST? (start/stop):" << std::endl;
            std::cin >> input;
        }
    }
    return 0;
}


