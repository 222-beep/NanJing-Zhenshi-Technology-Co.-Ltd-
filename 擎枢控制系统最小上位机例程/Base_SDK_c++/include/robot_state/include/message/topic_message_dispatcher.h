#pragma once
#include <memory>

#include "callback_registry.h"
#include "topic_message.h"
#include "BS_thread_pool.hpp"

namespace message_bus {
namespace detail {



/**
 * 接收消息派发类
 */
class TopicMessageDispatcher {
   public:
    explicit TopicMessageDispatcher(CallbackRegistry& registry, unsigned int thread_num);

    ~TopicMessageDispatcher() {
        std::cout << "~TopicMessageDispatcher" << std::endl;
    }

   public:
    /**
     * 根据topic_msg中的话题类型，查找已注册的回调，一次调用
     * @param topic_msg TopicMessage对象智能指针
     */
    void OnTopicMessage(std::shared_ptr<detail::TopicMessage>& topic_msg);

    /**
     * 获取注册回调对象
     * @turn CallbackRegistry对象智能指针
     */
    CallbackRegistry& GetCbRegistry() {
        return registry_;
    }

   private:

    CallbackRegistry& registry_;
    BS::thread_pool<BS::tp::none> thread_pool_;
};

}  // namespace detail
}  // namespace message_bus