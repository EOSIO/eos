/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>

#include <fc/thread_pool.hpp>
#include <fc/exception/exception.hpp>

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(thread_pool_tests)

BOOST_AUTO_TEST_CASE(basic)
{ try {

   fc::thread_pool pool;
   pool.init(5);

   auto f1 = pool.async(//<fc::flat_set<eosio::chain::public_key_type>>(
         [](){
      fc::flat_set<eosio::chain::public_key_type> tmp;
      tmp.emplace();
      return tmp;
   });

//      auto f2 = pool.async_t([](){
//         fc::flat_set<eosio::chain::public_key_type> tmp;
//         tmp.emplace();
//         return tmp;
//      });

   auto set1 = f1.get();
//   auto set2 = f2.get();

   BOOST_CHECK_EQUAL( 1, set1.size() );

   pool.stop();


} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
