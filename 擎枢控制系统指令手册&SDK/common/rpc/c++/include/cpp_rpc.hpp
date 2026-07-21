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
#include "util/reflection/json.hpp"
#include "util/reflection/easy_json.h"

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/uio.h>
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
#include <algorithm>
#include <regex>
#include "task_pool.hpp"
#include "msg.hpp"

namespace cpp_rpc
{

    constexpr int UNIQUE_CLEAR_ERR_ID = 198808;
    constexpr int RPC_ERROR_CODE = -1;
    constexpr int RPC_TIMEOUT_CODE = -3;

    // 连接阶段错误码
    constexpr int CONNECT_OK = 0;
    constexpr int CONNECT_ERR_SOCKET = -101;        // socket() 创建失败
    constexpr int CONNECT_ERR_INVALID_ADDR = -102;  // IP地址无效
    constexpr int CONNECT_ERR_FCNTL = -103;         // fcntl 设置非阻塞失败
    constexpr int CONNECT_ERR_CONNECT = -104;       // connect 立即失败
    constexpr int CONNECT_ERR_TIMEOUT = -105;       // poll 连接超时
    constexpr int CONNECT_ERR_REFUSED = -106;       // SO_ERROR 连接被拒绝
    constexpr int CONNECT_ERR_FCNTL_RESTORE = -107; // fcntl 恢复socket标志失败

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

        // 根据 fd 反查客户端 IP，用于错误日志
        std::string GetClientIPByFD(int fd)
        {
            std::lock_guard<std::mutex> lk(mtx4clientlist_);
            for (auto &[ip, f] : client_list_)
                if (f == fd)
                    return ip;
            return "unknown";
        }

        // [主动发起] 发送消息给指定 ip
        // 根据ip找到 fd句柄
        bool SendAwaitViaIp(std::string ip, core::Msg &msg)
        {
            int target_fd = -1;
            {
                std::lock_guard<std::mutex> lk(mtx4clientlist_);
                // 遍历查找匹配该 IP 的任意连接（key 格式为 "ip:port"）
                for (auto &[key, fd] : client_list_)
                {
                    if (key.compare(0, ip.size(), ip) == 0 &&
                        (key.size() == ip.size() || key[ip.size()] == ':'))
                    {
                        target_fd = fd;
                        break;
                    }
                }
                if (target_fd == -1)
                {
                    std::cout << "client: " << ip
                              << " not found, please check the connection is ok."
                              << std::endl;
                    return false;
                }
            } // 锁释放，fd 生命周期由 epoll 线程管理，不会在此期间失效

            // 设置固定回报id
            msg.setMsgID(8888);

            return SendAwaitViaFD(target_fd, msg);
        }

        // [被动响应] 发送消息对端client
        // 根据fd句柄
        // 消息队列 + writev 批量发送：
        // callback 线程只做入队（per-fd 锁，持有时间极短 ~1μs），
        // 专用 sender 线程 drain 队列后 writev 批量写入，避免全局锁竞争。
        bool SendAwaitViaFD(int fd, core::Msg &msg)
        {
#ifdef CODEIT_1_3_0
            const size_t total_size = msg.size() + sizeof(core::MsgHeader) + 1;
            auto send_data = std::make_unique<char[]>(total_size);
            memset(send_data.get(), 0x0, total_size);
            send_data.get()[0] = 0x22;
            memmove(send_data.get() + 1, reinterpret_cast<const char *>(&msg.header()),
                    msg.size() + sizeof(core::MsgHeader));
#else
            const size_t total_size = msg.size() + sizeof(core::MsgHeader);
            auto send_data = std::make_unique<char[]>(total_size);
            memmove(send_data.get(), reinterpret_cast<const char *>(&msg.header()), total_size);
#endif
            // 入队：per-fd 锁，持有时间极短
            {
                std::lock_guard<std::mutex> lk(send_queue_mtx_);
                auto &q = send_queues_[fd];
                auto &buf = q.buf;
                buf.insert(buf.end(), send_data.get(), send_data.get() + total_size);
            }
            send_queue_cv_.notify_one();
            return true;
        }

