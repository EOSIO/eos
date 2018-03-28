#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester_network.hpp>
#include <eosio/chain/producer_object.hpp>
#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>
#include <currency/currency.wast.hpp>
#include <currency/currency.abi.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;


BOOST_AUTO_TEST_SUITE(delay_tests)

asset get_currency_balance(const tester& chain, account_name account) {
   return chain.get_currency_balance(N(currency), symbol(SY(4,CUR)), account);
}

// test link to permission with delay directly on it
BOOST_AUTO_TEST_CASE( link_delay_direct_test ) { try {
   tester chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("data",  authority(chain.get_public_key(tester_account, "first")))
           ("delay", 0));
   chain.push_action(config::system_account_name, contracts::linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   trace = chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("data",  authority(chain.get_public_key(tester_account, "first")))
           ("delay", 10));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" ),
       20
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace.status);
   BOOST_REQUIRE_EQUAL(1, trace.deferred_transaction_requests.size());
   BOOST_REQUIRE_EQUAL(0, trace.action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks(18);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test link to permission with delay on permission which is parent of min permission (special logic in permission_object::satisfies)
BOOST_AUTO_TEST_CASE( link_delay_direct_parent_permission_test ) { try {
   tester chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("data",  authority(chain.get_public_key(tester_account, "first")))
           ("delay", 0));
   chain.push_action(config::system_account_name, contracts::linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   trace = chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "active")
           ("parent", "owner")
           ("data",  authority(chain.get_public_key(tester_account, "active")))
           ("delay", 15));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" ),
       20
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace.status);
   BOOST_REQUIRE_EQUAL(1, trace.deferred_transaction_requests.size());
   BOOST_REQUIRE_EQUAL(0, trace.action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks(28);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test link to permission with delay on permission between min permission and authorizing permission it
BOOST_AUTO_TEST_CASE( link_delay_direct_walk_parent_permissions_test ) { try {
   tester chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("data",  authority(chain.get_public_key(tester_account, "first")))
           ("delay", 0));
   chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "first")
           ("data",  authority(chain.get_public_key(tester_account, "second")))
           ("delay", 0));
   chain.push_action(config::system_account_name, contracts::linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "second"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   trace = chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("data",  authority(chain.get_public_key(tester_account, "first")))
           ("delay", 20));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" ),
       30
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace.status);
   BOOST_REQUIRE_EQUAL(1, trace.deferred_transaction_requests.size());
   BOOST_REQUIRE_EQUAL(0, trace.action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks(38);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test removing delay on permission
BOOST_AUTO_TEST_CASE( link_delay_permission_change_test ) { try {
   tester chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("data",  authority(chain.get_public_key(tester_account, "first")))
           ("delay", 10));
   chain.push_action(config::system_account_name, contracts::linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 sec
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace.status);
   BOOST_REQUIRE_EQUAL(1, trace.deferred_transaction_requests.size());
   BOOST_REQUIRE_EQUAL(0, trace.action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 sec
   trace = chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("data",  authority(chain.get_public_key(tester_account, "first")))
           ("delay", 0),
           30);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace.status);
   BOOST_REQUIRE_EQUAL(1, trace.deferred_transaction_requests.size());
   BOOST_REQUIRE_EQUAL(0, trace.action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 sec
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace.status);
   BOOST_REQUIRE_EQUAL(1, trace.deferred_transaction_requests.size());
   BOOST_REQUIRE_EQUAL(0, trace.action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transactions.size());

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks(15);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test moving link with delay on permission
BOOST_AUTO_TEST_CASE( link_delay_link_change_test ) { try {
   tester chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("data",  authority(chain.get_public_key(tester_account, "first")))
           ("delay", 10));
   chain.push_action(config::system_account_name, contracts::linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));
   chain.push_action(config::system_account_name, contracts::updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "active")
           ("data",  authority(chain.get_public_key(tester_account, "second")))
           ("delay", 0));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transactions.size());

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 sec
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace.status);
   BOOST_REQUIRE_EQUAL(1, trace.deferred_transactions.size());
   BOOST_REQUIRE_EQUAL(0, trace.action_traces.size());
   BOOST_REQUIRE_EQUAL(0, trace.canceled_deferred.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 sec
   chain.push_action(config::system_account_name, contracts::linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "second"),
           30);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace.status);
   BOOST_REQUIRE_EQUAL(1, trace.deferred_transactions.size());
   BOOST_REQUIRE_EQUAL(0, trace.action_traces.size());
   BOOST_REQUIRE_EQUAL(0, trace.canceled_deferred.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 sec
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace.status);
   BOOST_REQUIRE_EQUAL(1, trace.deferred_transactions.size());
   BOOST_REQUIRE_EQUAL(0, trace.action_traces.size());
   BOOST_REQUIRE_EQUAL(0, trace.canceled_deferred.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace.status);
   BOOST_REQUIRE_EQUAL(0, trace.deferred_transaction_requests.size());

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks(15);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.control->push_deferred_transactions(true);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

BOOST_AUTO_TEST_SUITE_END()
