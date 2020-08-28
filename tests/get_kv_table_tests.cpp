#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/kv_context.hpp>

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


BOOST_AUTO_TEST_SUITE(get_kv_table_tests)

BOOST_FIXTURE_TEST_CASE( kv_table_make_prefix_test, TESTER ) try {
   chain_apis::read_only plugin(*(this->control), std::optional<chain_apis::account_query_db>(), fc::microseconds::maximum());
   kv_database_config limits;

   vector<unsigned char> prefix;
   vector<unsigned char> expected = {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1};

   auto check = [&](const vector<unsigned char> &expect) {
      bool matched = true;
      for (int i = 0; i < prefix.size(); ++i) {
         if( (unsigned char)prefix[i] != expect[i]) {
            matched = false;
            break;
         }
      }
      return matched;
   };

   eosio::name table(0x01);;
   eosio::name index(0x01);
   BOOST_CHECK_THROW(plugin.make_prefix(table, index, 1, *reinterpret_cast<vector<char> *>(&prefix)), chain::contract_table_query_exception);

   prefix.resize(2 * sizeof(uint64_t) + 1);
   plugin.make_prefix(table, index, 1, *reinterpret_cast<vector<char> *>(&prefix));

   BOOST_TEST(check(expected));

   eosio::name table2("testtable");
   eosio::name index2("primarykey");
   prefix[0] = 0;
   vector<unsigned char> expected2 = {0, 0xca, 0xb1, 0x9c, 0x98, 0xf1, 0x50, 0x00, 0x00, 0xad, 0xdd, 0x23, 0x5f, 0xd0, 0x57, 0x80, 0x00};
   plugin.make_prefix(table2, index2, 0, *reinterpret_cast<vector<char> *>(&prefix));
   BOOST_TEST(check(expected2));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_table_get_next_prefix_test, TESTER)
try
{
   chain_apis::read_only plugin(*(this->control), std::optional<chain_apis::account_query_db>(), fc::microseconds::maximum());
   kv_database_config limits;

   vector<char> empty_prefix;
   auto check = [](const vector<char> &prefix, const vector<unsigned char> &expect) {
      bool matched = true;
      for (int i = 0; i < prefix.size(); ++i) {
         if ((unsigned char)prefix[i] != expect[i]) {
            matched = false;
            break;
         }
      }
      return matched;
   };

   BOOST_CHECK_THROW(plugin.get_next_prefix(*reinterpret_cast<vector<char> *>(&empty_prefix)), chain::contract_table_query_exception);

   vector<unsigned char> prefix1 = {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1};
   vector<unsigned char> expected1 = {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2};

   prefix1.resize(2 * sizeof(uint64_t) + 1);
   auto next_prefix1 = plugin.get_next_prefix(*reinterpret_cast<vector<char> *>(&prefix1));
   BOOST_TEST(check(next_prefix1, expected1));

   vector<unsigned char> prefix2 = {0, 0xca, 0xb1, 0x9c, 0x98, 0xf1, 0x50, 0x00, 0x00, 0xad, 0xdd, 0x23, 0x5f, 0xd0, 0x57, 0x80, 0xff};
   vector<unsigned char> expected2 = {0, 0xca, 0xb1, 0x9c, 0x98, 0xf1, 0x50, 0x00, 0x00, 0xad, 0xdd, 0x23, 0x5f, 0xd0, 0x57, 0x81, 0x00};
   auto next_prefix2 = plugin.get_next_prefix(*reinterpret_cast<vector<char> *>(&prefix2));
   BOOST_TEST(check(next_prefix2, expected2));

   vector<unsigned char> prefix3 = {0, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
   vector<unsigned char> expected3 = {0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   auto next_prefix3 = plugin.get_next_prefix(*reinterpret_cast<vector<char> *>(&prefix3));
   BOOST_TEST(check(next_prefix3, expected3));

   vector<unsigned char> prefix4 = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
   BOOST_CHECK_THROW(plugin.get_next_prefix(*reinterpret_cast<vector<char> *>(&prefix4)), chain::contract_table_query_exception);
}
FC_LOG_AND_RETHROW()




/*
struct test_kv_contest : public kv_context {
   ~kv_context() {}

   int64_t  kv_erase(uint64_t contract, const char* key, uint32_t key_size) { return 0; }
   int64_t  kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                           uint32_t value_size, account_name payer) { return 0; }
   bool     kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) { return true; }
   uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) { return 0; }

   std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) {
       return ;
   }
};
*/



BOOST_FIXTURE_TEST_CASE(get_kv_table_rows_test, TESTER)
try
{
   chain_apis::read_only plugin(*(this->control), std::optional<chain_apis::account_query_db>(), fc::microseconds::maximum());
   kv_database_config limits;

   const char *abi_def_abi = R"=====(
{
 "____comment": "This file was generated with eosio-abigen. DO NOT EDIT ",
 "version": "eosio::abi/1.2",
 "types": [],
 "structs": [
     {
         "name": "mystruct",
         "base": "",
         "fields": [
             {
                 "name": "primarykey",
                 "type": "name"
             },
             {
                 "name": "foo",
                 "type": "string"
             },
             {
                 "name": "bar",
                 "type": "uint64"
             }
         ]
     }
 ],
 "actions": [],
 "tables": [],
 "kv_tables": {
     "testtable1": {
         "type": "mystruct",
         "primary_index": {
             "name": "primarykey",
             "type": "name"
         },
         "secondary_indices": {
             "foo": {
                 "type": "string"
             },
             "bar": {
                 "type": "uint64"
             }
         }
     }
 },
 "ricardian_clauses": [],
 "variants": [],
 "action_results": []
}
)=====";

   auto var = fc::json::from_string(abi_def_abi);
   auto abi = var.as<abi_def>();


   chain_apis::read_only::get_kv_table_rows_params p{
        .json = false,
        .code = name("kvmindices"),
        .table = name("testtable"),
        .index_name = name("primarykey"),
        .encode_type = "bytes",
        .index_value = "pid3",
        .limit = 100,
        .reverse = false,
        .show_payer = false
   };

   chain_apis::read_only::get_kv_table_rows_params range1{
        .json = false,
        .code = name("kvmindices"),
        .table = name("testtable"),
        .index_name = name("foo"),
        .encode_type = "bytes",
        .lower_bound = "foo2",
        .upper_bound = "foo4",
        .limit = 100,
        .reverse = false,
        .show_payer = false
   };

   vector<char> l{0,0,0,0,0,0,0,10};
   vector<char> u{0,0,0,0,0,0,0,20};
   string lb(l.data(), l.size());
   string ub(u.data(), u.size());
   chain_apis::read_only::get_kv_table_rows_params range2{
        .json = false,
        .code = name("kvmindices"),
        .table = name("testtable"),
        .index_name = name("bar"),
        .encode_type = "bytes",
        .lower_bound = lb,
        .upper_bound = ub,
        .limit = 100,
        .reverse = false,
        .show_payer = false
   };


   //auto result = plugin.get_kv_table_rows_context(p);
   //result = plugin.get_kv_table_rows_context(range1);
   //result = plugin.get_kv_table_rows_context(range2);










}
FC_LOG_AND_RETHROW()





BOOST_AUTO_TEST_SUITE_END()
