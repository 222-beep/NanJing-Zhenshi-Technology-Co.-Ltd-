#pragma once

#include <string>
#include "message/mbexport.h"
//#include "callback_registry.h"

namespace message_bus {

/**
 * 话题(消息)订阅类。
 */
class MESSAGEBUS_EXPORT Subscription {
   public:
    Subscription(std::string topic, int id);

    ~Subscription();

    /**
     * 取消订阅
     */
    void Unsubscribe();

   private:
    int id_;
    std::string topic_;
    bool unsubscribed_ = false;
};

}  // namespace message_bus