    private:
        // 连接读取状态（用于 EPOLLET 非阻塞循环读取）
        enum class ConnReadState
        {
            READING_FLAG,
            READING_HEADER,
            READING_BODY
        };

        struct ConnBuffer
        {
#ifdef CODEIT_1_3_0
            ConnReadState state{ConnReadState::READING_FLAG};
            size_t expected_size{1}; // flag 阶段期望 1 字节
#else
            ConnReadState state{ConnReadState::READING_HEADER};
            size_t expected_size{sizeof(core::MsgHeader)}; // header 阶段
#endif
            std::string buf;             // 累积的未处理数据
            core::Msg partial_msg{0, 0}; // 半成品消息（header已解析）

            ConnBuffer() = default;
            ConnBuffer(ConnBuffer &&) = default;
            ConnBuffer &operator=(ConnBuffer &&) = default;
        };

        // per-fd 发送队列
        struct SendQueue
        {
            std::vector<char> buf; // 待发送的原始字节流
        };

        // 非阻塞读取：尽可能读取所有数据并解析消息，返回解析出的消息列表
        std::vector<core::Msg> NonBlockingRecvAndParse(int client_socket, ConnBuffer &conn);

        // 发送线程：drain 所有 per-fd 队列，writev 批量写入
        void SenderLoop();
        // drain 单个 fd 的发送队列，返回 false 表示连接已断开
        bool DrainFd(int fd);

        int server_port_{-1};
        int listen_socket_{-1};
#ifndef WIN32
        int epoll_fd_;
#else
        HANDLE epoll_fd_;
#endif
        std::unordered_map<std::string, int> client_list_;
        std::mutex mtx4clientlist_;
        std::unordered_map<int, ConnBuffer> conn_buffers_; // 每连接读缓冲
        std::mutex mtx4connbuffers_;

        // 消息队列 + writev 发送（替代旧的全局 mtx4send_）
        std::unordered_map<int, SendQueue> send_queues_; // per-fd 发送队列
        std::mutex send_queue_mtx_;                      // 保护 send_queues_
        std::condition_variable send_queue_cv_;          // 通知 sender 线程
        std::atomic<bool> sender_running_{false};
        std::thread sender_thread_;

    public:
        // 获取所有 fd 的队列总深度（供统计用）
        size_t GetSendQueueDepth()
        {
            std::lock_guard<std::mutex> lk(send_queue_mtx_);
            size_t total = 0;
            for (auto &[fd, q] : send_queues_)
            {
                total += q.buf.size();
            }
            return total;
        }

        static constexpr size_t BUFFER_SIZE = 1024;
        std::function<void(int)> new_connection_callback;
        std::function<void(int)> disconnection_callback;
        std::function<void(int, const core::Msg &)> receive_data_callback;
        ThreadPool callback_pool_{4}; // 回调线程池
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

#if 0
        static auto safe_recv(decltype(socket(AF_INET, SOCK_STREAM, 0)) s, char *data,
                              int size) -> int
        {
            int result{0};
            while (result < size)
            {
                int ret = recv(s, data + result, size - result, 0);
                if (ret <= 0)
                {
                    return ret; // 返回错误码（0或负值）
                }
                result += ret;
            }
            return result; // 全部收到size字节
        }
#endif

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
                    std::this_thread::yield(); // 缓冲区空，让出 CPU 等待新数据
                    continue;
                }
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                {
                    std::this_thread::yield(); // 缓冲区空，让出 CPU 等待新数据
                    continue;
                }
