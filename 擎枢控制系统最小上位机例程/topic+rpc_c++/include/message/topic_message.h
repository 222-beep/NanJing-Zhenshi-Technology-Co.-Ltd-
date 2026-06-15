#pragma once

#include <google/protobuf/descriptor.h>

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "google/protobuf/message.h"
#include "message/meta_data.pb.h"

namespace message_bus {
namespace detail {

/**
 * 不同节点间，实现消息传输功能时，实际需要传输的内容
 */
struct TopicMessage {
    //话题名
    std::string topic;
    //元数据,包括proto消息类型名称等，用于从字节流反射得到proto消息对象
    meta_data::MetaData meta_data;
    //实际传输的proto消息对象
    std::shared_ptr<google::protobuf::Message> proto_msg = nullptr;

    std::string DebugString() {
        std::stringstream ss;
        ss << "topic: " << topic << std::endl;
        ss << meta_data.DebugString();
        if (proto_msg) {
            ss << proto_msg->DebugString();
        }

        return ss.str();
    }
};

/**
 * 计算将一个TopicMessage对象序列化之后的字节数，一般用于申请缓冲区
 * @param topic_message 要计算的对象
 * @return 字节数
 */
size_t CalcPackedTopicMessageByteSize(const TopicMessage &topic_message);

/**
 * 将topic_message打包到dest_buf中
 * @param topic_message 要打包的TopicMessage对象
 * @param dest_buf 输出的缓存, 大小需要大于等于CalcPackedTopicMessageByteSize()的计算值
 * @return 成功或失败
 */
bool Pack(const TopicMessage &topic_message, uint8_t *dest_buf);

/**
 * 从字节缓存区解析得到TopicMessage对象
 * @param buf 缓存去首地址
 * @param length 缓存区大小
 * @param topic_message 解析得到的TopicMessage对象
 * @return 成功或失败
 */
bool Unpack(void *buf, size_t length, TopicMessage &topic_message);

}  // namespace detail
}  // namespace message_bus
