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

BOOST_AUTO_TEST_SUITE(get_kv_table_tests)

fc::microseconds max_serialization_time = fc::seconds(1);

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
  "testtable": {
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

const char *test_data1 = R"=====(
{
   "type": "mystruct",
   "primarykey": "pid1",
   "foo":  "aaa",
   "bar":  1000
}
)=====";

const char *test_data2 = R"=====(
{
   "type": "mystruct",
   "primarykey": "pid2",
   "foo":  "bbb",
   "bar":  2000
}
)=====";

const char *test_data3 = R"=====(
{
   "type": "mystruct",
   "primarykey": "pid3",
   "foo":  "ccc",
   "bar":  3000
}
)=====";

const char *test_data4 = R"=====(
{
   "type": "mystruct",
   "primarykey": "pid4",
   "foo":  "ddd",
   "bar":  4000
}
)=====";


const char *test_data5 = R"=====(
{
   "type": "mystruct",
   "primarykey": "pid5",
   "foo":  "eee",
   "bar":  5000
}
)=====";

BOOST_FIXTURE_TEST_CASE( kv_table_make_prefix_test, TESTER ) try {
   chain_apis::read_only plugin(*(this->control), std::optional<chain_apis::account_query_db>(), max_serialization_time);
   kv_database_config limits;

   vector<unsigned char> prefix;
   vector<unsigned char> expected = {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1};

   auto check = [&](const vector<unsigned char> &expect) {
      bool matched = true;
      for( int i = 0; i < prefix.size(); ++i ) {
         if( (unsigned char)prefix[i] != expect[i] ) {
            matched = false;
            break;
         }
      }
      return matched;
   };

   eosio::name table(0x01);
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

// Mock kv_iterator and kv_context
struct test_empty_kv_iterator : public kv_iterator {

   ~test_empty_kv_iterator() {}

   bool       is_kv_chainbase_context_iterator() const { return true; }
   bool is_kv_rocksdb_context_iterator() const override { return false; }
   kv_it_stat kv_it_status() { return kv_it_stat::iterator_ok; }
   int32_t    kv_it_compare(const kv_iterator& rhs) { return 0; }
   int32_t    kv_it_key_compare(const char* key, uint32_t size) { return 0; }
   kv_it_stat kv_it_move_to_end() { return kv_it_stat::iterator_ok; }
   kv_it_stat kv_it_next(uint32_t* found_key_size, uint32_t* found_value_size) { return kv_it_stat::iterator_ok; }
   kv_it_stat kv_it_prev(uint32_t* found_key_size, uint32_t* found_value_size) { return kv_it_stat::iterator_ok; }
   kv_it_stat kv_it_lower_bound(const char* key, uint32_t size,
                                       uint32_t* found_key_size, uint32_t* found_value_size) { return kv_it_stat::iterator_ok; }
   kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) { return kv_it_stat::iterator_ok; }
   kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) { return kv_it_stat::iterator_ok; }
};

struct test_empty_kv_context : public kv_context {
   int64_t  kv_erase(uint64_t contract, const char* key, uint32_t key_size) { return 0; }
   int64_t  kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                           uint32_t value_size, account_name payer) { return 0; }
   bool     kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) { return true; }
   uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) { return 0; }

   std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) {
      return std::unique_ptr<kv_iterator>(new test_empty_kv_iterator());
   }
};


struct test_point_query_kv_context : public test_empty_kv_context {
   test_point_query_kv_context(const abi_def &abidef) : abi(abidef), abis() {
      abis.set_abi(abi, abi_serializer::create_yield_function(max_serialization_time));

      auto var = fc::json::from_string(test_data1);
      auto binary = abis.variant_to_binary("testtable", var, abi_serializer::create_yield_function(max_serialization_time));
      binaries.push_back(binary);
      auto var2 = abis.binary_to_variant("testtable", binary, abi_serializer::create_yield_function(max_serialization_time));

      var = fc::json::from_string(test_data2);
      binary = abis.variant_to_binary("testtable", var, abi_serializer::create_yield_function(max_serialization_time));
      binaries.push_back(binary);
   }

    bool kv_get(uint64_t contract, const char *key, uint32_t key_size, uint32_t &value_size) {
       const auto &binary = binaries[current_data];
       value_size = binary.size();
       return true;
    }

   uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) {
      const auto &binary = binaries[current_data];
      memcpy(data, binary.data(), binary.size());
      current_data = (++current_data) % 2;
      return data_size;
   }

   fc::microseconds max_serialization_time = fc::seconds(1); // some test machines are very slow
   std::vector<bytes> binaries;
   int current_data = 0;
   abi_def abi;
   abi_serializer abis;
};

