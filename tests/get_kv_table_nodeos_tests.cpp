#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/backing_store/kv_context.hpp>

#include <contracts.hpp>

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

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

BOOST_AUTO_TEST_SUITE(get_kv_table_cleos_tests)


BOOST_FIXTURE_TEST_CASE( get_kv_table_cleos_test, TESTER ) try {
   produce_blocks(2);

   create_accounts({ "kvtable"_n });

   produce_block();

   set_code(config::system_account_name, contracts::kv_bios_wasm());
   set_abi(config::system_account_name, contracts::kv_bios_abi().data());
   push_action("eosio"_n, "ramkvlimits"_n, "eosio"_n, mutable_variant_object()("k", 1024)("v", 1024)("i", 1024));
   produce_blocks(1);

   set_code( "kvtable"_n, contracts::kv_table_test_wasm() );
   set_abi( "kvtable"_n, contracts::kv_table_test_abi().data() );
   produce_blocks(1);

   auto arg = mutable_variant_object();
   push_action("kvtable"_n, "setup"_n, "kvtable"_n, arg );


   eosio::chain_apis::read_only plugin(*(this->control), {}, fc::microseconds::maximum());
   eosio::chain_apis::read_only::get_kv_table_rows_params p;
   p.code = "kvtable"_n;
   p.table = "kvtable"_n;
   p.index_name = "primarykey"_n;
   p.index_value = "boba";
   p.encode_type = "name";
   p.lower_bound = "";
   p.upper_bound = "";
   p.json = true;
   p.reverse = false;
   eosio::chain_apis::read_only::get_table_rows_result result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());

   p.index_name = "primarykey"_n;
   p.index_value = "bobd";
   p.encode_type = "name";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());

   p.index_name = "primarykey"_n;
   p.index_value = "bobx";
   p.encode_type = "name";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.index_name = "primarykey"_n;
   p.index_value = "";
   p.encode_type = "name";
   p.lower_bound = "bobb";
   p.upper_bound = "bobe";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());

   p.code = "kvtable"_n;
   p.table = "kvtable"_n;
   p.index_name = "foo"_n;
   p.index_value = "1";
   p.encode_type = "hex";
   p.lower_bound = "";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());

   p.code = "kvtable"_n;
   p.table = "kvtable"_n;
   p.index_name = "foo"_n;
   p.index_value = "3";
   p.encode_type = "dec";
   p.lower_bound = "";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());

   p.code = "kvtable"_n;
   p.table = "kvtable"_n;
   p.index_name = "foo"_n;
   p.index_value = "999";
   p.encode_type = "dec";
   p.lower_bound = "";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.code = "kvtable"_n;
   p.table = "kvtable"_n;
   p.index_name = "foo"_n;
   p.index_value = "";
   p.encode_type = "dec";
   p.lower_bound = "2";
   p.upper_bound = "7";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(5u, result.rows.size());

   p.code = "kvtable"_n;
   p.table = "kvtable"_n;
   p.index_name = "bar"_n;
   p.index_value = "boba";
   p.encode_type = "string";
   p.lower_bound = "";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());

   p.code = "kvtable"_n;
   p.table = "kvtable"_n;
   p.index_name = "bar"_n;
   p.index_value = "x";
   p.encode_type = "string";
   p.lower_bound = "";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.code = "kvtable"_n;
   p.table = "kvtable"_n;
   p.index_name = "bar"_n;
   p.index_value = "";
   p.encode_type = "string";
   p.lower_bound = "bobb";
   p.upper_bound = "bobe";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
