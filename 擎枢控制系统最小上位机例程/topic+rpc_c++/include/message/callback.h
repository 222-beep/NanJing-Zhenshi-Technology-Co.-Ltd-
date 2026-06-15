#pragma once
#include <functional>
#include "message/mbexport.h"
#include "google/protobuf/message.h"

namespace message_bus {
namespace detail {


/**
 * 事件回调基类
 */
class MESSAGEBUS_EXPORT CallbackBase {
   public:
    virtual void Call(google::protobuf::Message* base_msg) = 0;
 virtual std::shared_ptr<google::protobuf::Message> ObtainMessage() {
        return nullptr;
		
    
	}
    int GetId() const { return id_; }

   protected:
    int id_ = 0;
    inline static int id_seq{0};
};

/**
 * 事件回调模板类
 * @tparam T
 */
template <class T>
class MESSAGEBUS_EXPORT CallbackT : public CallbackBase {
   public:
    explicit CallbackT(std::function<void(const T&)>&& cb) : cb_(std::move(cb)) { 
        id_ = CallbackBase::id_seq++;
        //std::cout << "id_:" << id_ << std::endl;
    }

    void Call(google::protobuf::Message* base_msg) override {
        T* msg = dynamic_cast<T*>(base_msg);
        assert(msg != nullptr);
        cb_(*msg);
    }


   private:
    std::function<void(const T&)> cb_;
};

/**
 * 事件回调模板类(带返回值)
 * @tparam T
 * @date 2024-09-13 
 * @author garin.yang
 */
template <class T>
class MESSAGEBUS_EXPORT CallbackNew : public CallbackBase {
public:
    explicit CallbackNew(std::function<std::shared_ptr<T>()>&& cb) : cb_(std::move(cb)) {
        id_ = CallbackBase::id_seq++;
    }

    void Call(google::protobuf::Message* base_msg) override {
        // NULL IMPL
        return;
    }

    std::shared_ptr<google::protobuf::Message> ObtainMessage() override {
        return cb_();
    }

private:
    std::function<std::shared_ptr<T>()> cb_;
};
}  // namespace detail
}  // namespace message_bus
