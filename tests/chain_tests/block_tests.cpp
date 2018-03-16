#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;


BOOST_AUTO_TEST_SUITE(block_tests)



BOOST_AUTO_TEST_CASE( schedule_test ) { try {
  tester test;

  for( uint32_t i = 0; i < 200; ++i )
     test.produce_block();

  auto lib = test.control->last_irreversible_block_num();
  auto head = test.control->head_block_num();
  idump((lib)(head));

  test.close();
  test.open();

  auto rlib = test.control->last_irreversible_block_num();
  auto rhead = test.control->head_block_num();

  FC_ASSERT( rhead == lib );

  for( uint32_t i = 0; i < 1000; ++i )
     test.produce_block();
  ilog("exiting");
} FC_LOG_AND_RETHROW() }/// schedule_test

BOOST_AUTO_TEST_CASE( push_block ) { try {
   tester test1;
   base_tester test2;

   for (uint32 i = 0; i < 1000; ++i) {
      test2.control->push_block(test1.produce_block());
   }

   test1.create_account(N(alice));
   test2.control->push_block(test1.produce_block());

   test1.push_dummy(N(alice), "Foo!");
   test2.control->push_block(test1.produce_block());
} FC_LOG_AND_RETHROW() }/// schedule_test



BOOST_AUTO_TEST_SUITE_END()
