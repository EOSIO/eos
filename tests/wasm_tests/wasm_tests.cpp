#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <asserter/asserter.wast.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;

struct assertdef {
   int8_t      condition;
   string      message;

   static scope_name get_scope() {
      return N(asserter);
   }

   static action_name get_name() {
      return N(procassert);
   }
};

FC_REFLECT(assertdef, (condition)(message));

struct provereset {
   static scope_name get_scope() {
      return N(asserter);
   }

   static action_name get_name() {
      return N(provereset);
   }
};

FC_REFLECT_EMPTY(provereset);

BOOST_AUTO_TEST_SUITE(wasm_tests)

/**
 * Prove that action reading and assertions are working
 */
BOOST_FIXTURE_TEST_CASE( basic_test, tester ) try {
   produce_blocks(2);

   create_accounts( {N(asserter)} );
   transfer( N(inita), N(asserter), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(asserter), asserter_wast);
   produce_blocks(1);

   transaction_id_type no_assert_id;
   {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{N(asserter),config::active_name}},
                                assertdef {1, "Should Not Assert!"} );

      set_tapos( trx );
      trx.sign( get_private_key( N(asserter), "active" ), chain_id_type() );
      control->push_transaction( trx );
      no_assert_id = trx.id();
   }

   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(no_assert_id));
   const auto& receipt = get_transaction_receipt(no_assert_id);
   BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);

   transaction_id_type yes_assert_id;
   {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{N(asserter),config::active_name}},
                                assertdef {0, "Should Assert!"} );

      set_tapos( trx );
      trx.sign( get_private_key( N(asserter), "active" ), chain_id_type() );
      BOOST_CHECK_THROW(control->push_transaction( trx ), fc::assert_exception);
      yes_assert_id = trx.id();
   }

   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(false, chain_has_transaction(yes_assert_id));

} FC_LOG_AND_RETHROW() /// basic_test

/**
 * Prove the modifications to global variables are wiped between runs
 */
BOOST_FIXTURE_TEST_CASE( prove_mem_reset, tester ) try {
   produce_blocks(2);

   create_accounts( {N(asserter)} );
   transfer( N(inita), N(asserter), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(asserter), asserter_wast);
   produce_blocks(1);

   // repeat the action multiple times, each time the action handler checks for the expected
   // default value then modifies the value which should not survive until the next invoction
   for (int i = 0; i < 5; i++) {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{N(asserter),config::active_name}},
                                provereset {} );

      set_tapos( trx );
      trx.sign( get_private_key( N(asserter), "active" ), chain_id_type() );
      control->push_transaction( trx );
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
      BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);
   }

} FC_LOG_AND_RETHROW() /// prove_mem_reset


BOOST_AUTO_TEST_SUITE_END()
