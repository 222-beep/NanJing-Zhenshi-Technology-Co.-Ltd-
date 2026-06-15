/*
 * Copyright (c) 2024 by YB
 * All Rights Reserved.
 *
 *
 * @Version: 0.0.1
 * @Author: yangbo
 * @Date: 2024-09-03 16:47:08
 * @LastEditors: yangbo
 * @LastEditTime: 2024-11-18 15:03:14
 * @FilePath: /maint_platform/maint_platform_srv/util/cpp-rpc/inc/cpp_rpc.hpp
 * @Description: 通信工具类（包含 客户端/服务端）
 */
#ifndef CPP_RPC_HPP
#define CPP_RPC_HPP
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <functional>
#include <future>
#include <chrono>
#include <thread>
#include <utility>
#include <inttypes.h>
#include "json.hpp"
#include "easy_json.h"

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/epoll.h>
#else
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// #include <Winsock2.h>
#include "wepoll.h"
#include <crtdefs.h>
#include <ws2tcpip.h>
typedef SSIZE_T ssize_t;

#pragma comment(lib, "Ws2_32.lib")
#endif

#include <mutex>
#include <condition_variable>
#include <string.h>
// #include <tuple>
#include <atomic>
#include <unordered_map>
#include <regex>
#include "task_pool.hpp"
#include "msg.hpp"

namespace cpp_rpc
{

    constexpr int UNIQUE_CLEAR_ERR_ID = 198808;
    constexpr int RPC_ERROR_CODE = -1;
    constexpr int RPC_TIMEOUT_CODE = -3;

    class CPP_RPC_EXPORT CPPServer
    {
    public:
        CPPServer() = default;

        /**
         * @description:
         * @return {*}
         * @param {int} port
         */
        CPPServer(int port);

        /**
         * @description:
         * @return {*}
         * @param {int} port
         */
        ~CPPServer();

        /**
         * @description: 启动服务端
         * @return {*}
         */
        void StartServer();

        /**
         * @description: 处理新连接
         * @return {*}
         */
        void HandleNewConnection();

        /**
         * @description: 处理现有连接
         * @return {*}
         * @param {int} client_socket
         */
        void HandleExistingConnection(int client_socket);

        /**
         * @description: 注册回调函数-连接
         * @return {*}
         * @param {function<void(int)>} callback
         */
        void SetNewConnectionCallback(std::function<void(int)> callback);

        /**
         * @description: 注册回调函数-断连
         * @return {*}
         * @param {function<void(int)>} callback
         */
        void SetDisconnectionCallback(std::function<void(int)> callback);

        /**
         * @description: 注册回调函数-接收数据
         * @return {*}
         * @param {function<void(int,core::Msg&)>} callback
         */
        void SetReceiveDataCallback(std::function<void(int, const core::Msg &)> callback);

        // TODO: 获取当前连接映射 <ip, fd> 'backup for safety'
        std::unordered_map<std::string, int> GetConnectedClients()
        {
            std::unique_lock<std::mutex> lk(mtx4clientlist_);
            return client_list_;
        }

        // [主动发起] 发送消息给指定 ip
        // 根据ip找到 fd句柄
        bool SendAwaitViaIp(std::string ip, core::Msg &msg)
        {

            // std::cout << "find ip:" << ip << ", map size: " << client_list_.size() << ", addr:" << &client_list_ << std::endl;
            //  dump
            //  std::unique_lock<std::mutex> lk(mtx4clientlist_);
            //  auto iter =  client_list_.begin();
            //  for(; iter != client_list_.end(); ++iter) {
            //      std::cout << " ==== " << iter->first << "," << iter->second << std::endl;
            //  }
            //  end

            auto it = client_list_.find(ip);
            if (it == client_list_.end())
            {
                std::cout << "client: " << ip << " not found, please check the connection is ok." << std::endl;
                return false;
            }

            // 设置固定回报id
            msg.setMsgID(8888);

            // 找到对应的客户端
            ssize_t sent = send(it->second, reinterpret_cast<const char *>(&msg.header()), msg.size() + sizeof(core::MsgHeader), 0);
            if (sent == -1)
            {
                std::cerr << "Failed to send message." << std::endl;
                return false;
            }

            return true;
        }

