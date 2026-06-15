#include "robot.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    const std::string robot_ip = "192.168.2.199";
    string input;

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

    auto client = robot::create_client(robot_ip);

    //init
    robot::send_rpcsy(client, init_cmds, 500, 100);

    std::cout << "Start dragging in CST? (start/stop): ";
    std::cin >> input; 

    // main motion
    while (1) {
        if (input == "start") {
            std::cout << "Start DragInCST!!!" << std::endl;
            robot::send_rpcsy(client, Dra_sta, 5000, 1000);
            std::cout << "Stop dragging in CST? (stop)" << std::endl;
            std::cin >> input;
        }
        else if (input == "stop") {
            robot::send_rpcsy(client, Dra_stp, 5000, 1000);
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


#ifdef _WIN32
#include <windows.h>
void delay_ms(unsigned int ms) { Sleep(ms); }
#else
#include <unistd.h>
void delay_ms(unsigned int ms) { usleep(ms * 1000); }
#endif