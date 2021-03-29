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

class transaction_sponsorship_tester : public TESTER {

};

BOOST_AUTO_TEST_SUITE(transaction_sponsorship_tests)

BOOST_FIXTURE_TEST_CASE( bill_to_accounts_tests, transaction_sponsorship_tester ) {
   create_accounts({"tester"_n, "respayer"_n});

   set_code("tester"_n, contracts::get_table_test_wasm());
   set_abi("tester"_n, contracts::get_table_test_abi().data());

   auto trace = push_action("tester"_n, "addhashobj"_n, "tester"_n, mutable_variant_object()("hashinput", "hello"));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   BOOST_REQUIRE_EQUAL(trace->bill_to_accounts.size(), 1);
   BOOST_REQUIRE_EQUAL(*trace->bill_to_accounts.begin(), "tester"_n);

   produce_block();

   create_accounts({"chgpayerhk"_n});
   set_code( "chgpayerhk"_n, contracts::chgpayerhk_wasm() );
   set_abi( "chgpayerhk"_n, contracts::chgpayerhk_abi().data() );

   push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  fc::mutable_variant_object()("account", "chgpayerhk"_n)("is_priv", 1));

   trace = push_action("chgpayerhk"_n, "changepayer"_n, "chgpayerhk"_n, mutable_variant_object());
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto msg = trace->action_traces.front().console;
   BOOST_REQUIRE_EQUAL(msg, "true");

   produce_block();

   trace = push_action("tester"_n, "addhashobj"_n, "tester"_n, mutable_variant_object()("hashinput", "hello2"));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   BOOST_REQUIRE_EQUAL(trace->bill_to_accounts.size(), 1);
   BOOST_REQUIRE_EQUAL(*trace->bill_to_accounts.begin(), "respayer"_n);
}

BOOST_AUTO_TEST_SUITE_END()
