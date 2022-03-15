#pragma once

#include <thread>
#include <array>
#include <future>
#include <memory>

#include <threads/threadsafe_queue.hpp>
#include <removeable.hpp>

namespace ds::th
{

    namespace detail
    {

        /**
         * @brief Wrapper for function
         *
         */
        class wrapper final
        {
        public:
            NONCOPYABLE(wrapper);

            wrapper() = default;

            ~wrapper() = default;

            template <class F>
            wrapper(F &&f)
                : _function(std::make_unique<impl_type<F>>(std::move(f)))
            {
            }

            wrapper(wrapper &&other) noexcept
                : _function(std::move(other._function))
            {
            }

            wrapper &operator=(wrapper &&other) noexcept
            {
                _function = std::move(other._function);
                return *this;
            }

            void operator()()
            {
                _function->call();
            }

        private:
            struct impl_base
            {
                impl_base() = default;
                virtual ~impl_base() = default;

                virtual void call() = 0;
            };

            template <class Func>
            struct impl_type final : impl_base
            {
                Func f;

                impl_type(Func &&f)
                    : f(std::move(f))
                {
                }

                virtual ~impl_type() = default;

                void call() override { f(); }
            };

            std::unique_ptr<impl_base> _function;
        };

    } // namespace detail

    template <size_t pool_size>
    class thread_pool final
    {
    public:
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
            _run = false;
            join();
        }

        /**
         * @brief Create a task and push it to queue
         *
         * @tparam Func
         * @param f
         * @return std::future<std::invoke_result_t<Func>>
         */
        template <class Func>
        std::future<std::invoke_result_t<Func>> submit(Func &&func)
        {
            using return_type = std::invoke_result_t<Func>;
            std::packaged_task<return_type()> task(std::move(func));

            auto future = task.get_future();
            _queue.push(std::move(task));

            return future;
        }

        /**
         * @brief Join all threads to end all tasks in queue
         * 
         */
        void join() noexcept
        {
            for (auto &t : _pool)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }
        }

        /**
         * @brief Wait until the queue is empty
         * 
         */
        void wait()
        {
            while (_queue.empty() == false)
            {
                detail::wrapper task;
                if (_queue.try_pop(task))
                {
                    task();
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        }

    private:
        void process()
        {
            while (_run)
            {
                detail::wrapper task;
                if (_queue.try_pop(task))
                {
                    task();
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        }

        std::array<std::thread, pool_size> _pool;
        threadsafe_queue<detail::wrapper> _queue;
        std::atomic_bool _run{true};
    };

} // namespace th
