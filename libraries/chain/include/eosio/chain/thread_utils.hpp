/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <future>
#include <memory>

namespace eosio { namespace chain {

   // async on thread_pool and return future
   template<typename F>
   auto async_thread_pool( boost::asio::thread_pool& thread_pool, F&& f ) {
      auto task = std::make_shared<std::packaged_task<decltype( f() )()>>( std::forward<F>( f ) );
      boost::asio::post( thread_pool, [task]() { (*task)(); } );
      return task->get_future();
   }

} } // eosio::chain