        // [被动响应] 发送消息对端client
        // 根据fd句柄
        bool SendAwaitViaFD(int fd, core::Msg &msg)
        {

            // 找到对应的客户端
            ssize_t sent = send(fd, reinterpret_cast<const char *>(&msg.header()), msg.size() + sizeof(core::MsgHeader), 0);
            if (sent == -1)
            {
                std::cerr << "Failed to send message." << std::endl;
                return false;
            }

            return true;
        }

    private:
        int server_port_{-1};
        int listen_socket_{-1};
#ifndef WIN32
        int epoll_fd_;
#else
        HANDLE epoll_fd_;
#endif
        std::unordered_map<std::string, int> client_list_;
        std::mutex mtx4clientlist_;
        static constexpr size_t BUFFER_SIZE = 1024;
        std::function<void(int)> new_connection_callback;
        std::function<void(int)> disconnection_callback;
        std::function<void(int, const core::Msg &)> receive_data_callback;
    };

    struct CommResp
    {
        int32_t return_code;
        int32_t subcmd_index;
        std::string return_message;
        virtual ~CommResp() = default;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommResp, return_code, subcmd_index, return_message)

    // 客户端类
    class CPP_RPC_EXPORT CPPClient
    {
        struct PendingItem
        {
            std::shared_ptr<std::promise<core::Msg>> promise;
            std::shared_ptr<std::atomic<bool>> valid_flag;
            std::function<void(int, const core::Msg &)> callback;
            std::chrono::steady_clock::time_point deadline;
            PendingItem() = default;
            PendingItem(std::shared_ptr<std::promise<core::Msg>> p,
                        std::shared_ptr<std::atomic<bool>> v)
                : promise(std::move(p)), valid_flag(std::move(v)) {}
            PendingItem(std::function<void(int, const core::Msg &)> cb,
                        std::shared_ptr<std::atomic<bool>> v,
                        std::chrono::steady_clock::time_point expires)
                : valid_flag(std::move(v)), callback(std::move(cb)), deadline(expires) {}
        };

    public:
        using PushCallback = void (*)(const core::Msg &);

        // 接收指定大小数据包
        static int safe_recv(decltype(socket(AF_INET, SOCK_STREAM, 0)) s, char *data, int size, std::atomic<bool> &destroyed)
        {
            int result = 0;
            if (size <= 0 || size > 16 * 1024 * 1024)
                return -1;

            while (result < size)
            {
                if (destroyed.load())
                    return -2;

                int ret = recv(s, data + result, size - result, 0);
                if (ret > 0)
                {
                    result += ret;
                    continue;
                }
                if (ret == 0)
                {
                    // 对端关闭
                    return 0;
                }

                // ret < 0
#ifdef WIN32
                int err = WSAGetLastError();

                if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK || err == WSAEINTR)
                {
                    continue; // 只是超时，继续收
                }
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                {
                    continue;
                }
#endif
                // 真错误
                return -1;
            }
            return result;
        }

        CPPClient(const std::string &ip, int port, int connect_timeout_ms = 2000,
                  PushCallback push_handler = nullptr)
            : server_ip_(ip), server_port_(port), client_socket_(-1),
              connect_timeout_ms_(connect_timeout_ms), destroyed_(false)
        {
            push_handler_ = push_handler;

#ifdef WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            {
                return;
            }
#endif

            // 连接服务端
            if (!ConnectToServer())
            {
                connected_.store(false);
                return;
            }

