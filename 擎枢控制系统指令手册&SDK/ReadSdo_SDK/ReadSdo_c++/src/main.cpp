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

    // ReadSdo 指令参数（可在菜单中随时修改 slave_id/index/sub_index/size）
    // slave_id  : 从站编号（从 0 开始，0 通常代表网络中第一个从站）
    // index     : 对象字典主索引（十六进制）
    // sub_index : 对象字典子索引（十六进制）
    // size      : 字节长度，指定要读取的数据大小
    // loop      : 循环次数，指令命令执行的总次数
    int slave_id = 5;
    int sdo_index = 0x6064;
    int sdo_sub_index = 0x00;
    int sdo_size = 4;

    cpp_rpc::CPPClient client(robot_ip, 5868);

    // 发送初始化指令
    send_rpcsy<RespDemo>(client, init_cmds, 100, 500);
    std::cout << "初始化完成" << std::endl;

    while (true) {
        std::cout << "\n=== ReadSdo 指令菜单 ===" << std::endl;
        std::cout << "readsdo  - 读取 SDO 数据  " << std::endl;
        std::cout << "set      - 修改参数(slave_id/index/sub_index/size)" << std::endl;
        std::cout << "exit     - 退出程序      " << std::endl;
        printf("当前参数: slave_id=%d  index=0x%04X  sub_index=0x%02X  size=%d\n",
               slave_id, sdo_index, sdo_sub_index, sdo_size);
        std::cout << "请输入命令: ";

        std::string user_input;
        if (!std::getline(std::cin, user_input)) {
            break;
        }

        if (user_input == "readsdo") {
            // 用当前参数拼接 ReadSdo 指令，用 RespSdo 解析返回值
            char index_str[16], sub_str[16];
            snprintf(index_str, sizeof(index_str), "0x%04X", sdo_index);
            snprintf(sub_str, sizeof(sub_str), "0x%02X", sdo_sub_index);
            std::string readsdo_cmd = "{ReadSdo --slave_id=" + std::to_string(slave_id) +
                " --index=" + index_str + " --sub_index=" + sub_str +
                " --size=" + std::to_string(sdo_size) + " --loop=1}";
            auto results = send_rpcsy<RespSdo>(client, {readsdo_cmd}, 500, 5000);
            for (const auto& r : results) {
                std::cout << "[ReadSdo] subcmd_index: " << r.subcmd_index << std::endl;
                std::cout << "[ReadSdo] return_code: " << r.return_code << std::endl;
                std::cout << "[ReadSdo] return_message: " << r.return_message << std::endl;
                if (r.has_sdo_value) {
                    printf("[ReadSdo] sdo_value: %d (0x%X)\n", r.sdo_value, r.sdo_value);
                }
            }
            std::cout << "[ReadSdo] 指令已发送" << std::endl;
        } else if (user_input == "set") {
            std::cout << "请输入要修改的参数名 (slave_id/index/sub_index/size): ";
            std::string param;
            if (!std::getline(std::cin, param)) {
                break;
            }
            std::cout << "请输入新值（index/sub_index 支持 0x 十六进制）: ";
            std::string value;
            if (std::getline(std::cin, value)) {
                try {
                    if (param == "slave_id") {
                        slave_id = std::stoi(value);
                        std::cout << "slave_id 已修改为: " << slave_id << std::endl;
                    } else if (param == "index") {
                        sdo_index = static_cast<int>(std::stoul(value, nullptr, 0));
                        printf("index 已修改为: 0x%04X\n", sdo_index);
                    } else if (param == "sub_index") {
                        sdo_sub_index = static_cast<int>(std::stoul(value, nullptr, 0));
                        printf("sub_index 已修改为: 0x%02X\n", sdo_sub_index);
                    } else if (param == "size") {
                        sdo_size = std::stoi(value);
                        std::cout << "size 已修改为: " << sdo_size << std::endl;
                    } else {
                        std::cout << "未知参数，可修改: slave_id/index/sub_index/size" << std::endl;
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
