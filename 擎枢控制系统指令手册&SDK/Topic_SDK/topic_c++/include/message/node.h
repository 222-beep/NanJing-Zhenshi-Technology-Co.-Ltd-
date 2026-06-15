#pragma once

#include <functional>
#include <memory>
#include <string>

#include "google/protobuf/message.h"
#include "message/callback_registry.h"
#include "message/qos.h"
#include "message/subscription.h"
#include "message/mbexport.h"

namespace message_bus {


    /**
     * 节点运行启动参数
     */
    struct NodeOptions {
        //节点名称
        std::string node_name;
        //订阅通道的地址
        std::string sub_url;
        //发布通道的地址
        std::string pub_url;
        //接收消息v处理线程
        int num_message_process_thread = 1;
        //发布队列大小
        int pub_queue_length = 50;
        //订阅队列大小
        int sub_queue_length = 1000;
    };

    class MESSAGEBUS_EXPORT NodeImpl;

    class MESSAGEBUS_EXPORT Node {
    public:
        explicit Node(const NodeOptions& options);

        ~Node();

        /**
         * 通用话题发布接口
         * @tparam T 消息类型
         * @param topic 要发布的话题名
         * @param msg_ptr 要发布的消息
         * @param qos 当前不用关心
         * @return 发布成功与否
         */
        template <class T>
        bool Publish(const std::string& topic, std::shared_ptr<T>& msg_ptr, Qos qos = Qos::DEFAULT) {
            std::shared_ptr<google::protobuf::Message> proto_ptr =
                std::dynamic_pointer_cast<google::protobuf::Message>(msg_ptr);
            return Publish(topic, proto_ptr, qos);
        }

        /**
         * 创建话题订阅，绑定回调函数到话题
         * @tparam T 消息类型
         * @param topic 话题名
         * @param cb 回调
         * @param qos 当前不用关心
         * @return 一个订阅实例
         */
        template <class T>
        std::shared_ptr<Subscription> CreateSubscription(const std::string& topic, std::function<void(const T&)>&& cb) {
            //std::cout << "Address of 0000000Register: " << &Instance().Register << std::endl;
            int id =
                detail::CallbackRegistry::Instance().Register(topic, std::make_shared<detail::CallbackT<T>>(std::move(cb)));
            std::cout << "Creating subscription for topic: " << topic << std::endl;

            return std::make_shared<Subscription>(topic, id);
        }

        /**
         * 创建话题订阅，绑定类成员函数到话题
         * @tparam T 消息类型
         * @tparam ObjType 成员函数所属类型
         * @param topic 话题名
         * @param fp 类成员函数指针
         * @param obj 成员函数所属类对象指针
         * @param qos 当前不用关心
         * @return 一个订阅实例
         */
        template <class T, class ObjType>
        std::shared_ptr<Subscription> CreateSubscription(const std::string& topic, void (ObjType::* fp)(const T&),
            ObjType* obj, Qos qos = Qos::DEFAULT) {
            (void)qos;
            int id = detail::CallbackRegistry::Instance().Register(
                topic, std::make_shared<detail::CallbackT<T>>(std::bind(fp, obj, std::placeholders::_1)));
            return std::make_shared<Subscription>(topic, id);
        }

        int GetMapSize() {
            printf("hehe %p, %d\n", &(detail::CallbackRegistry::Instance().callback_map_), detail::CallbackRegistry::Instance().callback_map_.size());
            printf("vector: %p, %lu\n", &(detail::CallbackRegistry::Instance().test_1), detail::CallbackRegistry::Instance().test_1.size());
            return detail::CallbackRegistry::Instance().callback_map_.size();
        }
    /*
    * 设置连接断开事件回调
    * @param handler 回调函数
    */
    void SetOnConnectHandler(const std::function<void(const int& event, const char* addr)>& handler);

    /*
    * 设置连接断开事件回调
    * @param handler 回调函数
    */
    void SetOnDisconnectHandler(const std::function<void(const int& event, const char* addr)>& handler);

    /**
     * 启动通信节点
     */
    bool Start();

        /**
         * 关闭通信节点
         */
        void Shutdown();

        /**
         * 获取消息回调入口对象
         */
        detail::CallbackRegistry& GetCbRegistry();

    private:
        bool Publish(const std::string& topic, std::shared_ptr<google::protobuf::Message>& msg_ptr, Qos qos);

   private:


    std::shared_ptr<NodeImpl> impl_;

    std::function<void(const int& event, const char* addr)> on_connect_handler_ {nullptr};

    std::function<void(const int& event, const char* addr)> on_disconnect_handler_ {nullptr};

};

}  // namespace message_bus
