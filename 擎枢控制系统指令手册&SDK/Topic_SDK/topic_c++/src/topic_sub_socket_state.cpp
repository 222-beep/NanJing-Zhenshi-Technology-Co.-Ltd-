// topic_sub_socket_state.cpp
// 通过 Topic SDK 订阅系统实时状态，读取关节位置、速度、末端位姿和错误码。
// 使用 Direct Mode（自动快照），在主循环中直接调用 API 即可。

#include "system_state_reader.hpp"
#include <chrono>
#include <thread>

int main() {
    // 修改为实际发布者 IP
    std::string remote_ip = "192.168.11.11";
    start_subscriber(remote_ip);

    std::cout << "Subscriber started, listening to " << remote_ip << ":19091" << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (!hasRtData()) continue;

        auto rt = SystemStateReader::snapshotRt();
        if (!rt.valid()) continue;

        // 系统是否正在运行
        std::cout << "system_running: " << (rt.isSystemRunning() ? "true" : "false") << std::endl;

        for (size_t m = 0; m < rt.modelCount(); ++m) {
            std::cout << "\n--- Model " << m << " : " << rt.modelName(m) << " ---" << std::endl;

            // 模型错误码
            std::cout << "  model err_code : " << rt.modelErrorCode(m) << std::endl;

            // 末端位姿（笛卡尔坐标）
            if (rt.hasCurrentPoint(m)) {
                std::cout << "  robottarget : ";
                print_vector(rt.currentRobottarget(m));
                std::cout << std::endl;
            }

            // 各关节的位置、速度和错误码
            for (size_t j = 0; j < rt.jointCount(m); ++j) {
                std::cout << "  Joint " << j
                          << "  pos="     << rt.jointPosition(m, j)
                          << "  vel="     << rt.jointVelocity(m, j)
                          << "  err="     << rt.jointErrorCode(m, j)
                          << std::endl;
            }
        }

        std::cout << "\n==================================================\n" << std::endl;
    }

    return 0;
}
