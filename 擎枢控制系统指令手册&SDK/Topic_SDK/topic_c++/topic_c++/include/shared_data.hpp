#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <cstdint>
#include <stdexcept>
#include "sub.hpp"
#include "message/TDMessageBus.h"

struct TopicMessageReceiveStats {
    uint64_t received_count = 0;          // 订阅回调实际收到的消息总数
    int64_t publisher_timestamp = 0;      // 消息头中的发布端时间戳
    int64_t publisher_frame_id = 0;       // 消息头中的发布端帧号
    int64_t local_receive_timestamp_us = 0; // 本机收到消息的 Unix 时间戳（微秒）
};

struct TopicMessageFrequencyReport {
    double elapsed_seconds = 0.0;
    uint64_t rt_messages = 0;
    uint64_t nrt_messages = 0;
    double rt_hz = 0.0;
    double nrt_hz = 0.0;
    int64_t rt_start_frame_id = 0;
    int64_t rt_end_frame_id = 0;
    int64_t nrt_start_frame_id = 0;
    int64_t nrt_end_frame_id = 0;
    int64_t rt_start_publisher_timestamp = 0;
    int64_t rt_end_publisher_timestamp = 0;
    int64_t nrt_start_publisher_timestamp = 0;
    int64_t nrt_end_publisher_timestamp = 0;
};

// ---------- 线程安全的共享数据类 ----------
class SharedSystemState {
public:
    void updateRt(const overall_system_rtstate::SystemRtState& msg) {
        const auto receive_time = std::chrono::system_clock::now();
        auto new_data = std::make_shared<SystemStateData>();
        display_rt(msg, *new_data);
        std::lock_guard<std::mutex> lock(rt_mtx_);
        rt_data_ = new_data;
        rt_valid_ = true;
        ++rt_stats_.received_count;
        if (msg.has_head()) {
            rt_stats_.publisher_timestamp = msg.head().timestamp();
            rt_stats_.publisher_frame_id = msg.head().frame_id();
        }
        rt_stats_.local_receive_timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            receive_time.time_since_epoch()).count();
    }

    void updateNrt(const overall_system_nrtstate::SystemNrtState& msg) {
        const auto receive_time = std::chrono::system_clock::now();
        auto new_data = std::make_shared<SystemStateData>();
        display_nrt(msg, *new_data);
        std::lock_guard<std::mutex> lock(nrt_mtx_);
        nrt_data_ = new_data;
        nrt_valid_ = true;
        ++nrt_stats_.received_count;
        if (msg.has_head()) {
            nrt_stats_.publisher_timestamp = msg.head().timestamp();
            nrt_stats_.publisher_frame_id = msg.head().frame_id();
        }
        nrt_stats_.local_receive_timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            receive_time.time_since_epoch()).count();
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

    TopicMessageReceiveStats getRtReceiveStats() const {
        std::lock_guard<std::mutex> lock(rt_mtx_);
        return rt_stats_;
    }

    TopicMessageReceiveStats getNrtReceiveStats() const {
        std::lock_guard<std::mutex> lock(nrt_mtx_);
        return nrt_stats_;
    }

private:
    mutable std::mutex rt_mtx_;
    mutable std::mutex nrt_mtx_;
    std::shared_ptr<SystemStateData> rt_data_;
    std::shared_ptr<SystemStateData> nrt_data_;
    bool rt_valid_ = false;
    bool nrt_valid_ = false;
    TopicMessageReceiveStats rt_stats_;
    TopicMessageReceiveStats nrt_stats_;
};

// 全局单例访问
inline SharedSystemState& getSystemState() {
    static SharedSystemState instance;
    return instance;
}

// 在指定窗口内按订阅回调计数，测得的是工程实际收到消息的频率。
// 该函数不会改变发布端频率，也不按业务层 get*() 调用次数计数。
inline TopicMessageFrequencyReport measureMessageFrequency(int duration_ms = 5000) {
    if (duration_ms <= 0) {
        throw std::invalid_argument("duration_ms must be greater than 0");
    }

    const auto& state = getSystemState();
    const auto rt_before = state.getRtReceiveStats();
    const auto nrt_before = state.getNrtReceiveStats();
    const auto begin = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    const auto end = std::chrono::steady_clock::now();
    const auto rt_after = state.getRtReceiveStats();
    const auto nrt_after = state.getNrtReceiveStats();

    TopicMessageFrequencyReport report;
    report.elapsed_seconds = std::chrono::duration<double>(end - begin).count();
    report.rt_messages = rt_after.received_count - rt_before.received_count;
    report.nrt_messages = nrt_after.received_count - nrt_before.received_count;
    report.rt_hz = report.rt_messages / report.elapsed_seconds;
    report.nrt_hz = report.nrt_messages / report.elapsed_seconds;
    report.rt_start_frame_id = rt_before.publisher_frame_id;
    report.rt_end_frame_id = rt_after.publisher_frame_id;
    report.nrt_start_frame_id = nrt_before.publisher_frame_id;
    report.nrt_end_frame_id = nrt_after.publisher_frame_id;
    report.rt_start_publisher_timestamp = rt_before.publisher_timestamp;
    report.rt_end_publisher_timestamp = rt_after.publisher_timestamp;
    report.nrt_start_publisher_timestamp = nrt_before.publisher_timestamp;
    report.nrt_end_publisher_timestamp = nrt_after.publisher_timestamp;
    return report;
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
