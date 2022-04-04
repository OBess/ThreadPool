#pragma once

#include <thread>
#include <array>
#include <future>
#include <memory>
#include <functional>

#include <threads/threadsafe_queue.hpp>
#include <removeable.hpp>

namespace ds::th
{
    template <size_t pool_size>
    class thread_pool final
    {
    public:
        using FuncType = std::function<void()>;

        NONCOPYABLE(thread_pool);
        NONMOVEABLE(thread_pool);

        thread_pool()
        {
            try
            {
                for (auto &p : _pool)
                {
                    p = std::move(std::thread(&thread_pool::process, this));
                }
            }
            catch (...)
            {
                _run = false;
                throw std::runtime_error("[thread_pool] Cannot create threads");
            }
        }

        ~thread_pool()
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
        void push(FuncType &&f)
        {
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
        std::future<std::invoke_result_t<Func, Args...>> submit(Func &&func, Args &&...args)
        {
            using return_type = std::invoke_result_t<Func, Args...>;
            using p_task = std::packaged_task<return_type()>;
            p_task task(std::bind(func, std::forward<Args>(args)...));

            auto future = task.get_future();
            _queue.push([f = std::make_shared<p_task>(std::move(task))]
                        { (*f)(); });

            return future;
        }

        /**
         * @brief Wait until the queue is empty
         *
         */
        void wait()
        {
            while (_queue.empty() == false)
            {
                execute();
            }

            while (_counter)
                ;
        }

        inline void stop() noexcept
        {
            _run = false;
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
                ++_counter;
                task();
                --_counter;
            }
            else
            {
                std::this_thread::yield();
            }
        }

        std::array<std::thread, pool_size> _pool;
        threadsafe_queue<FuncType> _queue;
        std::atomic_bool _run{true};
        std::atomic_size_t _counter{0};
    };

} // namespace th
