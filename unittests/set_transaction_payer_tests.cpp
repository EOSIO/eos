#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/global_property_object.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <contracts.hpp>

#include <eosio/testing/eosio_system_tester.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

class set_transaction_payer_tester : public TESTER {

};

BOOST_AUTO_TEST_SUITE(set_transaction_payer_tests)

BOOST_FIXTURE_TEST_CASE( resource_payer_test, set_transaction_payer_tester ) {
   account_name acc = "respayer"_n;
   create_accounts({ acc });
   set_code( acc, contracts::set_trx_payer_wasm() );
   set_abi( acc, contracts::set_trx_payer_abi().data() );

   push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  fc::mutable_variant_object()("account", acc)("is_priv", 1));

   produce_block();

   auto trace = push_action(acc, "settrxrespyr"_n, acc, mutable_variant_object());
   auto msg = trace->action_traces.front().console;
   BOOST_REQUIRE_EQUAL(msg, "%true");
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   produce_block();

   // verify in the transaction_context
   // TODO
}

BOOST_AUTO_TEST_SUITE_END()
