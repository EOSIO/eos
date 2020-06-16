#pragma once

#include <fc/optional.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <future>
#include <memory>

namespace eosio { namespace chain {

   /**
    * Wrapper class for boost asio thread pool and io_context run.
    * Also names threads so that tools like htop can see thread name.
    */
   class named_thread_pool {
   public:
      // name_prefix is name appended with -## of thread.
      // short name_prefix (6 chars or under) is recommended as console_appender uses 9 chars for thread name
      named_thread_pool( std::string name_prefix, size_t num_threads, bool one_io_context = false );

      // calls stop()
      ~named_thread_pool();

      // round robin executor from pool
      boost::asio::io_context& get_executor();

      // destroy work guard, stop io_context, join thread_pool, and stop thread_pool
      void stop();

   private:
      using ioc_work_t = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

      std::vector<std::thread>                               _thread_pool;
      std::vector<std::unique_ptr<boost::asio::io_context>>  _iocs;
      std::optional<std::vector<ioc_work_t>>                 _ioc_works;
      std::atomic<size_t>                                    _next_ioc = 0;
   };


   // async on thread_pool and return future
   template<typename F>
   auto async_thread_pool( boost::asio::io_context& thread_pool, F&& f ) {
      auto task = std::make_shared<std::packaged_task<decltype( f() )()>>( std::forward<F>( f ) );
      boost::asio::post( thread_pool, [task]() { (*task)(); } );
      return task->get_future();
   }

} } // eosio::chain


