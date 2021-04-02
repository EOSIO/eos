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

   // TEST_NOT_ACTIVATED_WITH_SPONSOR (IT-TS-001)
   // 1.all the test contract using the player account with sponsor metadata
   auto trace = push_action("tester"_n, "addhashobj"_n, {"tester"_n, "respayer"_n }, mutable_variant_object()("hashinput", "hello"));
   //Verify that the contract runs as normal and bill the user account
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   BOOST_REQUIRE_EQUAL(trace->bill_to_accounts.size(), 1);
   BOOST_REQUIRE_EQUAL(*trace->bill_to_accounts.begin(), "tester"_n);

   produce_block();

   // Activate features
   create_accounts({"chgpayerhk"_n});
   set_code( "chgpayerhk"_n, contracts::chgpayerhk_wasm() );
   set_abi( "chgpayerhk"_n, contracts::chgpayerhk_abi().data() );

   push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  fc::mutable_variant_object()("account", "chgpayerhk"_n)("is_priv", 1));

   trace = push_action("chgpayerhk"_n, "changepayer"_n, "chgpayerhk"_n, mutable_variant_object());
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   auto msg = trace->action_traces.front().console;
   BOOST_REQUIRE_EQUAL(msg, "true");

   produce_block();

   // TEST_ACTIVATED_NO_SPONSOR (IT-TS-002)
   // 1. Call the test contract using the player account without any transaction metadata
   trace = push_action("tester"_n, "addhashobj"_n, {"tester"_n }, mutable_variant_object()("hashinput", "hello"));
   // 2. Verify that the user account is billed for the usage
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   BOOST_REQUIRE_EQUAL(trace->bill_to_accounts.size(), 1);
   BOOST_REQUIRE_EQUAL(*trace->bill_to_accounts.begin(), "tester"_n);

   // Test Case TEST_ACTIVATED_SELF_SPONSORED (IT-TS-003)
   // 1. Call the test contract using the player account with sponsor metadata
   trace = push_action("tester"_n, "addhashobj"_n, {"tester"_n }, mutable_variant_object()("hashinput", "hello2"));
   // 2. Verify that the user account is billed for the usage
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   BOOST_REQUIRE_EQUAL(trace->bill_to_accounts.size(), 1);
   BOOST_REQUIRE_EQUAL(*trace->bill_to_accounts.begin(), "tester"_n);

   // Test Case TEST_WITH_SPONSOR_NO_SIG (IT-TS-004)
   // Call the test contract using the player account with sponsor metadata
   vector<account_name> empty;
   // Verify that the contract fails
   BOOST_CHECK_EXCEPTION(push_action("tester"_n, "addhashobj"_n, empty, mutable_variant_object()("hashinput", "hello2")), tx_no_auths,
                         [](const fc::assert_exception& e) {
                            return expect_assert_message(e, "transaction must have at least one authorization");
                         }
   );

   // Test Case TEST_WITH_SPONSOR (IT-TS-005)
   // 1. Call the test contract signed with the sponsor key and using the player account with sponsor metadata
   trace = push_action("tester"_n, "addhashobj"_n, {"tester"_n , "respayer"_n }, mutable_variant_object()("hashinput", "hello2"));
   // 2. Verify that the contract succeeds and the sponsor account is billed
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);
   BOOST_REQUIRE_EQUAL(trace->bill_to_accounts.size(), 1);
   BOOST_REQUIRE_EQUAL(*trace->bill_to_accounts.begin(), "respayer"_n);
}

BOOST_AUTO_TEST_SUITE_END()
