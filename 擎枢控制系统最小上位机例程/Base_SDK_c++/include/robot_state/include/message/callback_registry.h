#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include "mbexport.h"
#include "callback.h"

namespace message_bus {
namespace detail {
/**
 * 话题订阅类，保存话题和订阅回调的映射
 */
class MESSAGEBUS_EXPORT CallbackRegistry {
   private:
    CallbackRegistry() = default;

   public:
    CallbackRegistry(const CallbackRegistry& other) = delete;
    CallbackRegistry& operator=(const CallbackRegistry& other) = delete;

    static CallbackRegistry& Instance() {
        static CallbackRegistry subscription_registry;
        //std::cout << "inner: " << &subscription_registry << std::endl;
        return subscription_registry;
    }

    /**
     * 注册一个订阅事件到话题，支持注册多个事件到同一个话题
     * @param topic 要注册的话题
     * @param event_base 封装了处理回调的订阅事件
     * @return
     */
    int Register(const std::string& topic, std::shared_ptr<CallbackBase>&& event_base);

    /**
     * 注销话题订阅
     * @param topic 话题名
     * @param id 订阅ID
     */
    void Unregister(const std::string& topic, int id);

    /**
     * 获取话题topic关联的所有订阅事件
     * @param topic
     * @return
     */
    std::vector<std::shared_ptr<CallbackBase>> GetRegisteredEvents(const std::string& topic);

    public:
    std::unordered_multimap<std::string, std::shared_ptr<CallbackBase>> callback_map_;
    std::vector<int> test_1;
    std::mutex mutex_;
    int test{ -1 };
};

}  // namespace detail
}  // namespace message_bus