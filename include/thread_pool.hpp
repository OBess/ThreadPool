#pragma once

#include <thread>
#include <array>
#include <future>
#include <memory>
#include <functional>

#include <threads/threadsafe_queue.hpp>
#include <common/rules.hpp>
#include <iostream>

namespace ds::th
{
    class thread_pool final
    {
    public:
        using FuncType = std::function<void()>;

        NONCOPYABLE(thread_pool);
        NONMOVEABLE(thread_pool);

        inline thread_pool(const size_t pool_size)
        {
            try
            {
                _pool.reserve(pool_size);
                for (size_t i = 0; i < pool_size; ++i)
                {
                    _pool.emplace_back(&thread_pool::process, this);
                }
            }
            catch (...)
            {
                _run = false;
                throw std::runtime_error("[thread_pool] Cannot create threads");
            }
        }

        inline ~thread_pool()
        {
            stop();

            for (auto &t : _pool)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }
        }

        /**
         * @brief Push function to queue
         *
         * @param f
         */
        inline void push(FuncType &&f)
        {
            _counter.fetch_add(1);
            _queue.push(std::move(f));
        }

        /**
         * @brief Create a task and push it to queue
         *
         * @tparam Func
         * @tparam Args
         * @param func
         * @param args
         * @return std::future<std::invoke_result_t<Func, Args...>>
         */
        template <class Func, class... Args>
        inline auto submit(Func &&func, Args &&...args)
        {
            using return_type = std::invoke_result_t<Func, Args...>;

            auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::move(func), std::forward<Args>(args)...));

            push([task]
                 { (*task)(); });

            return task->get_future();
        }

        /**
         * @brief Wait until the queue is empty
         *
         */
        inline void wait()
        {
            while (_queue.empty() == false)
            {
                execute();
            }

            while (_counter > 0)
            {
            }
        }

        /**
         * @brief Add more threads to pool
         *
         * @param amount
         */
        inline void add_threads(uint16_t amount = 1)
        {
            while (amount--)
                _pool.emplace_back(&thread_pool::process, this);
        }

        inline void stop() noexcept
        {
            _run = false;
        }

        /**
         * @brief Implement the Singleton template. The default pool size is one
         * to call this function without an argument
         *
         * @param pool_size
         * @return thread_pool&
         */
        static inline thread_pool &get(const size_t pool_size = 1)
        {
            static thread_pool instance(pool_size);
            return instance;
        }

    private:
        void process()
        {
            while (_run)
            {
                execute();
            }
        }

        inline void execute()
        {
            FuncType task;
            if (_queue.try_pop(task))
            {
                task();
                _counter.fetch_sub(1);
            }
            else
            {
                std::this_thread::yield();
            }
        }

        std::vector<std::thread> _pool;
        threadsafe_queue<FuncType> _queue;
        std::atomic_size_t _counter{0};
        std::atomic_bool _run{true};
    };

} // namespace th
