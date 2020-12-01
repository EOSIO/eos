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

BOOST_AUTO_TEST_SUITE_END()
