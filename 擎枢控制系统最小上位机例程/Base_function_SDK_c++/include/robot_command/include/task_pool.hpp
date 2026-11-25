// 线程池类
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string.h>
#include "util.hpp"

#ifndef __TASK_POOL__
#define __TASK_POOL__

class CPP_RPC_EXPORT ThreadPool {
public:
    ThreadPool(size_t num_threads);

    ~ThreadPool() ;

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
#endif // !__TASK_POOL__