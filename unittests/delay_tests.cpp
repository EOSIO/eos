/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/testing/tester_network.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;


BOOST_AUTO_TEST_SUITE(delay_tests)

BOOST_FIXTURE_TEST_CASE( delay_create_account, validating_tester) { try {

   produce_blocks(2);
   signed_transaction trx;

   account_name a = N(newco);
   account_name creator = config::system_account_name;

   auto owner_auth =  authority( get_public_key( a, "owner" ) );
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             newaccount{
                                .creator  = creator,
                                .name     = a,
                                .owner    = owner_auth,
                                .active   = authority( get_public_key( a, "active" ) )
                             });
   set_transaction_headers(trx);
   trx.delay_sec = 3;
   trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );

   auto trace = push_transaction( trx );

   produce_blocks(8);

} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE( delay_error_create_account, validating_tester) { try {

   produce_blocks(2);
   signed_transaction trx;

   account_name a = N(newco);
   account_name creator = config::system_account_name;

   auto owner_auth =  authority( get_public_key( a, "owner" ) );
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             newaccount{
                                .creator  = N(bad), /// a does not exist, this should error when execute
                                .name     = a,
                                .owner    = owner_auth,
                                .active   = authority( get_public_key( a, "active" ) )
                             });
   set_transaction_headers(trx);
   trx.delay_sec = 3;
   trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );

   ilog( fc::json::to_pretty_string(trx) );
   auto trace = push_transaction( trx );
   edump((*trace));

   produce_blocks(6);

   auto scheduled_trxs = get_scheduled_transactions();
   BOOST_REQUIRE_EQUAL(scheduled_trxs.size(), 1u);

   auto billed_cpu_time_us = control->get_global_properties().configuration.min_transaction_cpu_usage;
   auto dtrace = control->push_scheduled_transaction(scheduled_trxs.front(), fc::time_point::maximum(), billed_cpu_time_us, true);
   BOOST_REQUIRE_EQUAL(dtrace->except.valid(), true);
   BOOST_REQUIRE_EQUAL(dtrace->except->code(), missing_auth_exception::code_value);

} FC_LOG_AND_RETHROW() }


asset get_currency_balance(const TESTER& chain, account_name account) {
   return chain.get_currency_balance(N(eosio.token), symbol(SY(4,CUR)), account);
}

const std::string eosio_token = name(N(eosio.token)).to_string();

// test link to permission with delay directly on it
BOOST_AUTO_TEST_CASE( link_delay_direct_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "first"));
   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token)
           ("maximum_supply", "9000000.0000 CUR")
   );


   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0u, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" ),
       20, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks(18);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test


