#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <arisen/testing/tester.hpp>
#include <arisen/chain/abi_serializer.hpp>
#include <arisen/chain/wasm_arisen_constraints.hpp>
#include <arisen/chain/resource_limits.hpp>
#include <arisen/chain/exceptions.hpp>
#include <arisen/chain/wast_to_wasm.hpp>
#include <arisen/chain_plugin/chain_plugin.hpp>

#include <asserter/asserter.wast.hpp>
#include <asserter/asserter.abi.hpp>

#include <stltest/stltest.wast.hpp>
#include <stltest/stltest.abi.hpp>

#include <arisen.system/arisen.system.wast.hpp>
#include <arisen.system/arisen.system.abi.hpp>

#include <arisen.token/arisen.token.wast.hpp>
#include <arisen.token/arisen.token.abi.hpp>

#include <fc/io/fstream.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <array>
#include <utility>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace arisen;
using namespace arisen::chain;
using namespace arisen::testing;
using namespace fc;

BOOST_AUTO_TEST_SUITE(get_table_tests)

BOOST_FIXTURE_TEST_CASE( get_scope_test, TESTER ) try {
   produce_blocks(2);

   create_accounts({ N(arisen.token), N(arisen.ram), N(arisen.ramfee), N(arisen.stake),
      N(eosio.bpay), N(eosio.vpay), N(eosio.saving), N(eosio.names) });

   std::vector<account_name> accs{N(inita), N(initb), N(initc), N(initd)};
   create_accounts(accs);
   produce_block();

   set_code( N(arisen.token), arisen_token_wast );
   set_abi( N(arisen.token), arisen_token_abi );
   produce_blocks(1);

   // create currency 
   auto act = mutable_variant_object()
         ("issuer",       "eosio")
         ("maximum_supply", arisen::chain::asset::from_string("1000000000.0000 SYS"));
   push_action(N(arisen.token), N(create), N(arisen.token), act );

   // issue
   for (account_name a: accs) {
      push_action( N(arisen.token), N(issue), "eosio", mutable_variant_object()
                  ("to",      name(a) )
                  ("quantity", arisen::chain::asset::from_string("999.0000 SYS") )
                  ("memo", "")
                  );
   }
   produce_blocks(1);

   // iterate over scope
   arisen::chain_apis::read_only plugin(*(this->control), fc::microseconds(INT_MAX));
   arisen::chain_apis::read_only::get_table_by_scope_params param{N(arisen.token), N(accounts), "inita", "", 10};
   arisen::chain_apis::read_only::get_table_by_scope_result result = plugin.read_only::get_table_by_scope(param);

   BOOST_REQUIRE_EQUAL(4, result.rows.size());
   BOOST_REQUIRE_EQUAL("", result.more);
   if (result.rows.size() >= 4) {
      BOOST_REQUIRE_EQUAL(name(N(arisen.token)), result.rows[0].code);
      BOOST_REQUIRE_EQUAL(name(N(inita)), result.rows[0].scope);
      BOOST_REQUIRE_EQUAL(name(N(accounts)), result.rows[0].table);
      BOOST_REQUIRE_EQUAL(name(N(eosio)), result.rows[0].payer);
      BOOST_REQUIRE_EQUAL(1, result.rows[0].count);

      BOOST_REQUIRE_EQUAL(name(N(initb)), result.rows[1].scope);
      BOOST_REQUIRE_EQUAL(name(N(initc)), result.rows[2].scope);
      BOOST_REQUIRE_EQUAL(name(N(initd)), result.rows[3].scope);
   }

   param.lower_bound = "initb";
   param.upper_bound = "initd";
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(2, result.rows.size());
   BOOST_REQUIRE_EQUAL("", result.more);
   if (result.rows.size() >= 2) {
      BOOST_REQUIRE_EQUAL(name(N(initb)), result.rows[0].scope);
      BOOST_REQUIRE_EQUAL(name(N(initc)), result.rows[1].scope);      
   }

   param.limit = 1;
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(1, result.rows.size());
   BOOST_REQUIRE_EQUAL("initc", result.more);

   param.table = name(0);
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(1, result.rows.size());
   BOOST_REQUIRE_EQUAL("initc", result.more);

   param.table = N(invalid);
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(0, result.rows.size());
   BOOST_REQUIRE_EQUAL("", result.more); 

} FC_LOG_AND_RETHROW() /// get_scope_test

BOOST_AUTO_TEST_SUITE_END()

