/*
 * Copyright (c) 2024 by YB
 * All Rights Reserved. 
 *  
 *  
 * @Version: 0.0.1
 * @Author: yangbo
 * @Date: 2024-09-03 16:47:08
 * @LastEditors: yangbo
 * @LastEditTime: 2024-09-18 16:35:21
 * @FilePath: /maint_platform/maint_platform_srv/util/cpp-rpc/inc/cpp_rpc.hpp
 * @Description: 通信工具类（包含 客户端/服务端）
 */
#ifndef CPP_RPC_HPP
#define CPP_RPC_HPP

// Platform sockets before <future>: on MSVC, <future> may include windows.h and
// break winsock2 if winsock2 is not included first.
#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include "wepoll.h"
#include <crtdefs.h>
typedef SSIZE_T ssize_t;
#endif

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <functional>
#include <utility>
#include "json.hpp"
#include "easy_json.h"
#include <future>

#include <mutex>
#include <thread>
#include <condition_variable>
#include <string.h>
#include <tuple>
#include <atomic>
#include <unordered_map>
#include <regex>
#include <queue>
#include "util.hpp"
#include "rpc_msg.hpp"
//#include "codeit/core/log.hpp"

namespace cpp_rpc
{

    class ThreadPool {
    public:
        ThreadPool(size_t num_threads);

        ~ThreadPool();

        template <typename Func, typename... Args>
        void Enqueue(Func&& func, Args&&... args) {
            std::function<void()> task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                tasks_.emplace(std::move(task));
            }

            condition_.notify_one();
        }

    private:
        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        bool stop;
    };

    ThreadPool::ThreadPool(size_t num_threads)
        : stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] { return stop || !tasks_.empty(); });

                        if (stop && tasks_.empty())
                            return;

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    // 执行任务
                    task();
                }
             });
        }
    }

    ThreadPool::~ThreadPool() {

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop = true;
        }
        condition_.notify_all();

        for (std::thread& worker : workers_) {
            worker.join();
        }
    }

const int UNIQUE_CLEAR_ERR_ID = 198808;

class CPP_RPC_EXPORT CPPServer {
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
        void SetReceiveDataCallback(std::function<void(int, const core::Msg&)> callback);

        // TODO: 获取当前连接映射 <ip, fd> 'backup for safety'
        std::unordered_map<std::string, int> GetConnectedClients() 
		{
            std::unique_lock<std::mutex> lk(mtx4clientlist_);
            return client_list_;
        }

        // [主动发起] 发送消息给指定 ip
        // 根据ip找到 fd句柄
        bool SendAwaitViaIp(std::string ip, core::Msg& msg) 
		{

            //std::cout << "find ip:" << ip << ", map size: " << client_list_.size() << ", addr:" << &client_list_ << std::endl;
            // dump
            // std::unique_lock<std::mutex> lk(mtx4clientlist_);
            // auto iter =  client_list_.begin();
            // for(; iter != client_list_.end(); ++iter) {
            //     std::cout << " ==== " << iter->first << "," << iter->second << std::endl;
            // }
            // end

            auto it = client_list_.find(ip);
            if (it == client_list_.end()) 
			{
                std::cout << "client: " << ip << " not found, please check the connection is ok." << std::endl;
                return false;
            }

            // 设置固定回报id
            msg.setMsgID(8888);

            // 找到对应的客户端
            ssize_t sent = send(it->second, reinterpret_cast<const char*>(&msg.header()), msg.size() + sizeof(core::MsgHeader), 0);
            if (sent == -1) 
			{
                std::cerr << "Failed to send message." << std::endl;
                return false;
            }

            return true;
        }

        // [被动响应] 发送消息对端client
        // 根据fd句柄
        bool SendAwaitViaFD(int fd, core::Msg& msg) 
		{

            // 找到对应的客户端
            ssize_t sent = send(fd, reinterpret_cast<const char*>(&msg.header()), msg.size() + sizeof(core::MsgHeader), 0);
            if (sent == -1) 
			{
                std::cerr << "Failed to send message." << std::endl;
                return false;
            }

            return true;
        }

