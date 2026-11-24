// 线程池类
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string.h>

#ifndef __TASK_POOL__
#define __TASK_POOL__

class ThreadPool {
public:
    ThreadPool(size_t num_threads)
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

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop = true;
        }
        condition_.notify_all();

        for (std::thread& worker : workers_) {
            worker.join();
        }
    }

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
