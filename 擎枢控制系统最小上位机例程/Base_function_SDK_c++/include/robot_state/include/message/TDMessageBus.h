#pragma once

#include <algorithm>
#include <memory>

#include "node.h"

namespace message_bus {

    typedef google::protobuf::Message GoogleMsg;
    typedef std::shared_ptr<google::protobuf::Message> GoogleMsgPtr;

    class MESSAGEBUS_EXPORT TDMessageBus
    {
    public:
        static TDMessageBus& GetInstance() {
            static TDMessageBus instance;
            return instance;
        }

        //virtual bool Start(const std::string& sNode, const std::string& sPub, const std::string& sSub);
        //virtual void Stop() {};

        // 注册类成员函数
        template<typename T>
        std::shared_ptr<Subscription> Register(const std::string& topic, void (T::* fp)(const GoogleMsg&),
            T* obj)
        {
            std::shared_ptr<Subscription> subscriPtr;
            if (m_nodePtr && !topic.empty())
            {
                subscriPtr = m_nodePtr->CreateSubscription<GoogleMsg, T>(topic, fp, obj);
            }

            return subscriPtr;
        }

        template<typename T>
        std::shared_ptr<Subscription> Register(const std::string& topic, void (T::* fp)(),
            T* obj)
        {
            std::shared_ptr<Subscription> subscriPtr;
            if (m_nodePtr && !topic.empty())
            {
                subscriPtr = m_nodePtr->CreateSubscription<GoogleMsg, T>(topic, fp, obj);
            }

            return subscriPtr;
        }

        //virtual void Unregister(const std::shared_ptr<Subscription>& subPtr) {};

        // 注册普通函数
        std::shared_ptr<Subscription> Register(const std::string& topic, std::function<void(const GoogleMsg&)>&& cb);

        bool Publish(const std::string& topic, const GoogleMsgPtr& msgPtr);

        static GoogleMsgPtr createMessage(const GoogleMsg& _msg);
        static void copyMessage(GoogleMsgPtr& _msgPtr, const GoogleMsg& _msg);

    private:
        TDMessageBus() = default;
        //~TDMessageBus() { TDMessageBus::Stop(); }

        TDMessageBus(const TDMessageBus&) = delete;
        TDMessageBus& operator=(const TDMessageBus&) = delete;

    private:
        std::string m_sNode;
        std::string m_sPub;
        std::string m_sSub;
        std::shared_ptr<Node> m_nodePtr;
    };

}