    private:
        int server_port_{ -1 };
        int listen_socket_{ -1 };
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
        std::function<void(int, const core::Msg&)> receive_data_callback;
    };

    struct CommResp
    {
        int32_t return_code;
        int32_t subcmd_index;
        std::string return_message;
        JSON_HELP(return_code, subcmd_index, return_message);
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommResp, return_code, subcmd_index, return_message)

    // 客户端类
    class CPP_RPC_EXPORT CPPClient
    {
        struct PendingItem
        {
            std::shared_ptr<std::promise<std::string>> promise;
            std::shared_ptr<std::atomic<bool>> valid_flag;
            PendingItem() = default;
            PendingItem(std::shared_ptr<std::promise<std::string>> p,
                        std::shared_ptr<std::atomic<bool>> v)
                : promise(std::move(p)), valid_flag(std::move(v)) {}
        };

    public:
        // 接收指定大小数据包
        static auto safe_recv(int s, char *data, int size) -> int
        {
            int result{0};
            while (result < size)
            {
                int ret = ::recv(s, data + result, size - result, 0);
                if (ret <= 0)
                {
                    return ret; // 返回错误码（0或负值）
                }
                result += ret;
            }
            return result; // 全部收到size字节
        }

        CPPClient(const std::string& ip, int server_port) 
			: server_ip_(ip),
            server_port_(server_port),
            client_socket_(-1),
            destroyed_(false) 
		{
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
            CPP_RPC_LOG("!!! failed to connect to server.\n");
            return ;
        }
    }

        ~CPPClient() 
		{
            // SPD_LOG_INFO("CPPClient::~CPPClient\n");

            // 注销接收线程
            destroyed_.store(true);

            // 关闭
            if (client_socket_ != -1) 
			{
#ifndef WIN32
                (void)shutdown(client_socket_, SHUT_RDWR);
#else
                (void)shutdown(client_socket_, SD_BOTH);
                WSACleanup();
#endif
            }

            // 资源回收
            if (recv_thread_.joinable()) 
			{
                recv_thread_.join();
        }
        }

        // 同步发送消息 (限定: 命令列表中不包含SubLoop)
        template<typename RES>
        std::pair<int, std::vector<RES>> CallAwait(const core::Msg& msg, const int64_t& timeout = 5000) 
		{

            int ret = 0;
            std::vector<RES> resp_list{};

            if (client_socket_ == -1)
            {
                snprintf(error_info_, 1024, "connection invalid.");
                return {-1, resp_list};
            }

            // 注册 promise
            auto promise = std::make_shared<std::promise<std::string>>();
            auto valid_flag = std::make_shared<std::atomic<bool>>(true);
            auto future = promise->get_future();
            {
                std::lock_guard<std::mutex> lock(resp_map_mutex_);
                pending_responses_[msg.seqID()] = PendingItem{promise, valid_flag};
            }
        // 发送消息
        ssize_t sent = 0;
#ifdef CODEIT_1_3_0
            auto send_data = std::make_unique<char[]>(msg.size() + sizeof(core::MsgHeader) + 1);
            memset(send_data.get(), 0x0, msg.size() + sizeof(core::MsgHeader) + 1);
            send_data.get()[0] = 0x22; // 合法消息标识头
            memmove(send_data.get() + 1, reinterpret_cast<const char *>(&msg.header()), msg.size() + sizeof(core::MsgHeader));
            std::cout << ">>>" << sizeof(*send_data.get()) << " [await] msg: " << msg.toString() << std::endl;
            sent = send(client_socket_, send_data.get(), msg.size() + sizeof(core::MsgHeader) + 1, 0);
#else
           sent = send(client_socket_, reinterpret_cast<const char*>(&msg.header()),
               msg.size() + sizeof(core::MsgHeader), 0);
#endif

            if (sent == -1)
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
                auto it = pending_responses_.find(msg.seqID());
                if (it != pending_responses_.end())
                {
                    it->second.valid_flag->store(false);
                    pending_responses_.erase(it);
                }
                snprintf(error_info_, 1024, "RPC timeout for seqID %ld", msg.seqID());
                return {-1, resp_list};
            }

            // 解析响应
            try
            {
                std::string res_str = future.get();
                std::vector<RES> resp_array = nlohmann::json::parse(res_str).get<std::vector<RES>>();
                for (const auto &r : resp_array)
                {
                    if (r.return_code < 0)
                        has_err.store(true);
                    resp_list.emplace_back(r);
                }
                if (has_err.load() && msg.toString() == "{Clear}")
                {
                    has_err.store(false);
                }
            }
            catch (const std::exception &e)
            {
                snprintf(error_info_, 1024, "JSON parse error: %s", e.what());
                return {-1, resp_list};
            }

            return {ret, resp_list};
        }

        template <typename RES>
        bool CallAsync(const core::Msg &msg,
                       const uint64_t &time_out,
                       std::function<void(int, const std::vector<RES> &)> cb)
        {
            if (has_err.load() && msg.toString() != "{Clear}")
            {
                snprintf(error_info_, 1024, "Error occured, please invoke [ClearErr] to fix it.");
                return false;
            }

            if (!cb)
            {
                snprintf(error_info_, 1024, "Callback is null.");
                return false;
            }

            if (client_socket_ == -1) {
                std::cerr << "Not connected to server." << std::endl;
                snprintf(error_info_, 1024, "connection invalid.");
                return false;
            }

            // 创建 promise 并注册
            auto promise = std::make_shared<std::promise<std::string>>();
            auto valid_flag = std::make_shared<std::atomic<bool>>(true);
            {
                std::lock_guard<std::mutex> lock(resp_map_mutex_);
                pending_responses_[msg.seqID()] = PendingItem{promise, valid_flag};
            }

        // 发送消息
        ssize_t sent = 0;
        #ifdef CODEIT_1_3_0
            auto send_data = std::make_unique<char[]>(msg.size() + sizeof(core::MsgHeader) + 1);
            //char * pdata = new char[data.size() + sizeof(MsgHeader) + 1];
            memset(send_data.get(), 0x0, msg.size() + sizeof(core::MsgHeader) + 1);
            send_data.get()[0] = 0x22; // 合法消息标识头
            memmove(send_data.get() + 1, reinterpret_cast<const char *>(&msg.header()), msg.size() + sizeof(core::MsgHeader));
            std::cout << ">>>" << sizeof(*send_data.get()) << " [async] msg: " << msg.toString() << std::endl;
            sent = send(client_socket_, send_data.get(), msg.size() + sizeof(core::MsgHeader) + 1, 0);
#else
            sent = send(client_socket_, reinterpret_cast<const char *>(&msg.header()),
                        msg.size() + sizeof(core::MsgHeader), 0);
#endif

            if (sent == -1)
            {
                snprintf(error_info_, 1024, "Failed to send message.");
                std::lock_guard<std::mutex> lock(resp_map_mutex_);
                pending_responses_.erase(msg.seqID());
                return false;
            }

            thread_pool_.Enqueue(
                [this, promise, time_out, cb, seq_id = msg.seqID()]()
                {
                    std::vector<RES> resp_list{};
                    int ret = 0;

                    auto future = promise->get_future();
                    if (future.wait_for(std::chrono::milliseconds(time_out)) != std::future_status::ready)
                    {
                        ret = -1;
                        std::cout << "Async request timeout: seqID=" << seq_id << '\n';
                        {
                            std::lock_guard<std::mutex> lock(resp_map_mutex_);
                            auto it = pending_responses_.find(seq_id);
                            if (it != pending_responses_.end())
                            {
                                it->second.valid_flag->store(false);
                                pending_responses_.erase(it);
                            }
                        }
                        cb(ret, resp_list);
                        return;
                    }

                    try
                    {
                        std::string res_str = future.get();
                        std::vector<RES> resp_array = nlohmann::json::parse(res_str).get<std::vector<RES>>();
                        for (const auto &r : resp_array)
                        {
                            if (r.return_code < 0)
                            {
                                has_err.store(true);
                                ret = -1;
                            }

                            resp_list.emplace_back(r);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cout << "Async response parse failed: " << e.what() << '\n';
                        ret = -1;
                    }
                    cb(ret, resp_list);
                });

            return true;
        }

        // 同步清理处理
        bool ClearErr()
        {
            int res = -1;
            do
            {
                // 1.send '{Clear}' cmd to clear error
                core::Msg msg("{Clear}");
                msg.setMsgID(UNIQUE_CLEAR_ERR_ID); // 全局唯一固定清楚ID
                msg.setMsgSeqID(-1);
                auto ret = CallAwait<CommResp>(msg);
                res = ret.first;

                // 2. reset flag
                has_err.store(false);

            } while (0);
            return res >= 0;
        }

        // 是否存在错误
        bool HasError() 
		{
            return has_err.load();
        }

        // YC100服务端状态
        bool IsYC100SrvOk() 
		{
            return is_yc100_available_.load();
        }

        // 获取错误信息
        char* GetErrorInfo() 
		{
            return error_info_;
        }

    private:

        // 解析客户端消息体是否合法
        bool IsValidCmdLists(const std::string& cmds) 
		{

            // 正则表达式来验证字符串
            std::regex pattern(R"re(^\{\s*(?:[^{}||]*\s*\|\|?\s*){0,6}[^{}||]*\s*\}$)re");

            // return std::regex_match(cmds, pattern);
            return true;
        }

        // 合法：返回 true，各个模型的命令数组；
        // 否则，false
        bool ExtractCmds(const std::string& cmd, std::vector<std::string>& model_cmds) 
		{

            // 合法格式，形如: "{UShow||Idle||xxx||...}"
            // 正则判断
            if (!IsValidCmdLists(cmd)) 
			{
                std::cerr << "Invalid input string: " << cmd << std::endl;
                return false;
            }

            // 使用正则表达式提取各个部分
            std::regex itemPattern(R"re(\s*([^{}||]*)\s*(?:\|\||$))re");
            //std::regex itemPattern(R"((?:[^{}\|]|\\.|\{[^{}]*\})+)");
            std::sregex_token_iterator iter(cmd.begin(), cmd.end(), itemPattern, 1);
            std::sregex_token_iterator end;

            while (iter != end) 
			{
                model_cmds.push_back(*iter++);
            }

            return true;
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
            inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr);
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

        // 设置为非阻塞模式
        // fcntl(client_socket_, F_SETFL, O_NONBLOCK);

        if (connect(client_socket_, (struct sockaddr *)&server_addr,
                    sizeof(server_addr)) == -1) 
					{
            std::cerr << "Failed to connect to server. port:" << server_port_ << std::endl;
        #ifndef WIN32
            close(client_socket_);
        #else
            closesocket(client_socket_);
        #endif
            client_socket_ = -1;
            return false;
        }
        CPP_RPC_LOG("succeed to connect to server, pport:%d\n", server_port_);

            // 设置为非阻塞模式
            // fcntl(client_socket, F_SETFL, O_NONBLOCK);

            // epoll_fd = epoll_create1(0);
            // if (epoll_fd == -1) {
            //     std::cerr << "Failed to create epoll instance." << std::endl;
            //     close(client_socket);
            //     client_socket = -1;
            //     return false;
            // }

            // struct epoll_event ev;
            // ev.events = EPOLLIN | EPOLLOUT;
            // ev.data.fd = client_socket;
            // if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
            //     std::cerr << "Failed to add client socket to epoll." << std::endl;
            //     close(client_socket);
            //     client_socket = -1;
            //     return false;
            // }

            // 启动接收线程
            recv_thread_ = std::thread(
			[this]() 
			{
                CPP_RPC_LOG("recv thread is running ... n");

                while (!destroyed_.load()) 
				{

                    core::Msg recv_msg;
                    recv_msg.resize(1024);

#ifdef CODEIT_1_3_0
                    // 接收头部标志
                    char head_flag = 0xff;
                    int ret = safe_recv(client_socket_, reinterpret_cast<char*>(&head_flag), 1);
                    if (ret <= 0) {
                        if (errno == EINTR) continue;  // 信号中断，重试
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
                            continue;  // 非阻塞无数据，稍后再试
                        }
                        if (!destroyed_.load()) {
                            CPP_RPC_LOG("[header-flag] safe_recv failed(ret:%d, errno:%d), closing socket.\n", ret, errno);
                        }
                        break;  
                    }
#endif
                    // 接收消息头
                    int header_recv = safe_recv(client_socket_, reinterpret_cast<char*>(&recv_msg.header()), sizeof(core::MsgHeader));
                    if (header_recv <= 0) {
                        if (!destroyed_.load()) {
                            CPP_RPC_LOG("[header] safe_recv failed(ret:%d, errno:%d), closing socket.\n", header_recv, errno);
                        }
                        break;
                    }
            
                    // 接收消息体
                    recv_msg.resize(recv_msg.size());
                    if (recv_msg.size() > 0) {
                        int body_recv = safe_recv(client_socket_, recv_msg.data(), recv_msg.size());
                        if (body_recv <= 0) {
                            if (!destroyed_.load()) {
                                CPP_RPC_LOG("[body] safe_recv failed(ret:%d, errno:%d)\n", body_recv, errno);
                            }
                            continue; // body失败不退出，可能只是临时问题
                        }
                    }
            
                    // 推入消息队列（确保锁顺序一致，避免死锁风险）
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
                    
                        if (item.promise && item.valid_flag && item.valid_flag->load()) {
                            try {
                                item.promise->set_value(recv_msg.toString());
                            } catch (const std::future_error& e) {
                                std::cerr << "set_value error (probably already satisfied): " << e.what() << '\n';
                            }
                        } else {
                            std::cout << "No valid pending response for seqID: " << seq_id << '\n';
                        }
                    }
                }
            
                CPP_RPC_LOG("recv thread exit.\n"); });

            return true;
        }

    private:
        std::string server_ip_;
        int server_port_;
        int client_socket_;
        static constexpr size_t BUFFER_SIZE = 1024;
        ThreadPool thread_pool_{4};
        std::condition_variable resp_cv_;
        std::mutex mtx_;
        std::thread recv_thread_;
        std::atomic<bool> destroyed_{false};
        mutable std::mutex resp_map_mutex_;
        std::unordered_map<int64_t, PendingItem> pending_responses_;
        std::atomic<bool> has_err{false};
        std::atomic<bool> is_yc100_available_{true};
        char error_info_[1024]{0x0};
    };
}
#endif