#endif
                // 真错误
                return -1;
            }
            return result;
        }

        CPPClient(const std::string &ip, int port, int connect_timeout_ms = 2000,
                  PushCallback push_handler = nullptr, int send_buffer_size = 0)
            : server_ip_(ip), server_port_(port), client_socket_(-1),
              connect_timeout_ms_(connect_timeout_ms), send_buffer_size_(send_buffer_size),
              destroyed_(false)
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
            if (ConnectToServer() != CONNECT_OK)
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
                return {RPC_TIMEOUT_CODE, core::Msg{std::string("RPC timeout for seqID: ") + std::to_string(msg.seqID())}};
            }

            // 获取响应字符串
            resp_msg = future.get();

            return {ret, resp_msg};
        }

        template <typename RES>
        bool CallAsync(const core::Msg &msg,
                       const uint64_t &time_out = std::numeric_limits<uint64_t>::max(),
                       std::function<void(int, const std::vector<RES> &)> cb = nullptr)
        {
            if (client_socket_ == -1)
            {
                snprintf(error_info_, 1024, "connection invalid.");
                return false;
            }

            if (!cb)
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
                          const uint64_t &time_out = std::numeric_limits<uint64_t>::max(),
                          std::function<void(int, const core::Msg &)> cb = nullptr)
        {
            if (client_socket_ == -1)
            {
                snprintf(error_info_, 1024, "connection invalid.");
                if (cb)
                    cb(-1, core::Msg{"connection invalid."});
                return false;
            }

            if (!cb)
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

        // 获取最近一次连接错误码（0=成功, <0=具体错误）
        int GetConnectErrorCode() const
        {
            return connect_error_code_;
        }

        // 根据错误码获取可读的错误描述
        static const char *GetErrorMsg(int code)
        {
            switch (code)
            {
            case CONNECT_OK:
                return "连接成功";
            case CONNECT_ERR_SOCKET:
                return "创建socket失败";
            case CONNECT_ERR_INVALID_ADDR:
                return "IP地址无效或主机不存在";
            case CONNECT_ERR_FCNTL:
                return "设置socket非阻塞模式失败";
            case CONNECT_ERR_CONNECT:
                return "连接服务器失败";
            case CONNECT_ERR_TIMEOUT:
                return "连接服务器超时";
            case CONNECT_ERR_REFUSED:
                return "连接被服务器拒绝";
            case CONNECT_ERR_FCNTL_RESTORE:
                return "恢复socket标志失败";
            case RPC_TIMEOUT_CODE:
                return "RPC请求超时";
            case RPC_ERROR_CODE:
                return "RPC通用错误";
            default:
                return "未知错误";
            }
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

#if 1
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
                {
                    RPC_ERROR_LOG("type=send_failed | fd=%d | target=%s:%d | errno=%d(%s) | msg=send返回0，对端关闭连接",
                                  client_socket_, server_ip_.c_str(), server_port_, errno, errno_name(errno));
                    return false;
                }

#ifdef WIN32
                const int err = WSAGetLastError();
                if (err != WSAEINTR && err != WSAEWOULDBLOCK)
                {
                    RPC_ERROR_LOG("type=send_failed | fd=%d | target=%s:%d | err=%d(%s) | msg=发送数据出错",
                                  client_socket_, server_ip_.c_str(), server_port_, err, errno_name(err));
                    return false;
                }
#else
                if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    RPC_ERROR_LOG("type=send_failed | fd=%d | target=%s:%d | errno=%d(%s) | msg=发送数据出错",
                                  client_socket_, server_ip_.c_str(), server_port_, errno, errno_name(errno));
                    return false;
                }
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
#else

        bool SendAll(const char *data, size_t size, bool nonblocking)
        {
            using Clock = std::chrono::steady_clock;

            const auto enter_tp = Clock::now();
            std::unique_lock<std::mutex> send_lock(send_mutex_);
            const auto locked_tp = Clock::now();

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
            uint64_t would_block_count = 0;

            while (total_sent < size && !destroyed_.load())
            {
                const auto sent = send(client_socket_,
                                       data + total_sent,
                                       size - total_sent,
                                       flags);

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

                if (err == WSAEWOULDBLOCK)
                    ++would_block_count;
#else
                if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                    return false;

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    ++would_block_count;
#endif

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            const auto end_tp = Clock::now();

            const auto lock_wait_us =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    locked_tp - enter_tp)
                    .count();

            const auto holding_lock_us =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    end_tp - locked_tp)
                    .count();

            const auto total_us =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    end_tp - enter_tp)
                    .count();

            if (total_us > 10000)
            {
                std::cout << "[SEND_BLOCK]"
                          << " size=" << size
                          << " lock_wait_us=" << lock_wait_us
                          << " holding_lock_us=" << holding_lock_us
                          << " would_block=" << would_block_count
                          << " sent=" << total_sent
                          << " total_us=" << total_us
                          << std::endl;
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
#endif

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

        // 连接服务端（返回 0=成功, <0=错误码，通过 GetErrorMsg() 获取具体描述）
        int ConnectToServer()
        {
            client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
            if (client_socket_ == -1)
            {
                connect_error_code_ = CONNECT_ERR_SOCKET;
                snprintf(error_info_, sizeof(error_info_), "Failed to create socket, errno=%d(%s)", errno, errno_name(errno));
                RPC_ERROR_LOG("type=socket_failed | target=%s:%d | errno=%d(%s) | msg=创建客户端socket失败",
                              server_ip_.c_str(), server_port_, errno, errno_name(errno));
                return CONNECT_ERR_SOCKET;
            }

            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port_);

#ifndef WIN32
            if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) != 1)
            {
                connect_error_code_ = CONNECT_ERR_INVALID_ADDR;
                snprintf(error_info_, sizeof(error_info_), "Invalid IP address: %s", server_ip_.c_str());
                RPC_ERROR_LOG("type=invalid_address | target=%s:%d | msg=IP地址无效或主机不存在",
                              server_ip_.c_str(), server_port_);
                close(client_socket_);
                client_socket_ = -1;
                return CONNECT_ERR_INVALID_ADDR;
            }

            timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = 200000; // 200ms
            setsockopt(client_socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            if (send_buffer_size_ > 0)
            {
                int sndbuf = send_buffer_size_;
                setsockopt(client_socket_, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
            }
#else
            int iResult = InetPtonA(AF_INET, server_ip_.c_str(), &server_addr.sin_addr);
            if (iResult == 0)
            {
                connect_error_code_ = CONNECT_ERR_INVALID_ADDR;
                snprintf(error_info_, sizeof(error_info_), "Invalid IP address: %s", server_ip_.c_str());
                closesocket(client_socket_);
                WSACleanup();
                return CONNECT_ERR_INVALID_ADDR;
            }

            if (send_buffer_size_ > 0)
            {
                int sndbuf = send_buffer_size_;
                setsockopt(client_socket_, SOL_SOCKET, SO_SNDBUF,
                           reinterpret_cast<const char *>(&sndbuf), sizeof(sndbuf));
            }
#endif

#ifndef WIN32
            int old_flags = fcntl(client_socket_, F_GETFL, 0);
            if (old_flags == -1 || fcntl(client_socket_, F_SETFL, old_flags | O_NONBLOCK) == -1)
            {
                connect_error_code_ = CONNECT_ERR_FCNTL;
                snprintf(error_info_, sizeof(error_info_), "Failed to set socket non-blocking, errno=%d(%s)", errno, errno_name(errno));
                RPC_ERROR_LOG("type=fcntl_failed | fd=%d | target=%s:%d | errno=%d(%s) | msg=设置非阻塞模式失败",
                              client_socket_, server_ip_.c_str(), server_port_, errno, errno_name(errno));
                close(client_socket_);
                client_socket_ = -1;
                return CONNECT_ERR_FCNTL;
            }

            int connect_result = connect(client_socket_, (struct sockaddr *)&server_addr,
                                         sizeof(server_addr));
            if (connect_result == -1 && errno != EINPROGRESS)
            {
                connect_error_code_ = CONNECT_ERR_CONNECT;
                snprintf(error_info_, sizeof(error_info_), "Connect failed immediately, errno=%d(%s)", errno, errno_name(errno));
                RPC_ERROR_LOG("type=connect_failed | fd=%d | target=%s:%d | errno=%d(%s) | msg=连接立即失败",
                              client_socket_, server_ip_.c_str(), server_port_, errno, errno_name(errno));
                close(client_socket_);
                client_socket_ = -1;
                return CONNECT_ERR_CONNECT;
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
                    connect_error_code_ = CONNECT_ERR_TIMEOUT;
                    snprintf(error_info_, sizeof(error_info_), "Connect timeout after %dms, errno=%d(%s)", timeout_ms, errno, errno_name(errno));
                    RPC_ERROR_LOG("type=connect_timeout | fd=%d | target=%s:%d | timeout=%dms | errno=%d(%s) | msg=连接超时或poll出错",
                                  client_socket_, server_ip_.c_str(), server_port_, timeout_ms, errno, errno_name(errno));
                    close(client_socket_);
                    client_socket_ = -1;
                    return CONNECT_ERR_TIMEOUT;
                }

                int socket_error = 0;
                socklen_t len = sizeof(socket_error);
                if (getsockopt(client_socket_, SOL_SOCKET, SO_ERROR, &socket_error, &len) == -1 ||
                    socket_error != 0)
                {
                    connect_error_code_ = CONNECT_ERR_REFUSED;
                    snprintf(error_info_, sizeof(error_info_), "Connection refused, so_error=%d(%s)", socket_error, errno_name(socket_error));
                    RPC_ERROR_LOG("type=connect_failed | fd=%d | target=%s:%d | so_error=%d(%s) | msg=连接被拒绝(poll就绪但SO_ERROR≠0)",
                                  client_socket_, server_ip_.c_str(), server_port_, socket_error, errno_name(socket_error));
                    close(client_socket_);
                    client_socket_ = -1;
                    return CONNECT_ERR_REFUSED;
                }
            }

            if (fcntl(client_socket_, F_SETFL, old_flags) == -1)
            {
                connect_error_code_ = CONNECT_ERR_FCNTL_RESTORE;
                snprintf(error_info_, sizeof(error_info_), "Failed to restore socket flags, errno=%d(%s)", errno, errno_name(errno));
                RPC_ERROR_LOG("type=fcntl_failed | fd=%d | target=%s:%d | errno=%d(%s) | msg=恢复socket标志失败",
                              client_socket_, server_ip_.c_str(), server_port_, errno, errno_name(errno));
                close(client_socket_);
                client_socket_ = -1;
                return CONNECT_ERR_FCNTL_RESTORE;
            }
#else
            if (connect(client_socket_, (struct sockaddr *)&server_addr,
                        sizeof(server_addr)) == -1)
            {
                int err = WSAGetLastError();
                connect_error_code_ = CONNECT_ERR_CONNECT;
                snprintf(error_info_, sizeof(error_info_), "Windows connect failed, err=%d(%s)", err, errno_name(err));
                RPC_ERROR_LOG("type=connect_failed | target=%s:%d | err=%d(%s) | msg=Windows connect失败",
                              server_ip_.c_str(), server_port_, err, errno_name(err));
                closesocket(client_socket_);
                client_socket_ = -1;
                return CONNECT_ERR_CONNECT;
            }
#endif
            connected_.store(true);
            connect_error_code_ = CONNECT_OK;

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

                        // 对端关闭连接（或本端 shutdown 触发）
                        if (ret == 0)
                        {
                            RPC_ERROR_LOG("type=connection_closed | fd=%d | target=%s:%d | msg=连接已关闭(header-flag读取时)",
                                          client_socket_, server_ip_.c_str(), server_port_);
                            break;
                        }

                        RPC_ERROR_LOG("type=recv_fatal | fd=%d | target=%s:%d | errno=%d(%s) | msg=连接断开(header-flag读取时出错)",
                                      client_socket_, server_ip_.c_str(), server_port_, errno, errno_name(errno));
                        break;
                    }
