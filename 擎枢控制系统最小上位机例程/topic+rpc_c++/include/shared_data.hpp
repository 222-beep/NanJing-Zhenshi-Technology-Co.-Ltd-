#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include "message/TDMessageBus.h"
#include "sub.hpp"

// ---------- 线程安全的共享数据类 ----------
class SharedSystemState {
public:
    void updateRt(const overall_system_rtstate::SystemRtState& msg) {
        auto new_data = std::make_shared<SystemStateData>();
        display_rt(msg, *new_data);
        std::lock_guard<std::mutex> lock(rt_mtx_);
        rt_data_ = new_data;
        rt_valid_ = true;
    }

    void updateNrt(const overall_system_nrtstate::SystemNrtState& msg) {
        auto new_data = std::make_shared<SystemStateData>();
        display_nrt(msg, *new_data);
        std::lock_guard<std::mutex> lock(nrt_mtx_);
        nrt_data_ = new_data;
        nrt_valid_ = true;
    }

    std::shared_ptr<const SystemStateData> getRt() const {
        std::lock_guard<std::mutex> lock(rt_mtx_);
        return rt_data_;
    }

    std::shared_ptr<const SystemStateData> getNrt() const {
        std::lock_guard<std::mutex> lock(nrt_mtx_);
        return nrt_data_;
    }

    bool hasRt() const {
        std::lock_guard<std::mutex> lock(rt_mtx_);
        return rt_valid_;
    }

    bool hasNrt() const {
        std::lock_guard<std::mutex> lock(nrt_mtx_);
        return nrt_valid_;
    }

private:
    mutable std::mutex rt_mtx_;
    mutable std::mutex nrt_mtx_;
    std::shared_ptr<SystemStateData> rt_data_;
    std::shared_ptr<SystemStateData> nrt_data_;
    bool rt_valid_ = false;
    bool nrt_valid_ = false;
};

// 全局单例访问
inline SharedSystemState& getSystemState() {
    static SharedSystemState instance;
    return instance;
}

// 启动订阅者（只传入 IP 地址，端口固定为 19091）
void start_subscriber(const std::string& remote_ip) {
    using namespace message_bus;
    using namespace message_bus::detail;

    // 固定端口号
    const int port = 19091;
    std::string sub_url = "tcp://" + remote_ip + ":" + std::to_string(port);

    // 配置并启动节点（static 保证生命周期）
    static NodeOptions options;
    options.node_name = "test_subscriber";
    options.sub_url = sub_url;
    static Node node(options);
    node.Start();

    // 创建订阅对象，并用 static 延长生命周期
    static auto sub_rt = node.CreateSubscription<overall_system_rtstate::SystemRtState>(
        "overall_system_rtstate",
        [](const overall_system_rtstate::SystemRtState& msg) {
            getSystemState().updateRt(msg);
        });

    static auto sub_nrt = node.CreateSubscription<overall_system_nrtstate::SystemNrtState>(
        "overall_system_nrtstate",
        [](const overall_system_nrtstate::SystemNrtState& msg) {
            getSystemState().updateNrt(msg);
        });

    //// 启动后台工作线程（示例，可按需修改）
    //static std::thread worker([] {
    //    while (true) {
    //        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //        auto& state = getSystemState();

    //        if (state.hasRt()) {
    //            auto rt_data = state.getRt();
    //            // 在这里添加你的业务逻辑
    //        }

    //        if (state.hasNrt()) {
    //            auto nrt_data = state.getNrt();
    //            // 处理非实时数据
    //        }
    //    }
    //    });
    //worker.detach();
}

#endif // SHARED_DATA_HPP