            timeout_thread_ = std::thread([this]()
                                          { HandleAsyncTimeouts(); });
        }

        ~CPPClient()
        {
            // 注销接收线程
            connected_.store(false);
            destroyed_.store(true);
            timeout_cv_.notify_all();

            // 关闭
            if (client_socket_ != -1)
            {
#ifndef WIN32
                (void)shutdown(client_socket_, SHUT_RDWR);
#else
                (void)shutdown(client_socket_, SD_BOTH);
#endif
            }

            // 资源回收
            if (recv_thread_.joinable())
            {
                recv_thread_.join();
            }
            if (timeout_thread_.joinable())
            {
                timeout_thread_.join();
            }

            if (client_socket_ != -1)
            {
#ifndef WIN32
                close(client_socket_);
                client_socket_ = -1;
#else
                closesocket(client_socket_);
                WSACleanup();
                client_socket_ = -1;
#endif
            }

            {
                std::lock_guard<std::mutex> lock(resp_map_mutex_);
                for (auto &kv : pending_responses_)
                {
                    kv.second.valid_flag->store(false);
                    if (kv.second.promise)
                    {
                        try
                        {
                            kv.second.promise->set_value(core::Msg());
                        }
                        catch (...)
                        {
                        }
                    }
                }
                pending_responses_.clear();
            }
        }

        template <typename RES>
        std::pair<int, std::vector<RES>> CallAwait(const core::Msg &msg, const int64_t &timeout = 5000)
        {
            int ret = 0;
            std::vector<RES> resp_list{};

            if (client_socket_ == -1)
            {
                snprintf(error_info_, 1024, "connection invalid.");
                return {-1, resp_list};
            }

            // 注册 promise
            auto promise = std::make_shared<std::promise<core::Msg>>();
            auto valid_flag = std::make_shared<std::atomic<bool>>(true);
            auto future = promise->get_future();
            {
                std::lock_guard<std::mutex> lock(resp_map_mutex_);
                pending_responses_[msg.seqID()] = PendingItem{promise, valid_flag};
            }

            if (!SendMessage(msg, false))
            {
                snprintf(error_info_, 1024, "network error while send.");
                {
                    std::lock_guard<std::mutex> lock(resp_map_mutex_);
                    auto it = pending_responses_.find(msg.seqID());
                    if (it != pending_responses_.end())
                    {
                        it->second.valid_flag->store(false);
                        pending_responses_.erase(it);
                    }
                }
                return {-1, resp_list};
            }

            if (future.wait_for(std::chrono::milliseconds(timeout)) != std::future_status::ready)
            {
                {
                    std::lock_guard<std::mutex> lock(resp_map_mutex_);
                    auto it = pending_responses_.find(msg.seqID());
                    if (it != pending_responses_.end())
                    {
                        it->second.valid_flag->store(false);
                        pending_responses_.erase(it);
                    }
                }
                snprintf(error_info_, 1024, "RPC timeout for seqID %ld", msg.seqID());
                return {RPC_TIMEOUT_CODE, resp_list};
            }

            // 解析响应
            try
            {
                std::string res_str = future.get().toString();
                std::vector<RES> resp_array = nlohmann::json::parse(res_str).get<std::vector<RES>>();
                for (const auto &r : resp_array)
                {
                    resp_list.emplace_back(r);
                }
            }
            catch (const std::exception &e)
            {
                snprintf(error_info_, 1024, "JSON parse error: %s", e.what());
                return {-1, resp_list};
            }

            return {ret, resp_list};
        }

        std::pair<int, core::Msg> CallAwaitRaw(const core::Msg &msg, const int64_t &timeout = 5000)
        {
            int ret = 0;
            core::Msg resp_msg{};

            if (client_socket_ == -1)
            {
                snprintf(error_info_, 1024, "connection invalid.");
                return {-1, core::Msg{"connection invalid."}};
            }

            // 注册 promise
            auto promise = std::make_shared<std::promise<core::Msg>>();
            auto valid_flag = std::make_shared<std::atomic<bool>>(true);
            auto future = promise->get_future();
            {
                std::lock_guard<std::mutex> lock(resp_map_mutex_);
                pending_responses_[msg.seqID()] = PendingItem{promise, valid_flag};
            }

            if (!SendMessage(msg, false))
            {
                snprintf(error_info_, 1024, "network error while send.");
                {
                    std::lock_guard<std::mutex> lock(resp_map_mutex_);
                    auto it = pending_responses_.find(msg.seqID());
                    if (it != pending_responses_.end())
                    {
                        it->second.valid_flag->store(false);
                        pending_responses_.erase(it);
                    }
                }
                return {-1, resp_msg};
            }

            if (future.wait_for(std::chrono::milliseconds(timeout)) != std::future_status::ready)
            {
                {
                    std::lock_guard<std::mutex> lock(resp_map_mutex_);
                    auto it = pending_responses_.find(msg.seqID());
                    if (it != pending_responses_.end())
                    {
                        it->second.valid_flag->store(false);
                        pending_responses_.erase(it);
                    }
                }

                snprintf(error_info_, 1024, "RPC timeout for seqID %ld", msg.seqID());
                return {RPC_TIMEOUT_CODE, core::Msg{"RPC timeout for seqID: " + msg.seqID()}};
            }

            // 获取响应字符串
            resp_msg = future.get();

            return {ret, resp_msg};
        }

        template <typename RES>
        bool CallAsync(const core::Msg &msg,
                       const uint64_t &time_out,
                       std::function<void(int, const std::vector<RES> &)> cb,
                       bool expect_resp = true)
        {
            if (expect_resp && !cb)
            {
                snprintf(error_info_, 1024, "Callback is null.");
                return false;
            }

            if (client_socket_ == -1)
            {
                snprintf(error_info_, 1024, "connection invalid.");
                return false;
            }

            if (!expect_resp)
            {
                if (!SendMessage(msg, true))
                {
                    snprintf(error_info_, 1024, "Failed to send message.");
                    return false;
                }
                return true;
            }

            auto valid_flag = std::make_shared<std::atomic<bool>>(true);
            auto raw_cb = [cb](int ret, const core::Msg &resp_msg)
            {
                std::vector<RES> resp_list{};
                if (ret < 0)
                {
                    cb(ret, resp_list);
                    return;
                }

                try
                {
                    std::string res_str = resp_msg.toString();
                    std::vector<RES> resp_array = nlohmann::json::parse(res_str).get<std::vector<RES>>();
                    for (const auto &r : resp_array)
                    {
                        resp_list.emplace_back(r);
                    }
                }
                catch (const std::exception &e)
                {
                    std::cout << "Async response parse failed: " << e.what() << '\n';
                    ret = -1;
                }
                cb(ret, resp_list);
            };
            {
                std::lock_guard<std::mutex> lock(resp_map_mutex_);
                pending_responses_[msg.seqID()] = PendingItem{std::move(raw_cb), valid_flag,
                                                              std::chrono::steady_clock::now() + std::chrono::milliseconds(time_out)};
            }
            timeout_cv_.notify_one();

            if (!SendMessage(msg, true))
            {
                snprintf(error_info_, 1024, "Failed to send message.");
                {
                    std::lock_guard<std::mutex> lock(resp_map_mutex_);
                    pending_responses_.erase(msg.seqID());
                }
                std::vector<RES> resp_list{};
                cb(-1, resp_list);
                return false;
            }

            return true;
        }

        bool CallAsyncRaw(const core::Msg &msg,
                          const uint64_t &time_out,
                          std::function<void(int, const core::Msg &)> cb,
                          bool expect_resp = true)
        {
            if (expect_resp && !cb)
            {
                snprintf(error_info_, 1024, "Callback is null.");
                return false;
            }

            if (client_socket_ == -1)
            {
                snprintf(error_info_, 1024, "connection invalid.");
                cb(-1, core::Msg{"connection invalid."});
                return false;
            }

            if (!expect_resp)
            {
                if (!SendMessage(msg, true))
                {
                    snprintf(error_info_, 1024, "Failed to send message.");
                    return false;
                }
                return true;
            }

            auto valid_flag = std::make_shared<std::atomic<bool>>(true);
            {
                std::lock_guard<std::mutex> lock(resp_map_mutex_);
                pending_responses_[msg.seqID()] = PendingItem{std::move(cb), valid_flag,
                                                              std::chrono::steady_clock::now() + std::chrono::milliseconds(time_out)};
            }
            timeout_cv_.notify_one();

            if (!SendMessage(msg, true))
            {
                snprintf(error_info_, 1024, "Failed to send message.");
                std::function<void(int, const core::Msg &)> failed_cb;
                {
                    std::lock_guard<std::mutex> lock(resp_map_mutex_);
                    auto it = pending_responses_.find(msg.seqID());
                    if (it != pending_responses_.end())
                    {
                        failed_cb = std::move(it->second.callback);
                        pending_responses_.erase(it);
                    }
                }
                if (failed_cb)
                    failed_cb(-1, core::Msg{"Failed to send message."});
                return false;
            }

            return true;
        }

        // 设置recv回调函数
        void SetPushHandler(PushCallback cb)
        {
            push_handler_ = cb;
        }

        // 是否连接中
        bool IsConnected()
        {
            return connected_.load();
        }

        // 获取错误信息
        char *GetErrorInfo()
        {
            return error_info_;
        }

    private:
        bool SendMessage(const core::Msg &msg, bool nonblocking)
        {
#ifdef CODEIT_1_3_0
            const size_t total_size = msg.size() + sizeof(core::MsgHeader) + 1;
            auto send_data = std::make_unique<char[]>(total_size);
            memset(send_data.get(), 0x0, total_size);
            send_data.get()[0] = 0x22;
            memmove(send_data.get() + 1, reinterpret_cast<const char *>(&msg.header()),
                    msg.size() + sizeof(core::MsgHeader));
            return SendAll(send_data.get(), total_size, nonblocking);
#else
            return SendAll(reinterpret_cast<const char *>(&msg.header()),
                           msg.size() + sizeof(core::MsgHeader), nonblocking);
#endif
        }

        bool SendAll(const char *data, size_t size, bool nonblocking)
        {
            std::lock_guard<std::mutex> send_lock(send_mutex_);

#ifdef WIN32
            u_long old_mode = 0;
            u_long nonblock_mode = nonblocking ? 1 : 0;
            if (nonblocking && ioctlsocket(client_socket_, FIONBIO, &nonblock_mode) != 0)
            {
                return false;
            }
            const int flags = 0;
#else
            const int flags = nonblocking ? MSG_DONTWAIT : 0;
#endif

            size_t total_sent = 0;
            while (total_sent < size && !destroyed_.load())
            {
                const auto sent = send(client_socket_, data + total_sent, size - total_sent, flags);
                if (sent > 0)
                {
                    total_sent += static_cast<size_t>(sent);
                    continue;
                }
                if (sent == 0)
                    return false;

#ifdef WIN32
                const int err = WSAGetLastError();
                if (err != WSAEINTR && err != WSAEWOULDBLOCK)
                    return false;
#else
                if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                    return false;
#endif
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

#ifdef WIN32
            if (nonblocking)
            {
                old_mode = 0;
                ioctlsocket(client_socket_, FIONBIO, &old_mode);
            }
#endif

            return total_sent == size;
        }

        void HandleAsyncTimeouts()
        {
            while (!destroyed_.load())
            {
                std::vector<std::function<void(int, const core::Msg &)>> callbacks;
                {
                    std::unique_lock<std::mutex> lock(resp_map_mutex_);
                    timeout_cv_.wait_for(lock, std::chrono::milliseconds(100),
                                         [this]()
                                         { return destroyed_.load(); });
                    if (destroyed_.load())
                        break;

                    const auto now = std::chrono::steady_clock::now();
                    for (auto it = pending_responses_.begin(); it != pending_responses_.end();)
                    {
                        if (!it->second.callback || it->second.deadline > now)
                        {
                            ++it;
                            continue;
                        }

                        bool expected = true;
                        if (it->second.valid_flag &&
                            it->second.valid_flag->compare_exchange_strong(expected, false))
                        {
                            callbacks.emplace_back(std::move(it->second.callback));
                        }
                        it = pending_responses_.erase(it);
                    }
                }

                for (auto &cb : callbacks)
                {
                    thread_pool_.Enqueue([cb = std::move(cb)]() mutable
                                         { cb(RPC_TIMEOUT_CODE, core::Msg{}); });
                }
            }
        }

        // 连接服务端
        bool ConnectToServer()
        {
            client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
            if (client_socket_ == -1)
            {
                std::cerr << "Failed to create client socket." << std::endl;
                return false;
            }

            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port_);

#ifndef WIN32
            if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) != 1)
            {
                std::cout << "Invalid address / Unknown host" << std::endl;
                close(client_socket_);
                client_socket_ = -1;
                return false;
            }

            timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = 200000; // 200ms
            setsockopt(client_socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#else
            int iResult = InetPtonA(AF_INET, server_ip_.c_str(), &server_addr.sin_addr);
            if (iResult == 0)
            {
                std::cout << "Invalid address / Unknown host" << std::endl;
                closesocket(client_socket_);
                WSACleanup();
                return false;
            }
#endif

#ifndef WIN32
            int old_flags = fcntl(client_socket_, F_GETFL, 0);
            if (old_flags == -1 || fcntl(client_socket_, F_SETFL, old_flags | O_NONBLOCK) == -1)
            {
                std::cerr << "Failed to set client socket non-blocking." << std::endl;
                close(client_socket_);
                client_socket_ = -1;
                return false;
            }

            int connect_result = connect(client_socket_, (struct sockaddr *)&server_addr,
                                         sizeof(server_addr));
            if (connect_result == -1 && errno != EINPROGRESS)
            {
                close(client_socket_);
                client_socket_ = -1;
                return false;
            }

            if (connect_result == -1)
            {
                pollfd pfd{};
                pfd.fd = client_socket_;
                pfd.events = POLLOUT;

                int timeout_ms = connect_timeout_ms_ > 0 ? connect_timeout_ms_ : 2000;
                int poll_result = poll(&pfd, 1, timeout_ms);
                if (poll_result <= 0)
                {
                    close(client_socket_);
                    client_socket_ = -1;
                    return false;
                }

                int socket_error = 0;
                socklen_t len = sizeof(socket_error);
                if (getsockopt(client_socket_, SOL_SOCKET, SO_ERROR, &socket_error, &len) == -1 ||
                    socket_error != 0)
                {
                    close(client_socket_);
                    client_socket_ = -1;
                    return false;
                }
            }

            if (fcntl(client_socket_, F_SETFL, old_flags) == -1)
            {
                std::cerr << "Failed to restore client socket flags." << std::endl;
                close(client_socket_);
                client_socket_ = -1;
                return false;
            }
#else
            if (connect(client_socket_, (struct sockaddr *)&server_addr,
                        sizeof(server_addr)) == -1)
            {
                closesocket(client_socket_);
                client_socket_ = -1;
                return false;
            }
#endif
            connected_.store(true);

            // 启动接收线程
            recv_thread_ = std::thread(
                [this]()
                {
                CPP_RPC_LOG("recv thread is running ...\n");

                while (!destroyed_.load()) {
                    core::Msg recv_msg;
                    recv_msg.resize(1024);

#ifdef CODEIT_1_3_0
                    // 接收头部标志
                    char head_flag = 0xff;
                    int ret = safe_recv(client_socket_, reinterpret_cast<char*>(&head_flag), 1, destroyed_);
                    if (ret <= 0) {
                        // 主动析构退出
                        if (ret == -2)
                        {
                            CPP_RPC_LOG("recv thread stopping (destroyed).\n");
                            break;
                        }

                        // 对端关闭连接
                        if (ret == 0)
                        {
                            CPP_RPC_LOG("peer closed connection.\n");
                            break;
                        }

                        printf("[header-flag] head data size:%d, id:%d, type:%lu, re1:%" PRId64 ", re2:%" PRId64 ", re3:%" PRId64 "\n",
                                recv_msg.header().msg_size_,
                                recv_msg.header().msg_id_,
                                recv_msg.header().msg_type_,
                                recv_msg.header().reserved1_,
                                recv_msg.header().reserved2_,
                                recv_msg.header().reserved3_);

#ifndef WIN32
                        CPP_RPC_LOG("[header-flag] safe_recv fatal error(ret:%d errno:%d)\n",ret, errno);
#else
                        CPP_RPC_LOG("[header-flag] safe_recv fatal error(ret:%d wsa:%d)\n", ret, WSAGetLastError());
#endif
                        break;
                    }
#endif
                    // 接收消息头
                    int header_recv = safe_recv(client_socket_, reinterpret_cast<char*>(&recv_msg.header()), sizeof(core::MsgHeader), destroyed_);
                    if (header_recv <= 0) {
                        printf("[header] head data size:%d, id:%d, type:%lu, re1:%" PRId64 ", re2:%" PRId64 ", re3:%" PRId64 "\n",
                                recv_msg.header().msg_size_,
                                recv_msg.header().msg_id_,
                                recv_msg.header().msg_type_,
                                recv_msg.header().reserved1_,
                                recv_msg.header().reserved2_,
                                recv_msg.header().reserved3_);
                        if (!destroyed_.load()) {
                            CPP_RPC_LOG("[header] safe_recv failed(ret:%d, errno:%d), closing socket.\n", header_recv, errno);
                        }
                        break;
                    }

                    // 接收消息体
                    size_t body_size = recv_msg.size();
                    if (body_size > 16 * 1024 * 1024)
                    {
                        CPP_RPC_LOG("bad packet size=%zu\n", body_size);
                        break;
                    }
                    recv_msg.resize(body_size);
                    if (recv_msg.size() > 0) {
                        int body_recv = safe_recv(client_socket_, recv_msg.data(), recv_msg.size(), destroyed_);
                        if (body_recv <= 0) {
                            printf("[body] head data size:%d, id:%d, type:%lu, re1:%" PRId64 ", re2:%" PRId64 ", re3:%" PRId64 "\n",
                                recv_msg.header().msg_size_,
                                recv_msg.header().msg_id_,
                                recv_msg.header().msg_type_,
                                recv_msg.header().reserved1_,
                                recv_msg.header().reserved2_,
                                recv_msg.header().reserved3_);
                            if (!destroyed_.load()) {
                                CPP_RPC_LOG("[body] safe_recv failed(ret:%d, errno:%d)\n", body_recv, errno);
                            }
                            break;
                        }
                    }

                    if(push_handler_)  {
                        push_handler_(recv_msg);
                        continue;
                    }

                    {
                        const auto seq_id = recv_msg.seqID();
                        PendingItem item;
                        {
                            std::lock_guard<std::mutex> lock(resp_map_mutex_);
                            auto it = pending_responses_.find(seq_id);
                            if (it != pending_responses_.end()) {
                                item = it->second;
                                pending_responses_.erase(it);
                            }
                        }

                        bool expected = true;
                        if (item.valid_flag && item.valid_flag->compare_exchange_strong(expected, false)) {
                            if (item.callback) {
                                thread_pool_.Enqueue(
                                    [cb = std::move(item.callback), recv_msg]() mutable {
                                        cb(0, recv_msg);
                                    });
                            } else if (item.promise) {
                                try {
                                    item.promise->set_value(std::move(recv_msg));
                                } catch (const std::future_error& e) {
                                    std::cerr << "set_value error (probably already satisfied): " << e.what() << '\n';
                                }
                            }
                        } else {
                            std::cout << "No valid pending response for seqID: " << seq_id << '\n';
                        }
                    }
                }

                connected_.store(false);
                CPP_RPC_LOG("recv thread exit.\n"); });

            return true;
        }

    private:
        std::string server_ip_; // 服务端ip
        int server_port_;
        int client_socket_;
        int connect_timeout_ms_;

        static constexpr size_t BUFFER_SIZE = 1024;
        ThreadPool thread_pool_{4}; // 仅执行已就绪的异步回调，不等待网络响应
        std::condition_variable resp_cv_;
        std::thread recv_thread_;            // 客户端接收回报线程
        std::thread timeout_thread_;         // 异步请求截止时间检查线程
        std::atomic<bool> destroyed_{false}; // 客户端存活状态
        PushCallback push_handler_ = nullptr;

        std::mutex send_mutex_;
        mutable std::mutex resp_map_mutex_;
        std::condition_variable timeout_cv_;
        std::unordered_map<int64_t, PendingItem> pending_responses_;

        std::atomic<bool> connected_{false};

        char error_info_[1024]{0x0}; // 存储最近一次错误信息
    };
}

#endif
