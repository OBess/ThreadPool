#include <iostream>

#include "include/thread_pool.hpp"

// -------------------------------------------------------------------------------------------------
void taskWithoutArgs()
{
   using namespace std::chrono_literals;
   std::this_thread::sleep_for(500ms);
   std::cout << "Task Without Args. Thread ID: " << std::this_thread::get_id() << std::endl;
}

// -------------------------------------------------------------------------------------------------
int taskWithArgs(int a, int b)
{
   using namespace std::chrono_literals;
   std::this_thread::sleep_for(500ms);

   std::cout << "Task With Args. Thread ID: " << std::this_thread::get_id() << std::endl;

   return a + b;
}

// -------------------------------------------------------------------------------------------------
class HasTask final
{
public:
   HasTask() = default;
   ~HasTask() = default;

   void task()
   {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(500ms);
      std::cout << _str << " Thread ID: " << std::this_thread::get_id() << std::endl;
   }

private:
   const std::string_view _str{"Task from class."};
};

// -------------------------------------------------------------------------------------------------
struct Functor final
{
   Functor() = default;
   ~Functor() = default;

   void operator()()
   {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(500ms);
      std::cout << "Functor. Thread ID: " << std::this_thread::get_id() << std::endl;
   }
};

// -------------------------------------------------------------------------------------------------
int main()
{
   ds::th::thread_pool<5> pool;

   auto result2P3 = pool.submit(std::bind(taskWithArgs, 2, 3));
   auto th1 = pool.submit(taskWithoutArgs);
   auto th2 = pool.submit(taskWithoutArgs);
   auto th3 = pool.submit(taskWithoutArgs);
   auto th4 = pool.submit(taskWithoutArgs);
   auto th5 = pool.submit(taskWithoutArgs);

   HasTask ht;
   auto th6 = pool.submit(std::bind(&HasTask::task, &ht));

   auto th7 = pool.submit(Functor{});
   
   pool.wait();

   std::cout << "2 + 3 = " << result2P3.get() << std::endl;

   /* Do not write code */
   return EXIT_SUCCESS;
}