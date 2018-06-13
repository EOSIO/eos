/**
 *  @file api_tests.cpp
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop

#include <eosio/testing/tester.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>

#include "eosio_system_tester.hpp"

#include <test_ram_limit/test_ram_limit.abi.hpp>
#include <test_ram_limit/test_ram_limit.wast.hpp>

#define DISABLE_EOSLIB_SERIALIZE
#include <test_api/test_api_common.hpp>

/*
 * register test suite `ram_tests`
 */
BOOST_AUTO_TEST_SUITE(ram_tests)



/*************************************************************************************
 * ram_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(ram_tests, eosio_system::eosio_system_tester) { try {
   auto init_request_bytes = 80000;
   const auto increment_contract_bytes = 10000;
   const auto table_allocation_bytes = 12000;
   BOOST_REQUIRE_MESSAGE(table_allocation_bytes > increment_contract_bytes, "increment_contract_bytes must be less than table_allocation_bytes for this test setup to work");
   buyrambytes(N(eosio), N(eosio), 70000);
   produce_blocks(10);
   create_account_with_resources(N(testram11111),N(eosio), init_request_bytes);
   create_account_with_resources(N(testram22222),N(eosio), init_request_bytes + 1150);
   produce_blocks(10);
   BOOST_REQUIRE_EQUAL( success(), stake( "eosio.stake", "testram11111", core_from_string("10.0000"), core_from_string("5.0000") ) );
   produce_blocks(10);

   for (auto i = 0; i < 10; ++i) {
      try {
         set_code( N(testram11111), test_ram_limit_wast );
         break;
      } catch (const ram_usage_exceeded&) {
         init_request_bytes += increment_contract_bytes;
         buyrambytes(N(eosio), N(testram11111), increment_contract_bytes);
         buyrambytes(N(eosio), N(testram22222), increment_contract_bytes);
      }
   }
   produce_blocks(10);

   for (auto i = 0; i < 10; ++i) {
      try {
         set_abi( N(testram11111), test_ram_limit_abi );
         break;
      } catch (const ram_usage_exceeded&) {
         init_request_bytes += increment_contract_bytes;
         buyrambytes(N(eosio), N(testram11111), increment_contract_bytes);
         buyrambytes(N(eosio), N(testram22222), increment_contract_bytes);
      }
   }
   produce_blocks(10);
   set_code( N(testram22222), test_ram_limit_wast );
   set_abi( N(testram22222), test_ram_limit_abi );
   produce_blocks(10);

   auto total = get_total_stake( N(testram11111) );
   const auto init_bytes =  total["ram_bytes"].as_uint64();

   auto rlm = control->get_resource_limits_manager();
   auto initial_ram_usage = rlm.get_account_ram_usage(N(testram11111));

   // calculate how many more bytes we need to have table_allocation_bytes for database stores
   auto more_ram = table_allocation_bytes + init_bytes - init_request_bytes;
   BOOST_REQUIRE_MESSAGE(more_ram >= 0, "Underlying understanding changed, need to reduce size of init_request_bytes");
   wdump((init_bytes)(initial_ram_usage)(init_request_bytes)(more_ram) );
   buyrambytes(N(eosio), N(testram11111), more_ram);
   buyrambytes(N(eosio), N(testram22222), more_ram);

   TESTER* tester = this;
   // allocate just under the allocated bytes
   tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                        ("payer", "testram11111")
                        ("from", 1)
                        ("to", 10)
                        ("size", 1780 /*1910*/));
   produce_blocks(1);
   auto ram_usage = rlm.get_account_ram_usage(N(testram11111));

   total = get_total_stake( N(testram11111) );
   const auto ram_bytes =  total["ram_bytes"].as_uint64();
   wdump((ram_bytes)(ram_usage)(initial_ram_usage)(init_bytes)(ram_usage - initial_ram_usage)(init_bytes - ram_usage) );

   wlog("ram_tests 1    %%%%%%");
   // allocate just beyond the allocated bytes
   BOOST_REQUIRE_EXCEPTION(
      tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                           ("payer", "testram11111")
                           ("from", 1)
                           ("to", 10)
                           ("size", 1790 /*1920*/)),
                           ram_usage_exceeded,
                           fc_exception_message_starts_with("account testram11111 has insufficient ram"));
   wlog("ram_tests 2    %%%%%%");
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL(ram_usage, rlm.get_account_ram_usage(N(testram11111)));

   // update the entries with smaller allocations so that we can verify space is freed and new allocations can be made
   tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                        ("payer", "testram11111")
                        ("from", 1)
                        ("to", 10)
                        ("size", 1680/*1810*/));
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL(ram_usage - 1000, rlm.get_account_ram_usage(N(testram11111)));

   // verify the added entry is beyond the allocation bytes limit
   BOOST_REQUIRE_EXCEPTION(
      tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                           ("payer", "testram11111")
                           ("from", 1)
                           ("to", 11)
                           ("size", 1680/*1810*/)),
                           ram_usage_exceeded,
                           fc_exception_message_starts_with("account testram11111 has insufficient ram"));
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL(ram_usage - 1000, rlm.get_account_ram_usage(N(testram11111)));

   // verify the new entry's bytes minus the freed up bytes for existing entries still exceeds the allocation bytes limit
   BOOST_REQUIRE_EXCEPTION(
      tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                           ("payer", "testram11111")
                           ("from", 1)
                           ("to", 11)
                           ("size", 1760)),
                           ram_usage_exceeded,
                           fc_exception_message_starts_with("account testram11111 has insufficient ram"));
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL(ram_usage - 1000, rlm.get_account_ram_usage(N(testram11111)));

   // verify the new entry's bytes minus the freed up bytes for existing entries are under the allocation bytes limit
   tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                        ("payer", "testram11111")
                        ("from", 1)
                        ("to", 11)
                        ("size", 1600/*1720*/));
   produce_blocks(1);

   tester->push_action( N(testram11111), N(rmentry), N(testram11111), mvo()
                        ("from", 3)
                        ("to", 3));
   produce_blocks(1);
   
   // verify that the new entry will exceed the allocation bytes limit
   BOOST_REQUIRE_EXCEPTION(
      tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                           ("payer", "testram11111")
                           ("from", 12)
                           ("to", 12)
                           ("size", 1780)),
                           ram_usage_exceeded,
                           fc_exception_message_starts_with("account testram11111 has insufficient ram"));
   produce_blocks(1);

   // verify that the new entry is under the allocation bytes limit
   tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                        ("payer", "testram11111")
                        ("from", 12)
                        ("to", 12)
                        ("size", 1620/*1720*/));
   produce_blocks(1);

   // verify that anoth new entry will exceed the allocation bytes limit, to setup testing of new payer
   BOOST_REQUIRE_EXCEPTION(
      tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                           ("payer", "testram11111")
                           ("from", 13)
                           ("to", 13)
                           ("size", 1660)),
                           ram_usage_exceeded,
                           fc_exception_message_starts_with("account testram11111 has insufficient ram"));
   produce_blocks(1);

   // verify that the new entry is under the allocation bytes limit
   tester->push_action( N(testram11111), N(setentry), {N(testram11111),N(testram22222)}, mvo()
                        ("payer", "testram22222")
                        ("from", 12)
                        ("to", 12)
                        ("size", 1720));
   produce_blocks(1);

   // verify that another new entry that is too big will exceed the allocation bytes limit, to setup testing of new payer
   BOOST_REQUIRE_EXCEPTION(
      tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                           ("payer", "testram11111")
                           ("from", 13)
                           ("to", 13)
                           ("size", 1900)),
                           ram_usage_exceeded,
                           fc_exception_message_starts_with("account testram11111 has insufficient ram"));
   produce_blocks(1);

   wlog("ram_tests 18    %%%%%%");
   // verify that the new entry is under the allocation bytes limit, because entry 12 is now charged to testram22222
   tester->push_action( N(testram11111), N(setentry), N(testram11111), mvo()
                        ("payer", "testram11111")
                        ("from", 13)
                        ("to", 13)
                        ("size", 1720));
   produce_blocks(1);

   // verify that new entries for testram22222 exceed the allocation bytes limit
   BOOST_REQUIRE_EXCEPTION(
      tester->push_action( N(testram11111), N(setentry), {N(testram11111),N(testram22222)}, mvo()
                           ("payer", "testram22222")
                           ("from", 12)
                           ("to", 21)
                           ("size", 1930)),
                           ram_usage_exceeded,
                           fc_exception_message_starts_with("account testram22222 has insufficient ram"));
   produce_blocks(1);

   // verify that new entries for testram22222 are under the allocation bytes limit
   tester->push_action( N(testram11111), N(setentry), {N(testram11111),N(testram22222)}, mvo()
                        ("payer", "testram22222")
                        ("from", 12)
                        ("to", 21)
                        ("size", 1910));
   produce_blocks(1);

   // verify that new entry for testram22222 exceed the allocation bytes limit
   BOOST_REQUIRE_EXCEPTION(
      tester->push_action( N(testram11111), N(setentry), {N(testram11111),N(testram22222)}, mvo()
                           ("payer", "testram22222")
                           ("from", 22)
                           ("to", 22)
                           ("size", 1910)),
                           ram_usage_exceeded,
                           fc_exception_message_starts_with("account testram22222 has insufficient ram"));
   produce_blocks(1);

   tester->push_action( N(testram11111), N(rmentry), N(testram11111), mvo()
                        ("from", 20)
                        ("to", 20));
   produce_blocks(1);

   // verify that new entry for testram22222 are under the allocation bytes limit
   tester->push_action( N(testram11111), N(setentry), {N(testram11111),N(testram22222)}, mvo()
                        ("payer", "testram22222")
                        ("from", 22)
                        ("to", 22)
                        ("size", 1910));
   produce_blocks(1);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
