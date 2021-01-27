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

BOOST_AUTO_TEST_SUITE(get_kv_table_addr_tests)


BOOST_FIXTURE_TEST_CASE( get_kv_table_addr_test, TESTER ) try {
   eosio::chain_apis::read_only::get_table_rows_result result;
   auto chk_result = [&](int row, int data) {
      if( data == 1 )
      {
         BOOST_REQUIRE_EQUAL("jane", result.rows[row]["account_name"].as_string());
         BOOST_REQUIRE_EQUAL("jane", result.rows[row]["first_name"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("Jane", result.rows[row]["first_name"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("jane", result.rows[row]["last_name"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("Doe", result.rows[row]["last_name"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("1234 MyStreet", result.rows[row]["street_city_state_cntry"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("jane", result.rows[row]["personal_id"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("jdoe", result.rows[row]["personal_id"]["field_1"].as_string());
      }
      else if (data == 2)
      {
         BOOST_REQUIRE_EQUAL("john", result.rows[row]["account_name"].as_string());
         BOOST_REQUIRE_EQUAL("john", result.rows[row]["first_name"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("John", result.rows[row]["first_name"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("john", result.rows[row]["last_name"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("Smith", result.rows[row]["last_name"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("123 MyStreet", result.rows[row]["street_city_state_cntry"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("john", result.rows[row]["personal_id"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("jsmith", result.rows[row]["personal_id"]["field_1"].as_string());
      }
      else if (data == 3)
      {
         BOOST_REQUIRE_EQUAL("lois", result.rows[row]["account_name"].as_string());
         BOOST_REQUIRE_EQUAL("lois", result.rows[row]["first_name"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("Lois", result.rows[row]["first_name"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("lois", result.rows[row]["last_name"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("Lane", result.rows[row]["last_name"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("5432 MyStreet", result.rows[row]["street_city_state_cntry"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("lois", result.rows[row]["personal_id"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("llane", result.rows[row]["personal_id"]["field_1"].as_string());
      }
      else
      {
         BOOST_REQUIRE_EQUAL("steve", result.rows[row]["account_name"].as_string());
         BOOST_REQUIRE_EQUAL("steve", result.rows[row]["first_name"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("Steve", result.rows[row]["first_name"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("steve", result.rows[row]["last_name"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("Jones", result.rows[row]["last_name"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("321 MyStreet", result.rows[row]["street_city_state_cntry"]["field_1"].as_string());
         BOOST_REQUIRE_EQUAL("steve", result.rows[row]["personal_id"]["field_0"].as_string());
         BOOST_REQUIRE_EQUAL("sjones", result.rows[row]["personal_id"]["field_1"].as_string());
      }
   };

   produce_blocks(2);

   create_accounts({ "kvaddrbook"_n });

   produce_block();

   set_code(config::system_account_name, contracts::kv_bios_wasm());
   set_abi(config::system_account_name, contracts::kv_bios_abi().data());
   push_action("eosio"_n, "ramkvlimits"_n, "eosio"_n, mutable_variant_object()("k", 1024)("v", 1024)("i", 1024));
   produce_blocks(1);

   set_code( "kvaddrbook"_n, contracts::kv_addr_book_wasm() );
   set_abi( "kvaddrbook"_n, contracts::kv_addr_book_abi().data() );
   produce_blocks(1);

   auto arg = mutable_variant_object();
   push_action("kvaddrbook"_n, "test"_n, "kvaddrbook"_n, arg );


   eosio::chain_apis::read_only plugin(*(this->control), {}, fc::microseconds::maximum());
   eosio::chain_apis::read_only::get_kv_table_rows_params p;
   p.code = "kvaddrbook"_n;
   p.table = "kvaddrbook"_n;
   p.index_name = "accname"_n;
   p.index_value = "john";
   p.encode_type = "name";
   p.lower_bound = "";
   p.upper_bound = "";
   p.json = true;
   p.reverse = false;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   chk_result(0, 2);

   p.index_name = "accname"_n;
   p.index_value = "";
   p.encode_type = "name";
   p.lower_bound = "aaa";
   p.upper_bound = "";
   p.reverse = false;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(4u, result.rows.size());
   chk_result(0, 1);
   chk_result(1, 2);
   chk_result(2, 3);
   chk_result(3, 4);

   p.index_name = "accname"_n;
   p.index_value = "";
   p.encode_type = "name";
   p.lower_bound = "john";
   p.upper_bound = "";
   p.reverse = false;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());
   chk_result(0, 2);
   chk_result(1, 3);
   chk_result(2, 4);

   p.index_name = "accname"_n;
   p.index_value = "";
   p.encode_type = "name";
   p.lower_bound = "john";
   p.upper_bound = "lois";
   p.reverse = false;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(2u, result.rows.size());
   chk_result(0, 2);

   p.index_name = "accname"_n;
   p.index_value = "";
   p.encode_type = "name";
   p.lower_bound = "";
   p.upper_bound = "steve";
   p.reverse = true;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(4u, result.rows.size());
   chk_result(0, 4);
   chk_result(1, 3);
   chk_result(2, 2);
   chk_result(3, 1);

   p.index_name = "accname"_n;
   p.index_value = "";
   p.encode_type = "name";
   p.lower_bound = "john";
   p.upper_bound = "steve";
   p.reverse = true;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());
   chk_result(0, 4);
   chk_result(1, 3);

   p.index_name = "accname"_n;
   p.index_value = "";
   p.encode_type = "name";
   p.lower_bound = "";
   p.upper_bound = "aaaa";
   p.reverse = true;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.index_name = "accname"_n;
   p.index_value = "";
   p.encode_type = "name";
   p.lower_bound = "steve";
   p.upper_bound = "john";
   p.reverse = true;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   p.index_name = "accname"_n;
   p.index_value = "";
   p.encode_type = "name";
   p.lower_bound = "";
   p.upper_bound = "john";
   p.reverse = true;
   result = plugin.read_only::get_kv_table_rows(p);
   BOOST_REQUIRE_EQUAL(2u, result.rows.size());
   chk_result(0, 2);
   chk_result(1, 1);
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
