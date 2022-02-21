#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <contracts.hpp>

#include <fc/io/fstream.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <array>
#include <utility>

#include <eosio/testing/backing_store_tester_macros.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

BOOST_AUTO_TEST_SUITE(get_table_seckey_tests)

using backing_store_ts = boost::mpl::list<TESTER, ROCKSDB_TESTER>;

transaction_trace_ptr
issue_tokens( TESTER& t, account_name issuer, account_name to, const asset& amount,
              std::string memo = "", account_name token_contract = "eosio"_n )
{
   signed_transaction trx;

   trx.actions.emplace_back( t.get_action( token_contract, "issue"_n,
                                           vector<permission_level>{{issuer, config::active_name}},
                                           mutable_variant_object()
                                             ("to", issuer.to_string())
                                             ("quantity", amount)
                                             ("memo", memo)
   ) );

   trx.actions.emplace_back( t.get_action( token_contract, "transfer"_n,
                                           vector<permission_level>{{issuer, config::active_name}},
                                           mutable_variant_object()
                                             ("from", issuer.to_string())
                                             ("to", to.to_string())
                                             ("quantity", amount)
                                             ("memo", memo)
   ) );

   t.set_transaction_headers(trx);
   trx.sign( t.get_private_key( issuer, "active" ), t.control->get_chain_id()  );
   return t.push_transaction( trx );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( get_table_next_key_test, TESTER_T, backing_store_ts) { try {
   TESTER_T t;
   t.create_account("test"_n);

   // setup contract and abi
   t.set_code( "test"_n, contracts::get_table_seckey_test_wasm() );
   t.set_abi( "test"_n, contracts::get_table_seckey_test_abi().data() );
   t.produce_block();

   chain_apis::read_only plugin(*(t.control), {}, fc::microseconds::maximum());
   chain_apis::read_only::get_table_rows_params params = []{
      chain_apis::read_only::get_table_rows_params params{};
      params.json=true;
      params.code="test"_n;
      params.scope="test";
      params.limit=1;
      return params;
   }();

   params.table = "numobjs"_n;


   // name secondary key type
   t.push_action("test"_n, "addnumobj"_n, "test"_n, mutable_variant_object()("input", 2)("nm", "a"));
   t.push_action("test"_n, "addnumobj"_n, "test"_n, mutable_variant_object()("input", 5)("nm", "b"));
   t.push_action("test"_n, "addnumobj"_n, "test"_n, mutable_variant_object()("input", 7)("nm", "c"));

   params.table = "numobjs"_n;
   params.key_type = "name";
   params.limit = 10;
   params.index_position = "6";
   params.lower_bound = "a";
   params.upper_bound = "a";
   auto res_nm = plugin.get_table_rows(params);
   BOOST_REQUIRE(res_nm.rows.size() == 1);

   params.lower_bound = "a";
   params.upper_bound = "b";
   res_nm = plugin.get_table_rows(params);
   BOOST_REQUIRE(res_nm.rows.size() == 2);

   params.lower_bound = "a";
   params.upper_bound = "c";
   res_nm = plugin.get_table_rows(params);
   BOOST_REQUIRE(res_nm.rows.size() == 3);

   t.push_action("test"_n, "addnumobj"_n, "test"_n, mutable_variant_object()("input", 8)("nm", "1111"));
   t.push_action("test"_n, "addnumobj"_n, "test"_n, mutable_variant_object()("input", 9)("nm", "2222"));
   t.push_action("test"_n, "addnumobj"_n, "test"_n, mutable_variant_object()("input", 10)("nm", "3333"));

   params.lower_bound = "1111";
   params.upper_bound = "3333";
   res_nm = plugin.get_table_rows(params);
   BOOST_REQUIRE(res_nm.rows.size() == 3);

   params.lower_bound = "2222";
   params.upper_bound = "3333";
   res_nm = plugin.get_table_rows(params);
   BOOST_REQUIRE(res_nm.rows.size() == 2);

} FC_LOG_AND_RETHROW() }/// get_table_next_key_test

BOOST_AUTO_TEST_CASE_TEMPLATE( get_table_next_key_reverse_test, TESTER_T, backing_store_ts) { try {
   TESTER_T t;
   t.produce_blocks(2);

   t.create_accounts({ "eosio.token"_n, "eosio.ram"_n, "eosio.ramfee"_n, "eosio.stake"_n,
      "eosio.bpay"_n, "eosio.vpay"_n, "eosio.saving"_n, "eosio.names"_n });

   std::vector<account_name> accs{"inita"_n, "initb"_n, "initc"_n, "initd"_n};
   t.create_accounts(accs);
   t.produce_block();

   t.set_code( "eosio"_n, contracts::eosio_token_wasm() );
   t.set_abi( "eosio"_n, contracts::eosio_token_abi().data() );
   t.produce_blocks(1);

   // create currency
   auto act = mutable_variant_object()
         ("issuer",       "eosio")
         ("maximum_supply", eosio::chain::asset::from_string("1000000000.0000 SYS"));
   t.push_action("eosio"_n, "create"_n, "eosio"_n, act );

   // issue
   for (account_name a: accs) {
      issue_tokens( t, config::system_account_name, a, eosio::chain::asset::from_string("999.0000 SYS") );
   }
   t.produce_blocks(1);

   // iterate over scope
   eosio::chain_apis::read_only plugin(*(t.control), {}, fc::microseconds::maximum());
   eosio::chain_apis::read_only::get_table_by_scope_params param{"eosio"_n, "accounts"_n, "inita", "", 10};

   param.lower_bound = "a";
   param.upper_bound = "z";
   param.reverse = true;
   eosio::chain_apis::read_only::get_table_by_scope_result result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(5u, result.rows.size());
   BOOST_REQUIRE_EQUAL("", result.more);

   BOOST_REQUIRE_EQUAL(name("initd"_n), result.rows[0].scope);
   BOOST_REQUIRE_EQUAL(name("initc"_n), result.rows[1].scope);
   BOOST_REQUIRE_EQUAL(name("initb"_n), result.rows[2].scope);
   BOOST_REQUIRE_EQUAL(name("inita"_n), result.rows[3].scope);
   BOOST_REQUIRE_EQUAL(name("eosio"_n), result.rows[4].scope);

} FC_LOG_AND_RETHROW() } /// get_scope_test

BOOST_AUTO_TEST_SUITE_END()