struct test_range_query_kv_context;

struct test_range_query_kv_iterator : public test_empty_kv_iterator {
   test_range_query_kv_iterator( map<string, bytes> &binaries, vector<unsigned char> &prefix, const string &current_key)
      : binaries(binaries), prefix(prefix), current_key(current_key) {
   }

   int32_t kv_it_key_compare(const char* key, uint32_t size) {
      string other_key(key, size);
      if( current_key == other_key) return 0;

      return current_key > other_key ? 1 : -1;
   }

   kv_it_stat kv_it_next(uint32_t* found_key_size, uint32_t* found_value_size) {
      auto next_itr = binaries.upper_bound(current_key);
      if (next_itr == binaries.end())
      {
         return kv_it_stat::iterator_end;
      }
      *found_key_size = next_itr->first.size();
      *found_value_size = next_itr->second.size();
      current_key = next_itr->first;

      return kv_it_stat::iterator_ok;
   }

   kv_it_stat kv_it_prev(uint32_t *found_key_size, uint32_t *found_value_size)
   {
      auto prev_itr = binaries.lower_bound(current_key);
      --prev_itr;
      *found_key_size = prev_itr->first.size();
      *found_value_size = prev_itr->second.size();
      current_key = prev_itr->first;

      return kv_it_stat::iterator_ok;
   }

   kv_it_stat kv_it_lower_bound(const char* key, uint32_t size, uint32_t* found_key_size, uint32_t* found_value_size) {
      string skey(key, size);
      auto lower_itr = binaries.lower_bound(skey);
      if( lower_itr == binaries.end() ) {
         return kv_it_stat::iterator_end;
      }
      *found_key_size = lower_itr->first.size();
      *found_value_size = lower_itr->second.size();
      current_key = lower_itr->first;

      return kv_it_stat::iterator_ok;
   }

   kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) {
      actual_size = current_key.size();
      memcpy(dest, current_key.data(), actual_size);

      return kv_it_stat::iterator_ok;
   }

   kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) {
      auto binary = binaries[current_key];
      actual_size = binary.size();
      memcpy(dest, binary.data(), actual_size);

      return kv_it_stat::iterator_ok;
   }

   map<string, bytes> &binaries;
   vector<unsigned char> &prefix;
   string current_key;
};


struct test_range_query_kv_context : public test_empty_kv_context {
   test_range_query_kv_context(const abi_def &abidef) : abi(abidef), abis() {
      abis.set_abi(abi, abi_serializer::create_yield_function(max_serialization_time));

      vector<string> test_data_vec{test_data1, test_data2, test_data3, test_data4, test_data5};
      vector<vector<unsigned char>> prefixes{prefix1, prefix2, prefix2};

      for( auto &prefix : prefixes ) {
         size_t prefix_size = prefix.size();
         vector<unsigned char> key;
         key.resize(prefix_size + 6); // index value "pidN\0\0" is appended where N is [1-5]
         for( int i = 0; i < prefix_size; ++i ) {
            key[i] = static_cast<unsigned char>(prefix[i]);
         }
         key[prefix_size] = static_cast<unsigned char>('p');
         key[prefix_size + 1] = static_cast<unsigned char>('i');
         key[prefix_size + 2] = static_cast<unsigned char>('d');
         key[prefix_size + 4] = 0;
         key[prefix_size + 5] = 0;


         for( int i = 0; i < 5; ++i ) {
            auto &test_data = test_data_vec[i];
            auto var = fc::json::from_string(test_data);
            auto binary = abis.variant_to_binary("testtable", var, abi_serializer::create_yield_function(max_serialization_time));
            key[prefix_size + 3] = static_cast<unsigned char>('1' + i);
            string skey1(reinterpret_cast<char *>(key.data()), key.size());
            binaries[skey1] = binary;
         }
      }
   }

   bool kv_get(uint64_t contract, const char *key, uint32_t key_size, uint32_t &value_size) {
      string skey(key, key_size);
      current_key = skey;
      const auto &binary = binaries[skey];
      value_size = binary.size();
      return true;
   }

   uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) {
      const auto &binary = binaries[current_key];
      memcpy(data, binary.data(), binary.size());
      return data_size;
   }

   std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) {
      return std::unique_ptr<kv_iterator>(new test_range_query_kv_iterator(binaries, this->prefix2, current_key));
   }

   fc::microseconds max_serialization_time = fc::seconds(1); // some test machines are very slow
   map<string, bytes> binaries;
   vector<unsigned char> prefix1{0x00, 0xca, 0xb1, 0x9c, 0x98, 0xf1, 0x50, 0x00, 0x00, 0xad, 0xdd, 0x23, 0x5f, 0xd0, 0x57, 0x80, 0x00};
   vector<unsigned char> prefix2{0x01, 0xca, 0xb1, 0x9c, 0x98, 0xf1, 0x50, 0x00, 0x00, 0xad, 0xdd, 0x23, 0x5f, 0xd0, 0x57, 0x80, 0x00};
   vector<unsigned char> prefix3{0x01, 0xca, 0xb1, 0x9c, 0x98, 0xf1, 0x50, 0x00, 0x00, 0xad, 0xdd, 0x23, 0x5f, 0xd0, 0x57, 0x80, 0x01};

   string current_key;
   abi_def abi;
   abi_serializer abis;

   friend struct test_range_query_kv_iterator;

};