BOOST_AUTO_TEST_CASE(delete_auth_test) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   transaction_trace_ptr trace;

   // can't delete auth because it doesn't exist
   BOOST_REQUIRE_EXCEPTION(
   trace = chain.push_action(config::system_account_name, deleteauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")),
   permission_query_exception,
   [] (const permission_query_exception &e)->bool {
      expect_assert_message(e, "permission_query_exception: Permission Query Exception\nFailed to retrieve permission");
      return true;
   });

   // update auth
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first")))
   );

   // link auth
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "eosio.token")
           ("type", "transfer")
           ("requirement", "first"));

   // create CUR token
   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", "eosio.token" )
           ("maximum_supply", "9000000.0000 CUR" )
   );

   // issue to account "eosio.token"
   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       "eosio.token")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   // transfer from eosio.token to tester
   trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", "eosio.token")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // can't delete auth because it's linked
   BOOST_REQUIRE_EXCEPTION(
   trace = chain.push_action(config::system_account_name, deleteauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")),
   action_validate_exception,
   [] (const action_validate_exception &e)->bool {
      expect_assert_message(e, "action_validate_exception: message validation exception\nCannot delete a linked authority");
      return true;
   });

   // unlink auth
   trace = chain.push_action(config::system_account_name, unlinkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "eosio.token")
           ("type", "transfer"));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   // delete auth
   trace = chain.push_action(config::system_account_name, deleteauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first"));

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   chain.produce_blocks(1);;

   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// delete_auth_test


// test link to permission with delay on permission which is parent of min permission (special logic in permission_object::satisfies)
BOOST_AUTO_TEST_CASE( link_delay_direct_parent_permission_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token)
           ("maximum_supply", "9000000.0000 CUR")
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "active")
           ("parent", "owner")
           ("auth",  authority(chain.get_public_key(tester_account, "active"), 15))
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" ),
       20, 15
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks(28);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test link to permission with delay on permission between min permission and authorizing permission it
BOOST_AUTO_TEST_CASE( link_delay_direct_walk_parent_permissions_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first")))
   );
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "first")
           ("auth",  authority(chain.get_public_key(tester_account, "second")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "second"));

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token)
           ("maximum_supply", "9000000.0000 CUR")
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 20))
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" ),
       30, 20
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks(38);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test removing delay on permission
BOOST_AUTO_TEST_CASE( link_delay_permission_change_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token )
           ("maximum_supply", "9000000.0000 CUR" )
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"))),
           30, 10);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // delayed update auth removing the delay will finally execute
   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks(15);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   // second transfer finally is performed
   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(0, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test removing delay on permission based on heirarchy delay
BOOST_AUTO_TEST_CASE( link_delay_permission_change_with_delay_heirarchy_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "first")
           ("auth",  authority(chain.get_public_key(tester_account, "second")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "second"));

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token)
           ("maximum_supply", "9000000.0000 CUR" )
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"))),
           30, 10
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks(14);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test moving link with delay on permission
BOOST_AUTO_TEST_CASE( link_delay_link_change_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "first"));
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "second")))
   );

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token)
           ("maximum_supply", "9000000.0000 CUR" )
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   BOOST_REQUIRE_EXCEPTION(
      chain.push_action( config::system_account_name, linkauth::get_name(),
                         vector<permission_level>{permission_level{tester_account, N(first)}},
                         fc::mutable_variant_object()
      ("account", "tester")
      ("code", eosio_token)
      ("type", "transfer")
      ("requirement", "second"),
      30, 3),
      unsatisfied_authorization,
      fc_exception_message_starts_with("transaction declares authority")
   );

   // this transaction will be delayed 20 blocks
   chain.push_action( config::system_account_name, linkauth::get_name(),
                      vector<permission_level>{{tester_account, N(first)}},
                      fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "second"),
           30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // delay on minimum permission of transfer is finally removed
   chain.produce_blocks();

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);


   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test


// test link with unlink
BOOST_AUTO_TEST_CASE( link_delay_unlink_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token )
           ("maximum_supply", "9000000.0000 CUR" )
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   BOOST_REQUIRE_EXCEPTION(
      chain.push_action( config::system_account_name, unlinkauth::get_name(),
                         vector<permission_level>{{tester_account, N(first)}},
                         fc::mutable_variant_object()
         ("account", "tester")
         ("code", eosio_token)
         ("type", "transfer"),
         30, 7
      ),
      unsatisfied_authorization,
      fc_exception_message_starts_with("transaction declares authority")
   );

   // this transaction will be delayed 20 blocks
   chain.push_action(config::system_account_name, unlinkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer"),
           30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // the delayed unlinkauth finally occurs
   chain.produce_blocks();

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks(15);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// link_delay_unlink_test

// test moving link with delay on permission's parent
BOOST_AUTO_TEST_CASE( link_delay_link_change_heirarchy_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "first")
           ("auth",  authority(chain.get_public_key(tester_account, "first")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "second"));
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "third")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "third")))
   );

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token)
           ("maximum_supply", "9000000.0000 CUR" )
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "third"),
           30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // delay on minimum permission of transfer is finally removed
   chain.produce_blocks();

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() } /// link_delay_link_change_heirarchy_test

// test delay_sec field imposing unneeded delay
BOOST_AUTO_TEST_CASE( mindelay_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token)
           ("maximum_supply", "9000000.0000 CUR")
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // send transfer with delay_sec set to 10
   const auto& acnt = chain.control->db().get<account_object,by_name>(N(eosio.token));
   const auto abi = acnt.get_abi();
   chain::abi_serializer abis(abi, chain.abi_serializer_max_time);
   const auto a = chain.control->db().get<account_object,by_name>(N(eosio.token)).get_abi();

   string action_type_name = abis.get_action_type(name("transfer"));

   action act;
   act.account = N(eosio.token);
   act.name = name("transfer");
   act.authorization.push_back(permission_level{N(tester), config::active_name});
   act.data = abis.variant_to_binary(action_type_name, fc::mutable_variant_object()
      ("from", "tester")
      ("to", "tester2")
      ("quantity", "3.0000 CUR")
      ("memo", "hi" ),
      chain.abi_serializer_max_time
   );

   signed_transaction trx;
   trx.actions.push_back(act);

   chain.set_transaction_headers(trx, 30, 10);
   trx.sign(chain.get_private_key(N(tester), "active"), chain.control->get_chain_id());
   trace = chain.push_transaction(trx);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks(18);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test canceldelay action cancelling a delayed transaction
