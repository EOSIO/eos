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

BOOST_AUTO_TEST_SUITE(get_kv_table_nodeos_tests)

BOOST_FIXTURE_TEST_CASE( get_kv_table_nodeos_test, TESTER ) try {
   eosio::chain_apis::read_only::get_table_rows_result result;
   auto chk_result = [&](int row, int data) {
      char bar[5] = {'b', 'o', 'b', 'a', 0};
      bar[3] += data - 1; // 'a' + data - 1


      BOOST_REQUIRE_EQUAL(bar, result.rows[row]["primary_key"].as_string());
      BOOST_REQUIRE_EQUAL(bar, result.rows[row]["bar"]);
      BOOST_REQUIRE_EQUAL(std::to_string(data), result.rows[row]["foo"].as_string());
   };

   produce_blocks(2);

   create_accounts({"kvtable"_n});

   produce_block();

   set_code(config::system_account_name, contracts::kv_bios_wasm());
   set_abi(config::system_account_name, contracts::kv_bios_abi().data());
   push_action("eosio"_n, "ramkvlimits"_n, "eosio"_n, mutable_variant_object()("k", 1024)("v", 1024)("i", 1024));
   produce_blocks(1);

   set_code("kvtable"_n, contracts::kv_table_test_wasm());
   set_abi("kvtable"_n, contracts::kv_table_test_abi().data());
   produce_blocks(1);

   auto arg = mutable_variant_object();
   push_action("kvtable"_n, "setup"_n, "kvtable"_n, arg);

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
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 1);

   p.index_name = "primarykey"_n;
   p.index_value = "bobj";
   p.encode_type = "name";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 10);

   p.index_value = "bobx";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.index_value = "";
   p.lower_bound = "bobb";
   p.upper_bound = "bobe";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());
   chk_result(0, 2);
   chk_result(1, 3);
   chk_result(2, 4);

   p.lower_bound = "aaaa";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(10u, result.rows.size());
   chk_result(0, 1);
   chk_result(1, 2);
   chk_result(2, 3);
   chk_result(3, 4);
   chk_result(4, 5);
   chk_result(5, 6);
   chk_result(6, 7);
   chk_result(7, 8);
   chk_result(8, 9);
   chk_result(9, 10);

   p.lower_bound = "boba";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(10u, result.rows.size());
   chk_result(0, 1);
   chk_result(9, 10);

   p.lower_bound = "bobj";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 10);

   p.lower_bound = "bobz";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.reverse = true;
   p.lower_bound = "";
   p.upper_bound = "bobz";
   result = plugin.read_only::get_kv_table_rows(p);
   chk_result(0, 10);
   chk_result(1, 9);
   chk_result(2, 8);
   chk_result(3, 7);
   chk_result(4, 6);
   chk_result(5, 5);
   chk_result(6, 4);
   chk_result(7, 3);
   chk_result(8, 2);
   chk_result(9, 1);

   p.lower_bound = "bobc";
   p.upper_bound = "";
   BOOST_CHECK_THROW(plugin.read_only::get_kv_table_rows(p), chain::contract_table_query_exception);

   p.reverse = false;
   p.lower_bound = "";
   p.upper_bound = "bobz";
   BOOST_CHECK_THROW(plugin.read_only::get_kv_table_rows(p), chain::contract_table_query_exception);

   p.reverse = true;
   p.lower_bound = "bobj";
   p.upper_bound = "bobz";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.lower_bound = "bobb";
   p.upper_bound = "bobe";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());
   chk_result(0, 5);
   chk_result(1, 4);
   chk_result(2, 3);

   p.lower_bound = "";
   p.upper_bound = "bobe";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(5u, result.rows.size());
   chk_result(0, 5);
   chk_result(1, 4);
   chk_result(2, 3);
   chk_result(3, 2);
   chk_result(4, 1);

   p.reverse = false;
   p.lower_bound = "boba";
   p.upper_bound = "";
   p.limit = 2;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(2u, result.rows.size());
   BOOST_REQUIRE_EQUAL(result.more, true);
   BOOST_REQUIRE_EQUAL(result.next_key != "", true);
   chk_result(0, 1);
   chk_result(1, 2);

   p.lower_bound = result.next_key;
   p.encode_type = "bytes";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(2u, result.rows.size());
   chk_result(0, 3);
   chk_result(1, 4);

   p.lower_bound = result.next_key;
   p.encode_type = "bytes";
   p.limit = 1;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   BOOST_REQUIRE_EQUAL(result.more, true);
   chk_result(0, 5);

   p.lower_bound = result.next_key;
   p.encode_type = "bytes";
   p.limit = 20;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(5u, result.rows.size());
   BOOST_REQUIRE_EQUAL(result.more, false);
   chk_result(0, 6);
   chk_result(1, 7);
   chk_result(2, 8);
   chk_result(3, 9);
   chk_result(4, 10);

   p.reverse = false;
   p.index_name = "foo"_n;
   p.index_value = "A"; // 10
   p.encode_type = "hex";
   p.lower_bound = "";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 10);

   p.index_value = "10";
   p.encode_type = "dec";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 10);

   p.index_value = "";
   p.encode_type = "dec";
   p.lower_bound = "0";
   p.upper_bound = "10";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(9u, result.rows.size());
   chk_result(0, 1);
   chk_result(1, 2);
   chk_result(2, 3);
   chk_result(3, 4);
   chk_result(4, 5);
   chk_result(5, 6);
   chk_result(6, 7);
   chk_result(7, 8);
   chk_result(8, 9);

   p.lower_bound = "2";
   p.upper_bound = "9";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(7u, result.rows.size());
   chk_result(0, 2);
   chk_result(1, 3);
   chk_result(2, 4);
   chk_result(3, 5);
   chk_result(4, 6);
   chk_result(5, 7);
   chk_result(6, 8);

   p.lower_bound = "0";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(10u, result.rows.size());
   chk_result(0, 1);
   chk_result(1, 2);
   chk_result(2, 3);
   chk_result(3, 4);
   chk_result(4, 5);
   chk_result(5, 6);
   chk_result(6, 7);
   chk_result(7, 8);
   chk_result(8, 9);

   p.lower_bound = "1";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(10u, result.rows.size());
   chk_result(0, 1);
   chk_result(1, 2);
   chk_result(2, 3);
   chk_result(3, 4);
   chk_result(4, 5);
   chk_result(5, 6);
   chk_result(6, 7);
   chk_result(7, 8);
   chk_result(8, 9);

   p.lower_bound = "10";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 10);

   p.lower_bound = "11";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.reverse = false;
   p.lower_bound = "0";
   p.upper_bound = "";
   p.limit = 2;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(2u, result.rows.size());
   BOOST_REQUIRE_EQUAL(result.more, true);
   chk_result(0, 1);
   chk_result(1, 2);

   p.lower_bound = result.next_key;
   p.encode_type = "bytes";
   p.limit = 3;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());
   BOOST_REQUIRE_EQUAL(result.more, true);
   chk_result(0, 3);
   chk_result(1, 4);
   chk_result(2, 5);

   p.lower_bound = result.next_key;
   p.encode_type = "bytes";
   p.limit = 20;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(5, result.rows.size());
   BOOST_REQUIRE_EQUAL(result.more, false);
   chk_result(0, 6);
   chk_result(1, 7);
   chk_result(2, 8);
   chk_result(3, 9);
   chk_result(4, 10);

   p.index_name = "bar"_n;
   p.index_value = "boba";
   p.encode_type = "string";
   p.lower_bound = "";
   p.upper_bound = "";
   p.reverse = false;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 1);

   p.index_value = "bobj";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 10);

   p.index_value = "bobx";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.index_value = "";
   p.lower_bound = "bobb";
   p.upper_bound = "bobe";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());
   chk_result(0, 2);
   chk_result(1, 3);
   chk_result(2, 4);

   p.lower_bound = "aaaa";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(10u, result.rows.size());
   chk_result(0, 1);

   p.lower_bound = "boba";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(10u, result.rows.size());
   chk_result(0, 1);

   p.lower_bound = "bobj";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 10);

   p.lower_bound = "bobz";
   p.upper_bound = "";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.reverse = true;
   p.lower_bound = "";
   p.upper_bound = "bobz";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(10u, result.rows.size());
   chk_result(0, 10);

   p.lower_bound = "bobj";
   p.upper_bound = "bobz";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.lower_bound = "bobb";
   p.upper_bound = "bobe";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());
   chk_result(0, 5);
   chk_result(1, 4);
   chk_result(2, 3);

   p.lower_bound = "";
   p.upper_bound = "bobe";
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(5u, result.rows.size());
   chk_result(0, 5);
   chk_result(1, 4);
   chk_result(2, 3);
   chk_result(3, 2);
   chk_result(4, 1);

   p.reverse = false;
   p.lower_bound = "boba";
   p.upper_bound = "";
   p.limit = 2;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(2u, result.rows.size());
   BOOST_REQUIRE_EQUAL(result.more, true);
   chk_result(0, 1);
   chk_result(1, 2);

   p.lower_bound = result.next_key;
   p.encode_type = "bytes";
   p.limit = 3;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());
   BOOST_REQUIRE_EQUAL(result.more, true);
   chk_result(0, 3);
   chk_result(1, 4);
   chk_result(2, 5);

   p.lower_bound = result.next_key;
   p.encode_type = "bytes";
   p.limit = 20;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(5u, result.rows.size());
   BOOST_REQUIRE_EQUAL(result.more, false);
   chk_result(0, 6);
   chk_result(1, 7);
   chk_result(2, 8);
   chk_result(3, 9);
   chk_result(4, 10);
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