BOOST_FIXTURE_TEST_CASE(get_kv_table_rows_point_query_test, TESTER) try {
   chain_apis::read_only plugin(*(this->control), std::optional<chain_apis::account_query_db>(), fc::microseconds::maximum());
   kv_database_config limits;

   auto var = fc::json::from_string(abi_def_abi);
   auto abi = var.as<abi_def>();

   chain_apis::read_only::get_kv_table_rows_params p{
      .json = true,
      .code = name("kvmindices"),
      .table = name("testtable"),
      .index_name = name("primarykey"),
      .encode_type = "string",
      .index_value = "pid1",
      .limit = 100,
      .reverse = false,
      .show_payer = false
   };

   // mock kv_context
   test_point_query_kv_context context(abi);

   auto result = plugin.get_kv_table_rows_context(p, context, abi);
   BOOST_TEST(result.rows[0]["primarykey"] == "pid1");
   BOOST_TEST(result.rows[0]["foo"] == "aaa");
   BOOST_TEST(result.rows[0]["bar"] == 1000);

   p.table = name("wrongtable");
   BOOST_CHECK_THROW( plugin.get_kv_table_rows_context(p, context, abi), fc::exception );

   p.index_name = name("wrongindex");
   BOOST_CHECK_THROW( plugin.get_kv_table_rows_context(p, context, abi), fc::exception );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(get_kv_table_rows_range_query_test, TESTER) try {
   chain_apis::read_only plugin(*(this->control), std::optional<chain_apis::account_query_db>(), fc::microseconds::maximum());
   kv_database_config limits;

   auto var = fc::json::from_string(abi_def_abi);
   auto abi = var.as<abi_def>();
   abi_serializer abis;
   abis.set_abi(abi, abi_serializer::create_yield_function(abi_serializer_max_time));

   test_range_query_kv_context context(abi);

   // missing index_value, lower_bound, upper_bound
   chain_apis::read_only::get_kv_table_rows_params range0{
       .json = true,
       .code = name("kvmindices"),
       .table = name("testtable"),
       .index_name = name("primarykey"),
       .encode_type = "string",
       .limit = 100,
       .reverse = false,
       .show_payer = false
   };
   BOOST_CHECK_THROW(plugin.get_kv_table_rows_context(range0, context, abi), fc::exception);

   chain_apis::read_only::get_kv_table_rows_params range1{
        .json = true,
        .code = name("kvmindices"),
        .table = name("testtable"),
        .index_name = name("primarykey"),
        .encode_type = "string",
        .index_value = "",
        .lower_bound = "pid2",
        .upper_bound = "pid4",
        .limit = 100,
        .reverse = false,
        .show_payer = false
   };

   auto result = plugin.get_kv_table_rows_context(range1, context, abi);
   BOOST_TEST(result.rows.size() == 2);
   BOOST_TEST(result.rows[0]["primarykey"] == "pid2");
   BOOST_TEST(result.rows[0]["foo"] == "bbb");
   BOOST_TEST(result.rows[0]["bar"] == 2000);
   BOOST_TEST(result.rows[1]["primarykey"] == "pid3");
   BOOST_TEST(result.rows[1]["foo"] == "ccc");
   BOOST_TEST(result.rows[1]["bar"] == 3000);

   range1.json = false;
   result = plugin.get_kv_table_rows_context(range1, context, abi);
   vector<char> bytes;
   from_variant(result.rows[0], bytes);
   auto bin_var = abis.binary_to_variant("testtable", bytes, abi_serializer::create_yield_function(max_serialization_time));
   BOOST_TEST(bin_var["primarykey"] == "pid2");
   BOOST_TEST(bin_var["foo"] == "bbb");
   BOOST_TEST(bin_var["bar"] == 2000);
   from_variant(result.rows[1], bytes);
   bin_var = abis.binary_to_variant("testtable", bytes, abi_serializer::create_yield_function(max_serialization_time));
   BOOST_TEST(bin_var["primarykey"] == "pid3");
   BOOST_TEST(bin_var["foo"] == "ccc");
   BOOST_TEST(bin_var["bar"] == 3000);

   range1.json = true;
   range1.reverse = true;
   result = plugin.get_kv_table_rows_context(range1, context, abi);
   BOOST_TEST(result.rows.size() == 2);
   BOOST_TEST(result.rows[0]["primarykey"] == "pid4");
   BOOST_TEST(result.rows[0]["foo"] == "ddd");
   BOOST_TEST(result.rows[0]["bar"] == 4000);
   BOOST_TEST(result.rows[1]["primarykey"] == "pid3");
   BOOST_TEST(result.rows[1]["foo"] == "ccc");
   BOOST_TEST(result.rows[1]["bar"] == 3000);

   range1.json = false;
   result = plugin.get_kv_table_rows_context(range1, context, abi);
   BOOST_TEST(result.rows.size() == 2);
   from_variant(result.rows[0], bytes);
   bin_var = abis.binary_to_variant("testtable", bytes, abi_serializer::create_yield_function(max_serialization_time));
   BOOST_TEST(bin_var["primarykey"] == "pid4");
   BOOST_TEST(bin_var["foo"] == "ddd");
   BOOST_TEST(bin_var["bar"] == 4000);
   from_variant(result.rows[1], bytes);
   bin_var = abis.binary_to_variant("testtable", bytes, abi_serializer::create_yield_function(max_serialization_time));
   BOOST_TEST(bin_var["primarykey"] == "pid3");
   BOOST_TEST(bin_var["foo"] == "ccc");
   BOOST_TEST(bin_var["bar"] == 3000);

   chain_apis::read_only::get_kv_table_rows_params range2{
      .json = true,
      .code = name("kvmindices"),
      .table = name("testtable"),
      .index_name = name("primarykey"),
      .encode_type = "string",
      .index_value = "",
      .lower_bound = "pid1",
      .upper_bound = "pid5",
      .limit = 100,
      .reverse = true,
      .show_payer = false
   };
   result = plugin.get_kv_table_rows_context(range2, context, abi);
   BOOST_TEST(result.rows.size() == 4);
   BOOST_TEST(result.rows.size() == 4);
   BOOST_TEST(result.rows[0]["primarykey"] == "pid5");
   BOOST_TEST(result.rows[1]["primarykey"] == "pid4");
   BOOST_TEST(result.rows[2]["primarykey"] == "pid3");
   BOOST_TEST(result.rows[3]["primarykey"] == "pid2");

   chain_apis::read_only::get_kv_table_rows_params range3{
      .json = true,
      .code = name("kvmindices"),
      .table = name("testtable"),
      .index_name = name("primarykey"),
      .encode_type = "string",
      .lower_bound = "pid1",
      .upper_bound = "pid2",
      .limit = 100,
      .reverse = false,
      .show_payer = false
   };
   result = plugin.get_kv_table_rows_context(range3, context, abi);
   BOOST_TEST(result.rows.size() == 1);

   chain_apis::read_only::get_kv_table_rows_params range4{
      .json = true,
      .code = name("kvmindices"),
      .table = name("testtable"),
      .index_name = name("primarykey"),
      .encode_type = "string",
      .lower_bound = "pid5",
      .upper_bound = "pid5",
      .limit = 100,
      .reverse = false,
      .show_payer = false};
   result = plugin.get_kv_table_rows_context(range4, context, abi);
   BOOST_TEST(result.rows.size() == 0);

   chain_apis::read_only::get_kv_table_rows_params range5{
      .json = true,
      .code = name("kvmindices"),
      .table = name("testtable"),
      .index_name = name("primarykey"),
      .encode_type = "string",
      .lower_bound = "pid1",
      .upper_bound = "pid5",
      .limit = 2,
      .reverse = false,
      .show_payer = false
   };
   result = plugin.get_kv_table_rows_context(range5, context, abi);
   BOOST_TEST(result.rows.size() == 2);
   range5.lower_bound = "pid3";
   result = plugin.get_kv_table_rows_context(range5, context, abi);
   BOOST_TEST(result.rows.size() == 2);
   BOOST_TEST(result.more == false);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
