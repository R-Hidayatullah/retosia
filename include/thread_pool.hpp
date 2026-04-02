#pragma once
#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <tuple>

namespace tp
{

    class ThreadPool
    {
    public:
        explicit ThreadPool(std::size_t thread_count = 0)
        {
            std::size_t n = (thread_count == 0)
                                ? std::max<std::size_t>(1u, std::thread::hardware_concurrency())
                                : thread_count;

            workers_.reserve(n);
            for (std::size_t i = 0; i < n; ++i)
            {
                workers_.emplace_back([this]
                                      { worker_loop(); });
            }
        }

        ~ThreadPool()
        {
            shutdown();
        }

        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;
        ThreadPool(ThreadPool &&) = delete;
        ThreadPool &operator=(ThreadPool &&) = delete;

        // ----------------------------------------------------------
        // submit
        // ----------------------------------------------------------
        template <typename F, typename... Args>
        auto submit(F &&f, Args &&...args)
            -> std::future<typename std::invoke_result<F, Args...>::type>
        {
            using Ret = typename std::invoke_result<F, Args...>::type;

            auto task_ptr = std::make_shared<std::packaged_task<Ret()>>(
                [func = std::forward<F>(f),
                 tup = std::make_tuple(std::forward<Args>(args)...)]() mutable -> Ret
                {
                    return std::apply(std::move(func), std::move(tup));
                });

            std::future<Ret> fut = task_ptr->get_future();

            {
                std::scoped_lock<std::mutex> lock(mutex_);

                if (stopped_.load(std::memory_order_relaxed))
                    throw std::runtime_error("ThreadPool::submit – pool is stopped");

                ++pending_;
                tasks_.emplace([task_ptr]()
                               { (*task_ptr)(); });
            }

            cv_.notify_one();
            return fut;
        }

        // ----------------------------------------------------------
        // wait_all
        // ----------------------------------------------------------
        void wait_all()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            idle_cv_.wait(lock, [this]
                          { return pending_ == 0 && tasks_.empty(); });
        }

        // ----------------------------------------------------------
        // shutdown
        // ----------------------------------------------------------
        void shutdown()
        {
            {
                std::scoped_lock<std::mutex> lock(mutex_);
                if (stopped_.exchange(true))
                    return;
            }

            cv_.notify_all();

            for (auto &t : workers_)
            {
                if (t.joinable())
                    t.join();
            }
        }

        // ----------------------------------------------------------
        // info
        // ----------------------------------------------------------
        std::size_t thread_count() const noexcept
        {
            return workers_.size();
        }

        std::size_t pending_count() const
        {
            std::scoped_lock<std::mutex> lock(mutex_);
            return pending_;
        }

    private:
        void worker_loop()
        {
            while (true)
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mutex_);

                    cv_.wait(lock, [this]
                             { return stopped_.load(std::memory_order_relaxed) || !tasks_.empty(); });

                    if (tasks_.empty())
                        return;

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                // Execute outside lock
                try
                {
                    task();
                }
                catch (...)
                {
                    // Optional: log error
                }

                {
                    std::scoped_lock<std::mutex> lock(mutex_);
                    --pending_;
                }

                idle_cv_.notify_one();
            }
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::condition_variable idle_cv_;

        std::queue<std::function<void()>> tasks_;

        std::atomic<bool> stopped_{false};
        std::size_t pending_{0};

        std::vector<std::thread> workers_;
    };

} // namespace tp

#endif // THREAD_POOL_HPP