#endif
                    // 接收消息头
                    int header_recv = safe_recv(client_socket_, reinterpret_cast<char*>(&recv_msg.header()), sizeof(core::MsgHeader), destroyed_);
                    if (header_recv <= 0) {
                        if (!destroyed_.load()) {
                            const char *reason = (header_recv == 0) ? "对端关闭连接(读消息头时)" : "连接断开(读消息头时出错)";
                            RPC_ERROR_LOG("type=recv_fatal | fd=%d | target=%s:%d | ret=%d | errno=%d(%s) | msg=%s",
                                          client_socket_, server_ip_.c_str(), server_port_,
                                          header_recv, errno, errno_name(errno), reason);
                        }
                        break;
                    }
            
                    // 接收消息体
                    size_t body_size = recv_msg.size();
                    if (body_size > 16 * 1024 * 1024)
                    {
                        RPC_ERROR_LOG("type=bad_packet | fd=%d | target=%s:%d | body_size=%zu | msg=包体大小超过16MB限制",
                                      client_socket_, server_ip_.c_str(), server_port_, body_size);
                        break;
                    }
                    // 防止 Msg::resize() 按被 header 覆盖后的 size() 拷贝旧缓冲，导致读越界。
                    recv_msg.header().msg_size_ = 0;
                    if (recv_msg.resize(body_size) < 0)
                    {
                        RPC_ERROR_LOG("type=resize_failed | fd=%d | target=%s:%d | body_size=%zu | msg=消息resize失败，可能是内存不足",
                                      client_socket_, server_ip_.c_str(), server_port_, body_size);
                        break;
                    }
                    if (recv_msg.size() > 0) {
                        int body_recv = safe_recv(client_socket_, recv_msg.data(), recv_msg.size(), destroyed_);
                        if (body_recv <= 0) {
                            if (!destroyed_.load()) {
                                const char *reason = (body_recv == 0) ? "对端关闭连接(读消息体时)" : "连接断开(读消息体时出错)";
                                RPC_ERROR_LOG("type=recv_fatal | fd=%d | target=%s:%d | ret=%d | errno=%d(%s) | msg_id=%u | seq_id=%ld | msg=%s",
                                              client_socket_, server_ip_.c_str(), server_port_,
                                              body_recv, errno, errno_name(errno),
                                              recv_msg.header().msg_id_, recv_msg.header().reserved1_, reason);
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
                        bool found_in_map = false;
                        size_t pending_size = 0;
                        {
                            std::lock_guard<std::mutex> lock(resp_map_mutex_);
                            pending_size = pending_responses_.size();
                            auto it = pending_responses_.find(seq_id);
                            if (it != pending_responses_.end()) {
                                item = it->second;
                                pending_responses_.erase(it);
                                found_in_map = true;
                            }
                        }

                        if (!found_in_map)
                        {
                            // seqID 找不到：通常是响应到达太晚，客户端已判定超时
                            RPC_ERROR_LOG("type=late_response | fd=%d | target=%s:%d | seq_id=%ld | pending_size=%zu | msg=响应超时后到达，seqID不在待处理表中",
                                          client_socket_, server_ip_.c_str(), server_port_,
                                          seq_id, pending_size);
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
                                    RPC_ERROR_LOG("type=set_value_error | fd=%d | target=%s:%d | seq_id=%ld | msg=%s",
                                                  client_socket_, server_ip_.c_str(), server_port_,
                                                  seq_id, e.what());
                                }
                            }
                        } else {
                            // std::cout << "No valid pending response for seqID: " << seq_id << '\n';
                        }
                    }
                }
            
                connected_.store(false);
                CPP_RPC_LOG("recv thread exit.\n"); });

            return CONNECT_OK;
        }

    private:
        std::string server_ip_; // 服务端ip
        int server_port_;
        int client_socket_;
        int connect_timeout_ms_;
        int send_buffer_size_;

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

        char error_info_[1024]{0x0};         // 存储最近一次错误信息
        int connect_error_code_{CONNECT_OK}; // 存储最近一次连接错误码
    };
}

#endif