BOOST_AUTO_TEST_CASE( canceldelay_test ) { try {
   TESTER chain;
   const auto& tester_account = N(tester);
   std::vector<transaction_id_type> ids;

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks(10);

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token)
           ("maximum_supply", "9000000.0000 CUR")
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();
   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   //wdump((fc::json::to_pretty_string(trace)));
   ids.push_back(trace->id);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   const auto& idx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
   auto itr = idx.find( trace->id );
   BOOST_CHECK_EQUAL( (itr != idx.end()), true );

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   BOOST_REQUIRE_EXCEPTION(
      chain.push_action( config::system_account_name,
                         updateauth::get_name(),
                         vector<permission_level>{{tester_account, N(first)}},
                         fc::mutable_variant_object()
            ("account", "tester")
            ("permission", "first")
            ("parent", "active")
            ("auth",  authority(chain.get_public_key(tester_account, "first"))),
            30, 7
      ),
      unsatisfied_authorization,
      fc_exception_message_starts_with("transaction declares authority")
   );

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"))),
           30, 10
   );
   //wdump((fc::json::to_pretty_string(trace)));
   ids.push_back(trace->id);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   //wdump((fc::json::to_pretty_string(trace)));
   ids.push_back(trace->id);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // send canceldelay for first delayed transaction
   signed_transaction trx;
   trx.actions.emplace_back(vector<permission_level>{{N(tester), config::active_name}},
                            chain::canceldelay{{N(tester), config::active_name}, ids[0]});

   chain.set_transaction_headers(trx);
   trx.sign(chain.get_private_key(N(tester), "active"), chain.control->get_chain_id());
   trace = chain.push_transaction(trx);
   //wdump((fc::json::to_pretty_string(trace)));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   const auto& cidx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
   auto citr = cidx.find( ids[0] );
   BOOST_CHECK_EQUAL( (citr == cidx.end()), true );

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   chain.produce_blocks();
   // update auth will finally be performed

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   //wdump((fc::json::to_pretty_string(trace)));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("90.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("10.0000 CUR"), liquid_balance);

   chain.produce_blocks(15);

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("90.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("10.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(0, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("85.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("15.0000 CUR"), liquid_balance);
} FC_LOG_AND_RETHROW() }

// test canceldelay action under different permission levels
BOOST_AUTO_TEST_CASE( canceldelay_test2 ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(eosio.token));
   chain.produce_blocks();

   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks();

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 5))
   );
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
          ("account", "tester")
          ("permission", "second")
          ("parent", "first")
          ("auth",  authority(chain.get_public_key(tester_account, "second")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", eosio_token)
           ("maximum_supply", "9000000.0000 CUR")
   );

   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       eosio_token)
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(eosio.token), name("transfer"), N(eosio.token), fc::mutable_variant_object()
       ("from", eosio_token)
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();
   auto liquid_balance = get_currency_balance(chain, N(eosio.token));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   ilog("attempting first delayed transfer");

   {
      // this transaction will be delayed 10 blocks
      trace = chain.push_action(N(eosio.token), name("transfer"), vector<permission_level>{{N(tester), N(first)}}, fc::mutable_variant_object()
          ("from", "tester")
          ("to", "tester2")
          ("quantity", "1.0000 CUR")
          ("memo", "hi" ),
          30, 5
      );
      auto trx_id = trace->id;
      BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(1, gen_size);
      BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

      const auto& idx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto itr = idx.find( trx_id );
      BOOST_CHECK_EQUAL( (itr != idx.end()), true );

      chain.produce_blocks();

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

      // attempt canceldelay with wrong canceling_auth for delayed transfer of 1.0000 CUR
      {
         signed_transaction trx;
         trx.actions.emplace_back(vector<permission_level>{{N(tester), config::active_name}},
                                  chain::canceldelay{{N(tester), config::active_name}, trx_id});
         chain.set_transaction_headers(trx);
         trx.sign(chain.get_private_key(N(tester), "active"), chain.control->get_chain_id());
         BOOST_REQUIRE_EXCEPTION( chain.push_transaction(trx), action_validate_exception,
                                  fc_exception_message_is("canceling_auth in canceldelay action was not found as authorization in the original delayed transaction") );
      }

      // attempt canceldelay with "second" permission for delayed transfer of 1.0000 CUR
      {
         signed_transaction trx;
         trx.actions.emplace_back(vector<permission_level>{{N(tester), N(second)}},
                                  chain::canceldelay{{N(tester), N(first)}, trx_id});
         chain.set_transaction_headers(trx);
         trx.sign(chain.get_private_key(N(tester), "second"), chain.control->get_chain_id());
         BOOST_REQUIRE_THROW( chain.push_transaction(trx), irrelevant_auth_exception );
         BOOST_REQUIRE_EXCEPTION( chain.push_transaction(trx), irrelevant_auth_exception,
                                  fc_exception_message_starts_with("canceldelay action declares irrelevant authority") );
      }

      // canceldelay with "active" permission for delayed transfer of 1.0000 CUR
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(tester), config::active_name}},
                               chain::canceldelay{{N(tester), N(first)}, trx_id});
      chain.set_transaction_headers(trx);
      trx.sign(chain.get_private_key(N(tester), "active"), chain.control->get_chain_id());
      trace = chain.push_transaction(trx);

      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(0, gen_size);

      const auto& cidx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto citr = cidx.find( trx_id );
      BOOST_REQUIRE_EQUAL( (citr == cidx.end()), true );

      chain.produce_blocks(10);

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);
   }

   ilog("reset minimum permission of transfer to second permission");

   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", eosio_token)
           ("type", "transfer")
           ("requirement", "second"),
           30, 5
   );

   chain.produce_blocks(11);


   ilog("attempting second delayed transfer");
   {
      // this transaction will be delayed 10 blocks
      trace = chain.push_action(N(eosio.token), name("transfer"), vector<permission_level>{{N(tester), N(second)}}, fc::mutable_variant_object()
          ("from", "tester")
          ("to", "tester2")
          ("quantity", "5.0000 CUR")
          ("memo", "hi" ),
          30, 5
      );
      auto trx_id = trace->id;
      BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
      auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_CHECK_EQUAL(1, gen_size);
      BOOST_CHECK_EQUAL(0, trace->action_traces.size());

      const auto& idx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto itr = idx.find( trx_id );
      BOOST_CHECK_EQUAL( (itr != idx.end()), true );

      chain.produce_blocks();

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

      // canceldelay with "first" permission for delayed transfer of 5.0000 CUR
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(tester), N(first)}},
                               chain::canceldelay{{N(tester), N(second)}, trx_id});
      chain.set_transaction_headers(trx);
      trx.sign(chain.get_private_key(N(tester), "first"), chain.control->get_chain_id());
      trace = chain.push_transaction(trx);

      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(0, gen_size);

      const auto& cidx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto citr = cidx.find( trx_id );
      BOOST_REQUIRE_EQUAL( (citr == cidx.end()), true );

      chain.produce_blocks(10);

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);
   }

   ilog("attempting third delayed transfer");

   {
      // this transaction will be delayed 10 blocks
      trace = chain.push_action(N(eosio.token), name("transfer"), vector<permission_level>{{N(tester), config::owner_name}}, fc::mutable_variant_object()
          ("from", "tester")
          ("to", "tester2")
          ("quantity", "10.0000 CUR")
          ("memo", "hi" ),
          30, 5
      );
      auto trx_id = trace->id;
      BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(1, gen_size);
      BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

      const auto& idx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto itr = idx.find( trx_id );
      BOOST_CHECK_EQUAL( (itr != idx.end()), true );

      chain.produce_blocks();

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

      // attempt canceldelay with "active" permission for delayed transfer of 10.0000 CUR
      {
         signed_transaction trx;
         trx.actions.emplace_back(vector<permission_level>{{N(tester), N(active)}},
                                  chain::canceldelay{{N(tester), config::owner_name}, trx_id});
         chain.set_transaction_headers(trx);
         trx.sign(chain.get_private_key(N(tester), "active"), chain.control->get_chain_id());
         BOOST_REQUIRE_THROW( chain.push_transaction(trx), irrelevant_auth_exception );
      }

      // canceldelay with "owner" permission for delayed transfer of 10.0000 CUR
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(tester), config::owner_name}},
                               chain::canceldelay{{N(tester), config::owner_name}, trx_id});
      chain.set_transaction_headers(trx);
      trx.sign(chain.get_private_key(N(tester), "owner"), chain.control->get_chain_id());
      trace = chain.push_transaction(trx);

      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(0, gen_size);

      const auto& cidx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto citr = cidx.find( trx_id );
      BOOST_REQUIRE_EQUAL( (citr == cidx.end()), true );

      chain.produce_blocks(10);

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);
   }
} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE( max_transaction_delay_create ) { try {
   //assuming max transaction delay is 45 days (default in config.hpp)
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.produce_blocks(10);

   BOOST_REQUIRE_EXCEPTION(
      chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
                        ("account", "tester")
                        ("permission", "first")
                        ("parent", "active")
                        ("auth",  authority(chain.get_public_key(tester_account, "first"), 50*86400)) ), // 50 days delay
      action_validate_exception,
      fc_exception_message_starts_with("Cannot set delay longer than max_transacton_delay")
   );
} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE( max_transaction_delay_execute ) { try {
   //assuming max transaction delay is 45 days (default in config.hpp)
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.create_account(N(eosio.token));
   chain.set_code(N(eosio.token), contracts::eosio_token_wasm());
   chain.set_abi(N(eosio.token), contracts::eosio_token_abi().data());

   chain.produce_blocks();
   chain.create_account(N(tester));

   chain.produce_blocks();
   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
           ("issuer", "eosio.token" )
           ("maximum_supply", "9000000.0000 CUR" )
   );
   chain.push_action(N(eosio.token), name("issue"), N(eosio.token), fc::mutable_variant_object()
           ("to",       "tester")
           ("quantity", "100.0000 CUR")
           ("memo", "for stuff")
   );

   //create a permission level with delay 30 days and associate it with token transfer
   auto trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
                     ("account", "tester")
                     ("permission", "first")
                     ("parent", "active")
                     ("auth",  authority(chain.get_public_key(tester_account, "first"), 30*86400)) // 30 days delay
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   trace = chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
                     ("account", "tester")
                     ("code", "eosio.token")
                     ("type", "transfer")
                     ("requirement", "first"));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   chain.produce_blocks();

   //change max_transaction_delay to 60 sec
   auto params = chain.control->get_global_properties().configuration;
   params.max_transaction_delay = 60;
   chain.push_action( config::system_account_name, N(setparams), config::system_account_name, mutable_variant_object()
                        ("params", params) );

   chain.produce_blocks();
   //should be able to create transaction with delay 60 sec, despite permission delay being 30 days, because max_transaction_delay is 60 sec
   trace = chain.push_action(N(eosio.token), name("transfer"), N(tester), fc::mutable_variant_object()
                           ("from", "tester")
                           ("to", "eosio.token")
                           ("quantity", "9.0000 CUR")
                           ("memo", "" ), 120, 60);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt->status);

   chain.produce_blocks();

   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   //check that the delayed transaction executed after after 60 sec
   chain.produce_blocks(120);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(0, gen_size);

   //check that the transfer really happened
   auto liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("91.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE( delay_expired, validating_tester) { try {

   produce_blocks(2);
   signed_transaction trx;

   account_name a = N(newco);
   account_name creator = config::system_account_name;

   auto owner_auth =  authority( get_public_key( a, "owner" ) );
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             newaccount{
                                .creator  = creator,
                                .name     = a,
                                .owner    = owner_auth,
                                .active   = authority( get_public_key( a, "active" ) )
                             });
   set_transaction_headers(trx);
   trx.delay_sec = 3;
   trx.expiration = control->head_block_time() + fc::microseconds(1000000);
   trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );

   auto trace = push_transaction( trx );

   BOOST_REQUIRE_EQUAL(transaction_receipt_header::delayed, trace->receipt->status);

   signed_block_ptr sb = produce_block();

   sb  = produce_block();

   BOOST_REQUIRE_EQUAL(transaction_receipt_header::delayed, trace->receipt->status);
   produce_empty_block(fc::milliseconds(610 * 1000));
   sb  = produce_block();
   BOOST_REQUIRE_EQUAL(1, sb->transactions.size());
   BOOST_REQUIRE_EQUAL(transaction_receipt_header::expired, sb->transactions[0].status);

   create_account(a); // account can still be created

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
