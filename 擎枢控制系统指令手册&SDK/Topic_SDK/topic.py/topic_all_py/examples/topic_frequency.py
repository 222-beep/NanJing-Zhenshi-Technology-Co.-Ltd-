"""检测底层 Topic 的实际接收频率。"""

import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from platform_loader import get_topic_module
from system_state_reader import (
    get_nrt_receive_stats,
    get_rt_receive_stats,
    measure_receive_frequency,
)


PUBLISHER_IP = "192.168.11.11"  # 修改为实际发布者 IP
TEST_SECONDS = 5.0


if __name__ == "__main__":
    topic = get_topic_module()
    topic.start_subscriber(PUBLISHER_IP)

    wait_deadline = time.monotonic() + 5.0
    while not (topic.has_rt_data() and topic.has_nrt_data()) and time.monotonic() < wait_deadline:
        time.sleep(0.01)

    print(f"Measuring received messages for {TEST_SECONDS:.1f} seconds...")
    report = measure_receive_frequency(TEST_SECONDS)
    rt = get_rt_receive_stats()
    nrt = get_nrt_receive_stats()

    print(f"RT : {report.rt_hz:.2f} Hz, messages={report.rt_messages}, "
          f"frame_delta={report.rt_end_frame_id - report.rt_start_frame_id}, "
          f"publisher_timestamp_delta={report.rt_end_publisher_timestamp - report.rt_start_publisher_timestamp}, "
          f"total={rt.received_count}, publisher_frame_id={rt.publisher_frame_id}")
    print(f"NRT: {report.nrt_hz:.2f} Hz, messages={report.nrt_messages}, "
          f"frame_delta={report.nrt_end_frame_id - report.nrt_start_frame_id}, "
          f"publisher_timestamp_delta={report.nrt_end_publisher_timestamp - report.nrt_start_publisher_timestamp}, "
          f"total={nrt.received_count}, publisher_frame_id={nrt.publisher_frame_id}")
    print("Frequency is counted in the subscription callback, not by API calls.")
    print("publisher_timestamp_delta is raw controller ticks, not milliseconds.")
