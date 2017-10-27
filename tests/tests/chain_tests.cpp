/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/permission_link_object.hpp>
#include <eos/chain/authority_checker.hpp>

#include <eos/native_contract/producer_objects.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/permutation.hpp>

#include "../common/database_fixture.hpp"

using namespace eos;
using namespace chain;


BOOST_AUTO_TEST_SUITE(chain_tests)

// Test transaction signature chain_controller::get_required_keys
BOOST_FIXTURE_TEST_CASE(get_required_keys, testing_fixture)
{ try {
      Make_Blockchain(chain)

      chain.set_auto_sign_transactions(false);
      chain.set_skip_transaction_signature_checking(false);

      SignedTransaction trx;
      trx.messages.resize(1);
      transaction_set_reference_block(trx, chain.head_block_id());
      trx.expiration = chain.head_block_time() + 100;
      trx.scope = sort_names( {"inita", "initb"} );
      types::transfer trans = { "inita", "initb", (100), "" };

      trx.messages[0].type = "transfer";
      trx.messages[0].authorization = {{"inita", "active"}};
      trx.messages[0].code = config::EosContractName;
      transaction_set_message(trx, 0, "transfer", trans);
      BOOST_REQUIRE_THROW(chain.push_transaction(trx), tx_missing_sigs);

      auto required_keys = chain.get_required_keys(trx, available_keys());
      BOOST_CHECK(required_keys.size() < available_keys().size()); // otherwise not a very good test
      chain.sign_transaction(trx); // uses get_required_keys
      chain.push_transaction(trx);

      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), Asset(100000 - 100));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("initb"), Asset(100000 + 100));

} FC_LOG_AND_RETHROW() }

// Test chain_controller::_transaction_message_rate message rate calculation
BOOST_FIXTURE_TEST_CASE(transaction_msg_rate_calculation, testing_fixture)
{ try {
   uint32_t now = 0;
   uint32_t last_update_sec = 0;
   uint32_t rate_limit_time_frame_sec = 10;
   uint32_t rate_limit = 10;
   uint32_t previous_rate = 9;
   auto rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                       rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(10, rate);

   previous_rate = 10;
   BOOST_CHECK_EXCEPTION(eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                                 rate_limit, previous_rate, "account", N(my.name)),\
                         tx_msgs_exceeded,
                         [](tx_msgs_exceeded const & e) -> bool { return true; } );

   last_update_sec = 10;
   now = 11;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                  rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(10, rate);

   now = 12;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                  rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(9, rate);

   now = 13;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                  rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(8, rate);

   now = 19;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                  rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(2, rate);

   now = 19;
   // our scenario will never have a previous_rate higher than max (since it was limited) but just checking algorithm
   previous_rate = 90;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                  rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(10, rate);

   now = 20;
   // our scenario will never have a previous_rate higher than max (since it was limited) but just checking algorithm
   previous_rate = 10000;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                  rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(1, rate);

   now = 2000;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                  rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(1, rate);

   rate_limit_time_frame_sec = 10000;
   now = 2010;
   previous_rate = 10;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                  rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(9, rate);

   rate_limit = 10000;
   now = 10000;
   last_update_sec = 9999;
   previous_rate = 10000;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                  rate_limit, previous_rate, "account", N(my.name));
   BOOST_CHECK_EQUAL(10000, rate);

   last_update_sec = 10000;
   BOOST_CHECK_EXCEPTION(eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                                 rate_limit, previous_rate, "account", N(my.name)),\
                         tx_msgs_exceeded,
                         [](tx_msgs_exceeded const & e) -> bool { return true; } );

} FC_LOG_AND_RETHROW() }

// Test chain_controller::_transaction_message_rate message rate calculation
BOOST_FIXTURE_TEST_CASE(transaction_msg_rate_running_calculation, testing_fixture)
{ try {
   uint32_t now = 1000;
   uint32_t last_update_sec = 1000;
   uint32_t rate_limit_time_frame_sec = 1000;
   uint32_t rate_limit = 1000;
   uint32_t rate = 0;
   for (uint32_t i = 0; i < 1000; ++i)
   {
      rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                          rate_limit, rate, "account", N(my.name));
   }
   BOOST_REQUIRE_EQUAL(1000, rate);

   BOOST_REQUIRE_EXCEPTION(eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                                   rate_limit, rate, "account", N(my.name)),\
                           tx_msgs_exceeded,
                           [](tx_msgs_exceeded const & e) -> bool { return true; } );

   ++now;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                       rate_limit, rate, "account", N(my.name));
   BOOST_REQUIRE_EQUAL(1000, rate);

   last_update_sec = now;
   BOOST_REQUIRE_EXCEPTION(eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                                   rate_limit, rate, "account", N(my.name)),\
                           tx_msgs_exceeded,
                           [](tx_msgs_exceeded const & e) -> bool { return true; } );

   now += 10;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                       rate_limit, rate, "account", N(my.name));
   last_update_sec = now;
   for (uint32_t i = 0; i < 9; ++i)
   {
      rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                          rate_limit, rate, "account", N(my.name));
   }
   BOOST_REQUIRE_EQUAL(1000, rate);

   BOOST_REQUIRE_EXCEPTION(eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                                   rate_limit, rate, "account", N(my.name)),\
                           tx_msgs_exceeded,
                           [](tx_msgs_exceeded const & e) -> bool { return true; } );

   for (uint32_t j = 0; j < 100; ++j)
   {

      now += 10;
      rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                          rate_limit, rate, "account", N(my.name));
      last_update_sec = now;
      for (uint32_t i = 0; i < 9; ++i)
      {
         rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                             rate_limit, rate, "account", N(my.name));
      }
      BOOST_REQUIRE_EQUAL(1000, rate);

      BOOST_REQUIRE_EXCEPTION(eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                                      rate_limit, rate, "account", N(my.name)),\
                              tx_msgs_exceeded,
                              [](tx_msgs_exceeded const & e) -> bool { return true; } );
   }

   now += 100;
   rate = eos::chain::chain_controller::_transaction_message_rate(now, last_update_sec, rate_limit_time_frame_sec,
                                                                       rate_limit, rate, "account", N(my.name));
   BOOST_REQUIRE_EQUAL(901, rate);
} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
