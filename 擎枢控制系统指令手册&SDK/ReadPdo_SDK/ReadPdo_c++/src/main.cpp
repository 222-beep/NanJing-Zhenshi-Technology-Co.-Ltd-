#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    const std::string robot_ip = "192.168.11.11";

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

    // ReadPdo 指令参数（可在菜单中随时修改 slave_id/index/size）
    // slave_id  : 从站编号
    // index     : PDO 对象索引（十六进制）
    // sub_index : PDO 对象子索引（十六进制）
    // size      : 读取数据位宽（bit）
    // interval  : 读取间隔（s）
    // loop      : 循环读取次数
    int slave_id = 6;
    int pdo_index = 0x6041;
    int pdo_size = 16;

    cpp_rpc::CPPClient client(robot_ip, 5868);

    // 发送初始化指令
    send_rpcsy<RespDemo>(client, init_cmds, 100, 500);
    std::cout << "初始化完成" << std::endl;

    while (true) {
        std::cout << "\n=== ReadPdo 指令菜单 ===" << std::endl;
        std::cout << "readpdo  - 读取 PDO 数据  " << std::endl;
        std::cout << "set      - 修改参数(slave_id/index/size)" << std::endl;
        std::cout << "exit     - 退出程序      " << std::endl;
        printf("当前参数: slave_id=%d  index=0x%04X  size=%d\n", slave_id, pdo_index, pdo_size);
        std::cout << "请输入命令: ";

        std::string user_input;
        if (!std::getline(std::cin, user_input)) {
            break;
        }

        if (user_input == "readpdo") {
            // 用当前参数拼接 ReadPdo 指令，用 RespPdo 解析返回值
            char index_str[16];
            snprintf(index_str, sizeof(index_str), "0x%04X", pdo_index);
            std::string readpdo_cmd = "{ReadPdo --slave_id=" + std::to_string(slave_id) +
                " --index=" + index_str + " --sub_index=0x00 --size=" + std::to_string(pdo_size) +
                " --interval=1 --loop=1}";
            auto results = send_rpcsy<RespPdo>(client, {readpdo_cmd}, 500, 5000);
            for (const auto& r : results) {
                std::cout << "[ReadPdo] subcmd_index: " << r.subcmd_index << std::endl;
                std::cout << "[ReadPdo] return_code: " << r.return_code << std::endl;
                std::cout << "[ReadPdo] return_message: " << r.return_message << std::endl;
                if (r.has_pdo_value) {
                    printf("[ReadPdo] pdo_value: %d (0x%X)\n", r.pdo_value, r.pdo_value);
                }
            }
            std::cout << "[ReadPdo] 指令已发送" << std::endl;
        } else if (user_input == "set") {
            std::cout << "请输入要修改的参数名 (slave_id/index/size): ";
            std::string param;
            if (!std::getline(std::cin, param)) {
                break;
            }
            std::cout << "请输入新值（index 支持 0x 十六进制）: ";
            std::string value;
            if (std::getline(std::cin, value)) {
                try {
                    if (param == "slave_id") {
                        slave_id = std::stoi(value);
                        std::cout << "slave_id 已修改为: " << slave_id << std::endl;
                    } else if (param == "index") {
                        pdo_index = static_cast<int>(std::stoul(value, nullptr, 0));
                        printf("index 已修改为: 0x%04X\n", pdo_index);
                    } else if (param == "size") {
                        pdo_size = std::stoi(value);
                        std::cout << "size 已修改为: " << pdo_size << std::endl;
                    } else {
                        std::cout << "未知参数，可修改: slave_id/index/size" << std::endl;
                    }
                } catch (...) {
                    std::cout << "输入无效，参数保持不变!" << std::endl;
                }
            }
        } else if (user_input == "exit") {
            std::cout << "退出程序..." << std::endl;
            break;
        } else {
            std::cout << "未知命令，请重新输入!" << std::endl;
        }
    }

    return 0;
}
