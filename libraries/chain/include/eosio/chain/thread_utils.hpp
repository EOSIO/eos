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
      named_thread_pool( std::string name_prefix, size_t num_threads );

      // calls stop()
      ~named_thread_pool();

      boost::asio::io_context& get_executor() { return _ioc; }

      // destroy work guard, stop io_context, join thread_pool, and stop thread_pool
      void stop();

   private:
      using ioc_work_t = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

      boost::asio::thread_pool       _thread_pool;
      boost::asio::io_context        _ioc;
      fc::optional<ioc_work_t>       _ioc_work;
   };


   // async on thread_pool and return future
   template<typename F>
   auto async_thread_pool( boost::asio::io_context& thread_pool, F&& f ) {
      auto task = std::make_shared<std::packaged_task<decltype( f() )()>>( std::forward<F>( f ) );
      boost::asio::post( thread_pool, [task]() { (*task)(); } );
      return task->get_future();
   }

} } // eosio::chain


