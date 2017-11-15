#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;


BOOST_AUTO_TEST_SUITE(block_tests)



BOOST_AUTO_TEST_CASE( schedule_test ) {
  tester test;

  for( uint32_t i = 0; i < 100; ++i )
     test.produce_block();



  test.close();
  test.open();


  for( uint32_t i = 0; i < 100; ++i )
     test.produce_block();
}

BOOST_AUTO_TEST_SUITE_END()
