/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>

#include <fc/thread_pool.hpp>
#include <fc/exception/exception.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio.hpp>


BOOST_AUTO_TEST_SUITE(thread_pool_tests)

template<typename F>
auto async( boost::asio::thread_pool& pool, F&& f ) {
   auto task = std::make_shared<std::packaged_task<decltype( f() )()>>( std::forward<F>(f) );
   boost::asio::post( pool, [task]() { (*task)(); } );
   return task->get_future();
}


BOOST_AUTO_TEST_CASE(basic)
{ try {

      fc::thread_pool pool;
      pool.init( 5 );

      auto f1 = pool.async( []() {
         fc::flat_set<eosio::chain::public_key_type> tmp;
         tmp.emplace();
         return tmp;
      } );

      auto set1 = f1.get();

      BOOST_CHECK_EQUAL( 1, set1.size());

      pool.stop();

      boost::asio::thread_pool p(5);

      auto f2 = async( p, []() {
         fc::flat_set<eosio::chain::public_key_type> tmp;
         tmp.emplace();
         return tmp;
      });

      auto set2 = f2.get();

      BOOST_CHECK_EQUAL( 1, set2.size());

      p.join();
      p.stop();

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
