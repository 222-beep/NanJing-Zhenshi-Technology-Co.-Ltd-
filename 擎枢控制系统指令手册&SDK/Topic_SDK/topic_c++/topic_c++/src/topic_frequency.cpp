#include "system_state_reader.hpp"
#include <iomanip>
#include <iostream>

int main() {
    const std::string remote_ip = "192.168.2.100"; // 修改为实际发布者 IP
    const int test_duration_ms = 5000;

    start_subscriber(remote_ip);
    const auto wait_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ((!hasRtData() || !hasNrtData()) && std::chrono::steady_clock::now() < wait_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "Measuring received messages for "
              << test_duration_ms / 1000.0 << " seconds..." << std::endl;

    const auto report = measureReceiveFrequency(test_duration_ms);
    const auto rt = getRtReceiveStats();
    const auto nrt = getNrtReceiveStats();

    std::cout << std::fixed << std::setprecision(2)
              << "RT : " << report.rt_hz << " Hz, messages=" << report.rt_messages
              << ", frame_delta=" << report.rt_end_frame_id - report.rt_start_frame_id
              << ", publisher_timestamp_delta="
              << report.rt_end_publisher_timestamp - report.rt_start_publisher_timestamp
              << ", total=" << rt.received_count
              << ", publisher_frame_id=" << rt.publisher_frame_id << '\n'
              << "NRT: " << report.nrt_hz << " Hz, messages=" << report.nrt_messages
              << ", frame_delta=" << report.nrt_end_frame_id - report.nrt_start_frame_id
              << ", publisher_timestamp_delta="
              << report.nrt_end_publisher_timestamp - report.nrt_start_publisher_timestamp
              << ", total=" << nrt.received_count
              << ", publisher_frame_id=" << nrt.publisher_frame_id << '\n'
              << "Frequency is counted in the subscription callback, not by API calls."
              << '\n'
              << "publisher_timestamp_delta is raw controller ticks, not milliseconds."
              << std::endl;
    return 0;
}
