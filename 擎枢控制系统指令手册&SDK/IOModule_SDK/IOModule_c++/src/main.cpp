#include "robot.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    const std::string robot_ip = "192.168.1.11";

    // 初始化指令
    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Mode}",
        "{SetMaxToq}",
        "{Recover}",
        "{SetRate}",
        "{Enable}",
    };

    // 三条 IO 指令（示例参数，可自行修改）
    std::vector<std::string> getdi_cmds = {
        "{GetDI --di_name=DI0}",
    };

    std::vector<std::string> setdo_cmds = {
        "{SetDO --do_name=DO2 --do_value=1}",
    };

    std::vector<std::string> dopulse_cmds = {
        "{DOPulse --do_name=DO1 --pulse_active=1 --high_cycles=10 --low_cycles=10}",
    };

    auto client = robot::create_client(robot_ip);

    // 发送初始化指令
    robot::send_rpcsy(*client, init_cmds, 500, 100);
    std::cout << "初始化完成" << std::endl;

    while (true) {
        std::cout << "\n=== IO 模块指令菜单 ===" << std::endl;
        std::cout << "getdi    - 读取数字输入  " << std::endl;
        std::cout << "setdo    - 设置数字输出  " << std::endl;
        std::cout << "dopulse  - 脉冲输出      " << std::endl;
        std::cout << "exit     - 退出程序      " << std::endl;
        std::cout << "请输入命令: ";

        std::string user_input;
        if (!std::getline(std::cin, user_input)) {
            break;
        }

        if (user_input == "getdi") {
            robot::send_rpcsy(*client, getdi_cmds, 5000, 500);
            std::cout << "[GetDI] 指令已发送" << std::endl;
        } else if (user_input == "setdo") {
            robot::send_rpcsy(*client, setdo_cmds, 5000, 500);
            std::cout << "[SetDO] 指令已发送" << std::endl;
        } else if (user_input == "dopulse") {
            robot::send_rpcsy(*client, dopulse_cmds, 5000, 500);
            std::cout << "[DOPulse] 指令已发送" << std::endl;
        } else if (user_input == "exit") {
            std::cout << "退出程序..." << std::endl;
            break;
        } else {
            std::cout << "未知命令，请重新输入!" << std::endl;
        }
    }

    return 0;
}
