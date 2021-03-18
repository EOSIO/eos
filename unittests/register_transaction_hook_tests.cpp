#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/global_property_object.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <contracts.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

class register_transaction_hook_tester : public TESTER {

};

BOOST_AUTO_TEST_SUITE(register_trx_hook_tests)

BOOST_FIXTURE_TEST_CASE( preexecution_hook_test, register_transaction_hook_tester ) {
   create_accounts({"trxhookreg"_n});
   set_code( "trxhookreg"_n, contracts::trxhookreg_wasm() );
   set_abi( "trxhookreg"_n, contracts::trxhookreg_abi().data() );

   push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  fc::mutable_variant_object()("account", "trxhookreg"_n)("is_priv", 1));

   auto trace = push_action("trxhookreg"_n, "preexreg"_n, "trxhookreg"_n, mutable_variant_object());
   auto msg = trace->action_traces.front().console;
   BOOST_REQUIRE_EQUAL(msg, "%false");
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   produce_block();

   create_accounts({"payloadless"_n});
   set_code( "payloadless"_n, contracts::payloadless_wasm() );
   set_abi( "payloadless"_n, contracts::payloadless_abi().data() );

   trace = push_action("trxhookreg"_n, "preexreg"_n, "trxhookreg"_n, mutable_variant_object());
   msg = trace->action_traces.front().console;
   BOOST_REQUIRE_EQUAL(msg, "%true");
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   produce_block();

   create_account("tester"_n);

   set_code("tester"_n, contracts::get_table_test_wasm());
   set_abi("tester"_n, contracts::get_table_test_abi().data());

   produce_blocks(2);

   trace = push_action("tester"_n, "addhashobj"_n, "tester"_n, mutable_variant_object()("hashinput", "hello"));
   BOOST_REQUIRE_EQUAL(trace->action_traces.size(), 1);
   BOOST_REQUIRE_EQUAL(trace->action_traces[0].act.name, "addhashobj"_n);
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   produce_blocks(2);

   // spot directly on the global property object
   const auto& gpo = control->db().get<global_property_object>();

   BOOST_REQUIRE_EQUAL(gpo.transaction_hooks.size(), 1);
   BOOST_REQUIRE_EQUAL(gpo.transaction_hooks[0].type, 0);
   BOOST_REQUIRE_EQUAL(gpo.transaction_hooks[0].contract, "payloadless"_n);
   BOOST_REQUIRE_EQUAL(gpo.transaction_hooks[0].action, "doit"_n);
}

BOOST_AUTO_TEST_SUITE_END()
