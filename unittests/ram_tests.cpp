/**
 *  @file api_tests.cpp
 *  @copyright defined in eos/LICENSE
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>

#include <contracts.hpp>

#include "eosio_system_tester.hpp"

/*
 * register test suite `ram_tests`
 */
BOOST_AUTO_TEST_SUITE(ram_tests)

#define PRINT_USAGE(account) { \
   auto rlm = control->get_resource_limits_manager(); \
   auto ram_usage = rlm.get_account_ram_usage(N(account)); \
   int64_t ram_bytes; \
   int64_t net_weight; \
   int64_t cpu_weight; \
   rlm.get_account_limits( N(account), ram_bytes, net_weight, cpu_weight); \
   const auto free_bytes = ram_bytes - ram_usage; \
   wdump((#account)(ram_usage)(free_bytes)(ram_bytes)(net_weight)(cpu_weight)); \
}

/*************************************************************************************
 * ram_tests test case
 *************************************************************************************/
   BOOST_FIXTURE_TEST_CASE(ram_tests, eosio_system::eosio_system_tester) {
      try {

         const uint64_t min_account_stake = get_global_state()["min_account_stake"].as<int64_t>();
         BOOST_TEST(min_account_stake == 1000000);
         PRINT_USAGE(eosio)
         PRINT_USAGE(eosio.stake)

         cross_15_percent_threshold();

         TESTER *tester = this;
         auto rlm = control->get_resource_limits_manager();

         //testing min stake 99'9999 REM
         BOOST_REQUIRE_EXCEPTION(
               create_account_with_resources(N(testram33333), config::system_account_name, false,
                                             core_from_string(std::to_string(min_account_stake / 10000 - 1) + ".9999")),
               eosio_assert_message_exception,
               fc_exception_message_starts_with("assertion failure with message: insufficient minimal account stake"));
         produce_blocks(1);

         BOOST_REQUIRE_EXCEPTION(
               create_account_with_resources(N(testram33333), config::system_account_name, false,
                                             core_from_string(std::to_string(min_account_stake / 10000 + 1) + ".0000"),
                                             false), //create without transfer
               eosio_assert_message_exception,
               fc_exception_message_starts_with("assertion failure with message: insufficient minimal account stake"));
         produce_blocks(10);

         create_account_with_resources(N(testram33333), config::system_account_name, false, core_from_string(
               std::to_string(min_account_stake / 10000 + 1) + ".0000")); //stake 101
         auto t3total = get_total_stake(N(testram33333));
         BOOST_REQUIRE_EQUAL(t3total["own_stake_amount"].as_uint64(), 1010000);

         produce_blocks(10);
         BOOST_REQUIRE_EXCEPTION(
               tester->push_action(config::system_account_name, N(undelegatebw), N(testram33333), mvo()
                     ("from", "testram33333")
                     ("receiver", "testram33333")
                     ("unstake_quantity", core_from_string("2.0000"))),
               eosio_assert_message_exception,
               fc_exception_message_starts_with("assertion failure with message: insufficient minimal account stake"));
         produce_blocks(10);
         PRINT_USAGE(testram33333)

         int64_t ram_bytes0;
         int64_t net_weight;
         int64_t cpu_weight;
         {//unstake own REM tokens
            unstake(N(testram33333), core_from_string("1.0000"));
            produce_blocks(10);
            PRINT_USAGE(testram33333)
            const auto t3total = get_total_stake(N(testram33333));
            rlm.get_account_limits(N(testram33333), ram_bytes0, net_weight, cpu_weight);
            BOOST_REQUIRE_EQUAL(ram_bytes0, 6871); // 100 REM
            BOOST_REQUIRE_EQUAL(net_weight, 1000000); // 100 REM
            BOOST_REQUIRE_EQUAL(cpu_weight, 1000000); // 100 REM
            BOOST_REQUIRE_EQUAL(t3total["own_stake_amount"].as_uint64(), 1000000);
         }

         { //system account stake without transfer tokens
            stake(config::system_account_name, N(testram33333), core_from_string("1.0000"), false);
            produce_blocks(10);
            PRINT_USAGE(testram33333)
            const auto t3total = get_total_stake(N(testram33333));
            int64_t ram_bytes1;
            int64_t net_weight;
            int64_t cpu_weight;
            rlm.get_account_limits(N(testram33333), ram_bytes1, net_weight, cpu_weight);
            BOOST_REQUIRE_EQUAL(ram_bytes0, ram_bytes1); //memory not changed
            BOOST_REQUIRE_EQUAL(net_weight, 1010000); // 101 REM increase 1 REM
            BOOST_REQUIRE_EQUAL(cpu_weight, 1010000); // 101 REM increase 1 REM
            BOOST_REQUIRE_EQUAL(t3total["own_stake_amount"].as_uint64(), 1000000);
         }

         int64_t ram_bytes2;
         { //system account stake with transfer tokens
            stake(config::system_account_name, N(testram33333), core_from_string("2.0000"), true);
            produce_blocks(10);
            PRINT_USAGE(testram33333)
            const auto t3total = get_total_stake(N(testram33333));
            int64_t net_weight;
            int64_t cpu_weight;
            rlm.get_account_limits(N(testram33333), ram_bytes2, net_weight, cpu_weight);
            BOOST_REQUIRE_GT(ram_bytes2, ram_bytes0); //memory changed
            BOOST_REQUIRE_EQUAL(net_weight, 1030000); // 103 REM increase 1 REM
            BOOST_REQUIRE_EQUAL(cpu_weight, 1030000); // 103 REM increase 1 REM
            BOOST_REQUIRE_EQUAL(t3total["own_stake_amount"].as_uint64(), 1020000);
         }

         int64_t ram_bytes3;
         { //testram33333 stake own tokens
            stake(N(testram33333), core_from_string("1.0000")); //stake own tokens
            produce_blocks(10);
            PRINT_USAGE(testram33333)
            const auto t3total = get_total_stake(N(testram33333));
            rlm.get_account_limits(N(testram33333), ram_bytes3, net_weight, cpu_weight);
            BOOST_REQUIRE_GT(ram_bytes3, ram_bytes2); //memory changed
            BOOST_REQUIRE_EQUAL(net_weight, 1040000);
            BOOST_REQUIRE_EQUAL(cpu_weight, 1040000);
            BOOST_REQUIRE_EQUAL(t3total["own_stake_amount"].as_uint64(), 1030000);
         }


         const auto increment_contract_ = core_from_string("2000.0000");
         produce_blocks(10);
         create_account_with_resources(N(testram11111), config::system_account_name, false,
                                       core_from_string(std::to_string(min_account_stake / 10000) + ".0000"));
         create_account_with_resources(N(testram22222), config::system_account_name, false,
                                       core_from_string(std::to_string(min_account_stake / 10000) + ".0000"));
         produce_blocks(10);

         PRINT_USAGE(testram11111)
         PRINT_USAGE(testram22222)
         BOOST_REQUIRE_EQUAL(success(), stake("eosio.stake", "testram11111", core_from_string("15.0000")));
         produce_blocks(10);
         PRINT_USAGE(testram11111)

         for (auto i = 0; i < 10; ++i) {
            try {
               set_code(N(testram11111), contracts::test_ram_limit_wasm());
               break;
            } catch (const ram_usage_exceeded &) {
               BOOST_REQUIRE_EQUAL(success(),
                                   stake(config::system_account_name, N(testram11111), increment_contract_, true));
               BOOST_REQUIRE_EQUAL(success(),
                                   stake(config::system_account_name, N(testram22222), increment_contract_, true));
               PRINT_USAGE(testram11111)
               PRINT_USAGE(testram22222)
            }
         }
         produce_blocks(10);
         PRINT_USAGE(testram11111)
         PRINT_USAGE(testram22222)

         for (auto i = 0; i < 10; ++i) {
            try {
               set_abi(N(testram11111), contracts::test_ram_limit_abi().data());
               break;
            } catch (const ram_usage_exceeded &) {
               BOOST_REQUIRE_EQUAL(success(), stake(config::system_account_name, N(testram11111), increment_contract_));
               BOOST_REQUIRE_EQUAL(success(), stake(config::system_account_name, N(testram22222), increment_contract_));
               PRINT_USAGE(testram11111)
               PRINT_USAGE(testram22222)
            }
         }
         produce_blocks(10);
         PRINT_USAGE(testram11111)
         PRINT_USAGE(testram22222)
         set_code(N(testram22222), contracts::test_ram_limit_wasm());
         set_abi(N(testram22222), contracts::test_ram_limit_abi().data());
         produce_blocks(10);
         PRINT_USAGE(testram22222)

         return;
//         auto total = get_total_stake(N(testram11111));
//
//         const auto init_bytes = total["ram_bytes"].as_uint64();
//
//         auto initial_ram_usage = rlm.get_account_ram_usage(N(testram11111));
//
//         // calculate how many more bytes we need to have table_allocation_bytes for database stores
//         const int64_t per_elem_ram = 1780;
//         const uint64_t count = 10;
//         const auto free_ram = init_bytes - initial_ram_usage;
//         const auto more_ram = per_elem_ram * count - free_ram;
//         BOOST_REQUIRE_MESSAGE(more_ram >= 0,
//                               "Underlying understanding changed, need to reduce size of init_request_bytes");
//         wdump((free_ram)(init_bytes)(initial_ram_usage)(more_ram));
////   buyrambytes(config::system_account_name, N(testram11111), more_ram);
////   buyrambytes(config::system_account_name, N(testram22222), more_ram);
//         produce_blocks(10);
//         PRINT_USAGE(testram11111)
//         PRINT_USAGE(testram22222)
//
//         // allocate just under the allocated bytes
//         tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//               ("payer", "testram11111")
//               ("from", 1)
//               ("to", count)
//               ("size", per_elem_ram /*1910*/));
//         produce_blocks(10);
//         auto ram_usage = rlm.get_account_ram_usage(N(testram11111));
//
//         total = get_total_stake(N(testram11111));
//         const auto ram_bytes = total["ram_bytes"].as_uint64();
//         wdump((ram_bytes)(ram_usage)(initial_ram_usage)(init_bytes)(ram_usage - initial_ram_usage)(
//               init_bytes - ram_usage));
//
//         wlog("ram_tests 1    %%%%%%");
//         // allocate just beyond the allocated bytes
//         BOOST_REQUIRE_EXCEPTION(
//               tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//                     ("payer", "testram11111")
//                     ("from", 1)
//                     ("to", 10)
//                     ("size", per_elem_ram + 10 /*1920*/)),
//               ram_usage_exceeded,
//               fc_exception_message_starts_with("account testram11111 has insufficient ram"));
//         PRINT_USAGE(testram11111)
//         wlog("ram_tests 2    %%%%%%");
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//         BOOST_REQUIRE_EQUAL(ram_usage, rlm.get_account_ram_usage(N(testram11111)));
//
//         // update the entries with smaller allocations so that we can verify space is freed and new allocations can be made
//         tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//               ("payer", "testram11111")
//               ("from", 1)
//               ("to", 10)
//               ("size", 1680/*1810*/));
//         produce_blocks(1);
//         BOOST_REQUIRE_EQUAL(ram_usage - 1000, rlm.get_account_ram_usage(N(testram11111)));
//         PRINT_USAGE(testram11111)
//
//         // verify the added entry is beyond the allocation bytes limit
//         BOOST_REQUIRE_EXCEPTION(
//               tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//                     ("payer", "testram11111")
//                     ("from", 1)
//                     ("to", 11)
//                     ("size", 1680/*1810*/)),
//               ram_usage_exceeded,
//               fc_exception_message_starts_with("account testram11111 has insufficient ram"));
//         produce_blocks(1);
//         BOOST_REQUIRE_EQUAL(ram_usage - 1000, rlm.get_account_ram_usage(N(testram11111)));
//         PRINT_USAGE(testram11111)
//
//         // verify the new entry's bytes minus the freed up bytes for existing entries still exceeds the allocation bytes limit
//         BOOST_REQUIRE_EXCEPTION(
//               tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//                     ("payer", "testram11111")
//                     ("from", 1)
//                     ("to", 11)
//                     ("size", 1760)),
//               ram_usage_exceeded,
//               fc_exception_message_starts_with("account testram11111 has insufficient ram"));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//         BOOST_REQUIRE_EQUAL(ram_usage - 1000, rlm.get_account_ram_usage(N(testram11111)));
//
//         // verify the new entry's bytes minus the freed up bytes for existing entries are under the allocation bytes limit
//         tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//               ("payer", "testram11111")
//               ("from", 1)
//               ("to", 11)
//               ("size", 1600/*1720*/));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//
//         tester->push_action(N(testram11111), N(rmentry), N(testram11111), mvo()
//               ("from", 3)
//               ("to", 3));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//
//         // verify that the new entry will exceed the allocation bytes limit
//         BOOST_REQUIRE_EXCEPTION(
//               tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//                     ("payer", "testram11111")
//                     ("from", 12)
//                     ("to", 12)
//                     ("size", 1780)),
//               ram_usage_exceeded,
//               fc_exception_message_starts_with("account testram11111 has insufficient ram"));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//
//         // verify that the new entry is under the allocation bytes limit
//         tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//               ("payer", "testram11111")
//               ("from", 12)
//               ("to", 12)
//               ("size", 1620/*1720*/));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//
//         // verify that anoth new entry will exceed the allocation bytes limit, to setup testing of new payer
//         BOOST_REQUIRE_EXCEPTION(
//               tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//                     ("payer", "testram11111")
//                     ("from", 13)
//                     ("to", 13)
//                     ("size", 1660)),
//               ram_usage_exceeded,
//               fc_exception_message_starts_with("account testram11111 has insufficient ram"));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//
//         // verify that the new entry is under the allocation bytes limit
//         tester->push_action(N(testram11111), N(setentry), {N(testram11111), N(testram22222)}, mvo()
//               ("payer", "testram22222")
//               ("from", 12)
//               ("to", 12)
//               ("size", 1720));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//         PRINT_USAGE(testram22222)
//
//         // verify that another new entry that is too big will exceed the allocation bytes limit, to setup testing of new payer
//         BOOST_REQUIRE_EXCEPTION(
//               tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//                     ("payer", "testram11111")
//                     ("from", 13)
//                     ("to", 13)
//                     ("size", 1900)),
//               ram_usage_exceeded,
//               fc_exception_message_starts_with("account testram11111 has insufficient ram"));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//
//         wlog("ram_tests 18    %%%%%%");
//         // verify that the new entry is under the allocation bytes limit, because entry 12 is now charged to testram22222
//         tester->push_action(N(testram11111), N(setentry), N(testram11111), mvo()
//               ("payer", "testram11111")
//               ("from", 13)
//               ("to", 13)
//               ("size", 1720));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//
//         // verify that new entries for testram22222 exceed the allocation bytes limit
//         BOOST_REQUIRE_EXCEPTION(
//               tester->push_action(N(testram11111), N(setentry), {N(testram11111), N(testram22222)}, mvo()
//                     ("payer", "testram22222")
//                     ("from", 12)
//                     ("to", 21)
//                     ("size", 1930)),
//               ram_usage_exceeded,
//               fc_exception_message_starts_with("account testram22222 has insufficient ram"));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//         PRINT_USAGE(testram22222)
//
//         // verify that new entries for testram22222 are under the allocation bytes limit
//         tester->push_action(N(testram11111), N(setentry), {N(testram11111), N(testram22222)}, mvo()
//               ("payer", "testram22222")
//               ("from", 12)
//               ("to", 21)
//               ("size", 1910));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//         PRINT_USAGE(testram22222)
//
//         // verify that new entry for testram22222 exceed the allocation bytes limit
//         BOOST_REQUIRE_EXCEPTION(
//               tester->push_action(N(testram11111), N(setentry), {N(testram11111), N(testram22222)}, mvo()
//                     ("payer", "testram22222")
//                     ("from", 22)
//                     ("to", 22)
//                     ("size", 1910)),
//               ram_usage_exceeded,
//               fc_exception_message_starts_with("account testram22222 has insufficient ram"));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//         PRINT_USAGE(testram22222)
//
//         tester->push_action(N(testram11111), N(rmentry), N(testram11111), mvo()
//               ("from", 20)
//               ("to", 20));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//
//         // verify that new entry for testram22222 are under the allocation bytes limit
//         tester->push_action(N(testram11111), N(setentry), {N(testram11111), N(testram22222)}, mvo()
//               ("payer", "testram22222")
//               ("from", 22)
//               ("to", 22)
//               ("size", 1910));
//         produce_blocks(1);
//         PRINT_USAGE(testram11111)
//         PRINT_USAGE(testram22222)

      } FC_LOG_AND_RETHROW()
   }

BOOST_AUTO_TEST_SUITE_END()
