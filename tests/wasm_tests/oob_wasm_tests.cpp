#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/exceptions.hpp>

#include "oob_test_wasts.hpp"

#include <array>
#include <utility>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;


BOOST_AUTO_TEST_SUITE(oob_wasm_tests, *boost::unit_test::disabled())

BOOST_FIXTURE_TEST_CASE( oob_table_access, tester ) try {
   produce_blocks(2);

   create_accounts( {N(tbl)} );
   produce_block();

   set_code(N(tbl), table_oob_wast);
   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = 888ULL;
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
   set_tapos(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());

   //an element that is out of range, but within element maximum, and has no mmap access permission either (should be a trapped segv)
   BOOST_CHECK_EXCEPTION(push_transaction(trx), eosio::chain::wasm_execution_error, [](const eosio::chain::wasm_execution_error &e) {return true;});
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()