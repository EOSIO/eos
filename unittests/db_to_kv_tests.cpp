#include <eosio/testing/tester.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/db_key_value_iter_store.hpp>

#include <boost/test/unit_test.hpp>
#include <cstring>

#include <boost/algorithm/string/predicate.hpp> //REMOVE

using key_type = eosio::chain::backing_store::db_key_value_format::key_type;
using name = eosio::chain::name;

BOOST_AUTO_TEST_SUITE(db_to_kv_tests)

constexpr uint64_t overhead_size_without_type = eosio::chain::backing_store::db_key_value_format::prefix_size; // 8 (scope) + 8 (table)
constexpr uint64_t overhead_size = overhead_size_without_type + 1; // ... + 1 (key type)

float128_t to_softfloat128( double d ) {
   return f64_to_f128(to_softfloat64(d));
}

double from_softfloat128( float128_t d ) {
   return from_softfloat64(f128_to_f64(d));
}

constexpr eosio::chain::uint128_t create_uint128(uint64_t upper, uint64_t lower) {
   return (eosio::chain::uint128_t(upper) << 64) + lower;
}

constexpr eosio::chain::key256_t create_key256(eosio::chain::uint128_t upper, eosio::chain::uint128_t lower) {
   return { upper, lower };
}

template <typename T>
struct helper;

template<>
struct helper<uint64_t> {
   using type_t = uint64_t;
   using alt_type_t = eosio::chain::uint128_t;
   const type_t init = 0x0;
   type_t inc(type_t t) { return ++t; }
};

template<>
struct helper<eosio::chain::uint128_t> {
   using type_t = eosio::chain::uint128_t;
   using alt_type_t = eosio::chain::key256_t;
   const type_t init = create_uint128(0, 0);
   type_t inc(type_t t) { return ++t; }
};

template<>
struct helper<eosio::chain::key256_t> {
   using type_t = eosio::chain::key256_t;
   using alt_type_t = float64_t;
   const type_t init =  create_key256(create_uint128(0, 0), create_uint128(0, 0));
   type_t inc(type_t t) {
      if(!++t[1]) {
         ++t[0];
      }
      return t;
   }
};

template<>
struct helper<float64_t> {
   using type_t = float64_t;
   using alt_type_t = float128_t;
   const type_t init =  to_softfloat64(0.0);
   type_t inc(type_t t) { return to_softfloat64(from_softfloat64(t) + 1.0); }
};

template<>
struct helper<float128_t> {
   using type_t = float128_t;
   using alt_type_t = uint64_t;
   const type_t init =  to_softfloat128(0.0);
   type_t inc(type_t t) { return to_softfloat128(from_softfloat128(t) + 1.0); }
};

template<typename Type>
void verify_secondary_wont_convert(const b1::chain_kv::bytes& composite_key, const name exp_scope, const name exp_table) {
   name scope;
   name table;
   Type key = helper<Type>().init;
   const Type exp_key = key;
   uint64_t primary_key = 0;
   BOOST_CHECK(!eosio::chain::backing_store::db_key_value_format::get_secondary_key(composite_key, scope, table, key, primary_key));
   BOOST_CHECK_EQUAL(exp_scope.to_string(), scope.to_string());
   BOOST_CHECK_EQUAL(exp_table.to_string(), table.to_string());
   BOOST_CHECK(exp_key == key);
}

void verify_other_gets(const b1::chain_kv::bytes& composite_key, key_type except, const name exp_scope, const name exp_table) {
   if (except != key_type::primary) {
      name scope;
      name table;
      uint64_t key = 0;
      BOOST_CHECK(!eosio::chain::backing_store::db_key_value_format::get_primary_key(composite_key, scope, table, key));
      BOOST_CHECK_EQUAL(exp_scope.to_string(), scope.to_string());
      BOOST_CHECK_EQUAL(exp_table.to_string(), table.to_string());
      BOOST_CHECK(0 == key);
   }
   if (except != key_type::sec_i64) { verify_secondary_wont_convert<uint64_t>(composite_key, exp_scope, exp_table); }
   if (except != key_type::sec_i128) { verify_secondary_wont_convert<eosio::chain::uint128_t>(composite_key, exp_scope, exp_table); }
   if (except != key_type::sec_i256) { verify_secondary_wont_convert<eosio::chain::key256_t>(composite_key, exp_scope, exp_table); }
   if (except != key_type::sec_double) { verify_secondary_wont_convert<float64_t>(composite_key, exp_scope, exp_table); }
   if (except != key_type::sec_long_double) { verify_secondary_wont_convert<float128_t>(composite_key, exp_scope, exp_table); }
   if (except != key_type::table) {
      name scope;
      name table;
      BOOST_CHECK(!eosio::chain::backing_store::db_key_value_format::get_table_key(composite_key, scope, table));
      BOOST_CHECK_EQUAL(exp_scope.to_string(), scope.to_string());
      BOOST_CHECK_EQUAL(exp_table.to_string(), table.to_string());
   }
   name scope;
   name table;
   BOOST_CHECK_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_prefix_key(composite_key, scope, table));
   BOOST_CHECK_EQUAL(exp_scope.to_string(), scope.to_string());
   BOOST_CHECK_EQUAL(exp_table.to_string(), table.to_string());
}

template<typename CharCont>
std::pair<int, unsigned int> compare_composite_keys(const CharCont& lhs_cont, const CharCont& rhs_cont, bool print = false) {
   unsigned int j = 0;
   const int gt = 1;
   const int lt = -1;
   const int equal = 0;
   const char* lhs = lhs_cont.data();
   const char* rhs = rhs_cont.data();
   const auto lhs_size = lhs_cont.size();
   const auto rhs_size = rhs_cont.size();
   for (; j < lhs_size; ++j) {
      if (j >= rhs_size) {
         if (print) ilog("${j}: lhs longer than rhs", ("j", j));
         return { gt, j };
      }
      const auto left = uint64_t(static_cast<unsigned char>(lhs[j]));
      const auto right = uint64_t(static_cast<unsigned char>(rhs[j]));
      if (print) ilog("${j}: ${l} ${sym} ${r}", ("j", j)("l", left)("sym",(left == right ? "==" : "!="))("r", right));
      if (left != right) {
         return { left < right ? lt : gt, j };
      }
   }
   if (rhs_size > lhs_size) {
      if (print) ilog("rhs longer (${r}) than lhs (${l})", ("r", rhs_size)("l", lhs_size));
      return { lt, j };
   }

   return { equal, j };
}

std::pair<int, unsigned int> compare_composite_keys(const std::vector<b1::chain_kv::bytes>& keys, uint64_t lhs_index) {
   return compare_composite_keys(keys[lhs_index], keys[lhs_index + 1]);
}

void verify_ascending_composite_keys(const std::vector<b1::chain_kv::bytes>& keys, uint64_t lhs_index, std::string desc) {
   const auto ret = compare_composite_keys(keys, lhs_index);
   BOOST_CHECK_MESSAGE(ret.first != 1, "expected " + std::to_string(lhs_index) +
                       "th composite key to be less than the " + std::to_string(lhs_index + 1) + "th, but the " + std::to_string(ret.second) +
                       "th character is actually greater on the left than the right.  Called from: " + desc);
   BOOST_CHECK_MESSAGE(ret.first != 0, "expected " + std::to_string(lhs_index) +
                       "th composite key to be less than the " + std::to_string(lhs_index + 1) +
                       "th, but they were identical.  Called from: " + desc);
}

template<typename T, typename KeyFunc>
void verify_scope_table_order(T low_key, T high_key, KeyFunc key_func, bool skip_trailing_prim_key) {
   std::vector<b1::chain_kv::bytes> comp_keys;
   comp_keys.push_back(key_func(name{0}, name{0}, low_key, 0x0));
   // this function always passes the extra primary key, but need to skip what would be redundant keys for when we are
   // actually getting a primary key
   if (!skip_trailing_prim_key) {
      comp_keys.push_back(key_func(name{0}, name{0}, low_key, 0x1));
      comp_keys.push_back(key_func(name{0}, name{0}, low_key, 0xffff'ffff'ffff'ffff));
   }
   comp_keys.push_back(key_func(name{1}, name{0}, low_key, 0x0));
   comp_keys.push_back(key_func(name{1}, name{1}, low_key, 0x0));
   comp_keys.push_back(key_func(name{1}, name{1}, high_key, 0x0));
   if (!skip_trailing_prim_key) {
      comp_keys.push_back(key_func(name{1}, name{1}, high_key, 0x1));
   }
   comp_keys.push_back(key_func(name{1}, name{2}, low_key, 0x0));
   comp_keys.push_back(key_func(name{1}, name{0xffff'ffff'ffff'ffff}, low_key, 0x0));
   comp_keys.push_back(key_func(name{0xffff'ffff'ffff'ffff}, name{0}, low_key, 0x0));
   comp_keys.push_back(key_func(name{0xffff'ffff'ffff'ffff}, name{0xffff'ffff'ffff'ffff}, low_key, 0x0));
   for (unsigned int i = 0; i < comp_keys.size() - 1; ++i) {
      verify_ascending_composite_keys(comp_keys, i, "verify_scope_table_order");
   }
}

// using this function to allow check_ordered_keys to work for all key types
b1::chain_kv::bytes prim_drop_extra_key(name scope, name table, uint64_t db_key, uint64_t unused_primary_key) {
   return eosio::chain::backing_store::db_key_value_format::create_primary_key(scope, table, db_key);
};

template<typename T, typename KeyFunc>
void check_ordered_keys(const std::vector<T>& ordered_keys, KeyFunc key_func, bool skip_trailing_prim_key = false) {
   verify_scope_table_order(ordered_keys[0], ordered_keys[1], key_func, skip_trailing_prim_key);
   name scope{0};
   name table{0};
   const uint64_t prim_key = 0;
   std::vector<b1::chain_kv::bytes> composite_keys;
   for (const auto& key: ordered_keys) {
      auto comp_key = key_func(scope, table, key, prim_key);
      composite_keys.push_back(comp_key);
   }
   for (unsigned int i = 0; i < composite_keys.size() - 1; ++i) {
      verify_ascending_composite_keys(composite_keys, i, "check_ordered_keys");
   }
}

void verify_secondary_key_pair_order(const eosio::chain::backing_store::db_key_value_format::secondary_key_pair& incoming_key_pair, const key_type kt) {
   using slice_array = std::array<rocksdb::Slice, 4>;
   const slice_array incoming {
         eosio::chain::backing_store::db_key_value_format::prefix_slice(incoming_key_pair.primary_to_secondary_key),
         eosio::chain::backing_store::db_key_value_format::prefix_type_slice(incoming_key_pair.primary_to_secondary_key),
         eosio::chain::backing_store::db_key_value_format::prefix_primary_to_secondary_slice(incoming_key_pair.primary_to_secondary_key, false),
         eosio::chain::backing_store::db_key_value_format::prefix_primary_to_secondary_slice(incoming_key_pair.primary_to_secondary_key, true)
   };

   // to make this test less error prone, extract the scope and table ...
   name scope;
   name table;
   uint64_t primary_key = 0x0;
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_prefix_key(incoming_key_pair.secondary_key, scope, table));

   // ... and the primary key
   const rocksdb::Slice full_key { incoming_key_pair.secondary_key.data(), incoming_key_pair.secondary_key.size() };
   const rocksdb::Slice all_but_primary_key { incoming_key_pair.secondary_key.data(), incoming_key_pair.secondary_key.size() - sizeof(primary_key) };
   BOOST_REQUIRE(eosio::chain::backing_store::db_key_value_format::get_trailing_primary_key(full_key, all_but_primary_key, primary_key));

   rocksdb::Slice pair_sec_key_prefix;
   rocksdb::Slice pair_sec_key_prefix_type;
   rocksdb::Slice pair_sec_key_all_but_secondary_prefix;

   bool type_seen = false; // keep track so we know if the type we are currently checking is after or before the incoming type
   unsigned int other_types_processed = 0;
   unsigned int expected_type_comp = 0;
   key_type current_kt = key_type::primary;
   // lambda to correctly identify comparisons for each type
   auto type_check = [&type_seen, &other_types_processed, &current_kt, &expected_type_comp, kt](key_type curr_kt) {
      current_kt = curr_kt;
      if (current_kt != kt) {
         ++other_types_processed;
         expected_type_comp = type_seen ? -1 : 1;
      } else {
         expected_type_comp = 0;
         BOOST_REQUIRE(!type_seen);
         type_seen = true;
      }
   };

   auto desc = [&kt, &current_kt](int comp) -> std::string {
      std::string msg = "incoming type: " + eosio::chain::backing_store::db_key_value_format::detail::to_string(kt);
      msg += ", comparing to type: " + eosio::chain::backing_store::db_key_value_format::detail::to_string(current_kt);
      msg += ", expected incoming to be ";
      if (comp > 0) {
         msg += "greater than";
      }
      else if (comp < 0) {
         msg += "less than";
      }
      else {
         msg += "equal to";
      }
      return msg;
   };

   auto split_keys = [](const b1::chain_kv::bytes& composite) -> slice_array {
      return {
         eosio::chain::backing_store::db_key_value_format::prefix_slice(composite),
         eosio::chain::backing_store::db_key_value_format::prefix_type_slice(composite),
         eosio::chain::backing_store::db_key_value_format::prefix_primary_to_secondary_slice(composite, false),
         eosio::chain::backing_store::db_key_value_format::prefix_primary_to_secondary_slice(composite, true)
      };
   };

   auto compare_keys = [&incoming, &expected_type_comp, &desc](const slice_array& values) {
      BOOST_CHECK_MESSAGE(0 == compare_composite_keys(incoming[0], values[0]).first, "table and scope prefix doesn't match: " + desc(0));
      BOOST_CHECK_MESSAGE(0 == compare_composite_keys(incoming[1], values[1]).first, "type prefix doesn't match: " + desc(0));
      BOOST_CHECK_MESSAGE(expected_type_comp == compare_composite_keys(incoming[2], values[2]).first, "secondary type prefix comparison failed expectations: " + desc(expected_type_comp));
      BOOST_CHECK_MESSAGE(expected_type_comp == compare_composite_keys(incoming[3], values[3]).first, "primary key prefix comparison failed expectations: " + desc(expected_type_comp));
      static_assert(std::tuple_size<slice_array>::value == 4); // ensure we don't miss comparing values
   };

   const auto primary_key2 = primary_key + 1;
   BOOST_REQUIRE_LT(primary_key, primary_key2);
   BOOST_REQUIRE_EQUAL(primary_key + 1, primary_key2);

   type_check(key_type::sec_i64);
   auto sec_key_pair = eosio::chain::backing_store::db_key_value_format::create_secondary_key_pair(scope, table, helper<uint64_t>().init, primary_key);
   auto values = split_keys(sec_key_pair.primary_to_secondary_key);
   compare_keys(values);
   auto prim_to_sec_key = eosio::chain::backing_store::db_key_value_format::create_primary_to_secondary_key(scope, table, primary_key2, helper<uint64_t>().init);
   auto values2 = split_keys(prim_to_sec_key);
   // ensure that the primary_to_secondary prefix for a specific type matches
   BOOST_CHECK_EQUAL(0, compare_composite_keys(values[2], values2[2]).first);
   // ensure that their ordered by primary_key
   BOOST_REQUIRE_EQUAL(-1, compare_composite_keys(values[3], values2[3]).first);

   type_check(key_type::sec_i128);
   sec_key_pair = eosio::chain::backing_store::db_key_value_format::create_secondary_key_pair(scope, table, helper<eosio::chain::uint128_t>().init, primary_key);
   values = split_keys(sec_key_pair.primary_to_secondary_key);
   compare_keys(values);
   prim_to_sec_key = eosio::chain::backing_store::db_key_value_format::create_primary_to_secondary_key(scope, table, primary_key2, helper<eosio::chain::uint128_t>().init);
   values2 = split_keys(prim_to_sec_key);
   // ensure that the primary_to_secondary prefix for a specific type matches
   BOOST_CHECK_EQUAL(0, compare_composite_keys(values[2], values2[2]).first);
   // ensure that their ordered by primary_key
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(sec_key_pair.primary_to_secondary_key, prim_to_sec_key).first);
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(values[3], values2[3]).first);

   type_check(key_type::sec_i256);
   sec_key_pair = eosio::chain::backing_store::db_key_value_format::create_secondary_key_pair(scope, table, helper<eosio::chain::key256_t>().init, primary_key);
   values = split_keys(sec_key_pair.primary_to_secondary_key);
   compare_keys(values);
   prim_to_sec_key = eosio::chain::backing_store::db_key_value_format::create_primary_to_secondary_key(scope, table, primary_key2, helper<eosio::chain::key256_t>().init);
   values2 = split_keys(prim_to_sec_key);
   // ensure that the primary_to_secondary prefix for a specific type matches
   BOOST_CHECK_EQUAL(0, compare_composite_keys(values[2], values2[2]).first);
   // ensure that their ordered by primary_key
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(values[3], values2[3]).first);

   type_check(key_type::sec_double);
   sec_key_pair = eosio::chain::backing_store::db_key_value_format::create_secondary_key_pair(scope, table, helper<float64_t>().init, primary_key);
   values = split_keys(sec_key_pair.primary_to_secondary_key);
   compare_keys(values);
   prim_to_sec_key = eosio::chain::backing_store::db_key_value_format::create_primary_to_secondary_key(scope, table, primary_key2, helper<float64_t>().init);
   values2 = split_keys(prim_to_sec_key);
   // ensure that the primary_to_secondary prefix for a specific type matches
   BOOST_CHECK_EQUAL(0, compare_composite_keys(values[2], values2[2]).first);
   // ensure that their ordered by primary_key
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(values[3], values2[3]).first);

   type_check(key_type::sec_long_double);
   sec_key_pair = eosio::chain::backing_store::db_key_value_format::create_secondary_key_pair(scope, table, helper<float128_t>().init, primary_key);
   values = split_keys(sec_key_pair.primary_to_secondary_key);
   compare_keys(values);
   prim_to_sec_key = eosio::chain::backing_store::db_key_value_format::create_primary_to_secondary_key(scope, table, primary_key2, helper<float128_t>().init);
   values2 = split_keys(prim_to_sec_key);
   // ensure that the primary_to_secondary prefix for a specific type matches
   BOOST_CHECK_EQUAL(0, compare_composite_keys(values[2], values2[2]).first);
   // ensure that their ordered by primary_key
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(values[3], values2[3]).first);

   BOOST_REQUIRE_EQUAL(4, other_types_processed); // make sure that if a new secondary key is added that this will fail
   BOOST_REQUIRE(type_seen); // make sure that if a new secondary key is added that this will fail
}

template<typename Key>
void verify_primary_to_sec_key(const eosio::chain::backing_store::db_key_value_format::secondary_key_pair& key_pair, name scope, name table, uint64_t primary_key, Key sec_key) {
   name extracted_scope;
   name extracted_table;
   const uint64_t default_primary_key = 0xffff'ffff'ffff'ffff;
   BOOST_REQUIRE_NE(default_primary_key, primary_key);
   uint64_t extracted_primary_key = default_primary_key;
   Key extracted_key;

   const key_type actual_kt = eosio::chain::backing_store::db_key_value_format::extract_key_type(key_pair.primary_to_secondary_key);
   BOOST_REQUIRE(key_type::primary_to_sec == actual_kt);
   const key_type actual_sec_kt = eosio::chain::backing_store::db_key_value_format::extract_primary_to_sec_key_type(key_pair.primary_to_secondary_key);
   const key_type expected_sec_kt = eosio::chain::backing_store::db_key_value_format::derive_secondary_key_type<Key>();
   BOOST_REQUIRE(expected_sec_kt == actual_sec_kt);

   b1::chain_kv::bytes::const_iterator composite_loc;
   key_type kt = key_type::table;
   std::tie(extracted_scope, extracted_table, composite_loc, kt) = eosio::chain::backing_store::db_key_value_format::get_prefix_thru_key_type(key_pair.primary_to_secondary_key);
   BOOST_CHECK_EQUAL(scope.to_string(), extracted_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), extracted_table.to_string());
   // make sure that there is a primary key and secondary key's worth of memory before reaching the end of the key
   BOOST_CHECK_EQUAL(sizeof(uint64_t) + sizeof(Key) + 1, std::distance(composite_loc, key_pair.primary_to_secondary_key.cend()));
   BOOST_CHECK(key_type::primary_to_sec == kt);

   extracted_scope = name{};
   extracted_table = name{};
   // use this primary_to_sec_key to retrieve the correct type
   auto result = eosio::chain::backing_store::db_key_value_format::get_primary_to_secondary_keys(
         key_pair.primary_to_secondary_key, extracted_scope, extracted_table, extracted_primary_key, extracted_key);
   BOOST_REQUIRE(eosio::chain::backing_store::db_key_value_format::prim_to_sec_type_result::valid_type == result);
   BOOST_CHECK_EQUAL(scope.to_string(), extracted_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), extracted_table.to_string());
   BOOST_CHECK_EQUAL(primary_key, extracted_primary_key);
   BOOST_CHECK(sec_key == extracted_key);

   // used this primary_to_sec_key to try and retrieve another secondary type
   extracted_scope = name{};
   extracted_table = name{};
   extracted_primary_key = default_primary_key;
   using alt_type_t = typename helper<Key>::alt_type_t;
   alt_type_t alt_key = helper<alt_type_t>{}.init;
   result = eosio::chain::backing_store::db_key_value_format::get_primary_to_secondary_keys(
         key_pair.primary_to_secondary_key, extracted_scope, extracted_table, primary_key, alt_key);
   BOOST_REQUIRE(eosio::chain::backing_store::db_key_value_format::prim_to_sec_type_result::wrong_sec_type == result);
   BOOST_CHECK_EQUAL(scope.to_string(), extracted_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), extracted_table.to_string());
   BOOST_CHECK_EQUAL(default_primary_key, extracted_primary_key);
   BOOST_CHECK(helper<alt_type_t>{}.init == alt_key);

   extracted_scope = name{};
   extracted_table = name{};
   extracted_primary_key = default_primary_key;
   extracted_key = helper<Key>{}.init;
   Key invalid_key;
   result = eosio::chain::backing_store::db_key_value_format::get_primary_to_secondary_keys(
      key_pair.secondary_key, extracted_scope, extracted_table, primary_key, alt_key);
   BOOST_REQUIRE(eosio::chain::backing_store::db_key_value_format::prim_to_sec_type_result::invalid_type == result);
   BOOST_CHECK_EQUAL(scope.to_string(), extracted_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), extracted_table.to_string());
   BOOST_CHECK_EQUAL(default_primary_key, extracted_primary_key);
   BOOST_CHECK(helper<Key>{}.init == extracted_key);
}

template<typename Key>
void verify_secondary_key_pair(const b1::chain_kv::bytes& composite_key) {
   name scope;
   name table;
   Key sec_key;
   uint64_t primary_key;
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_secondary_key(composite_key, scope, table, sec_key, primary_key));
   Key sec_key2= helper<Key>{}.init;
   uint64_t primary_key2 = 0;
   const auto type_prefix = eosio::chain::backing_store::db_key_value_format::prefix_type_slice(composite_key);
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_trailing_sec_prim_keys(composite_key, type_prefix, sec_key2, primary_key2));
   BOOST_CHECK(sec_key == sec_key2);
   BOOST_CHECK_EQUAL(primary_key, primary_key2);

   const auto sec_key_pair = eosio::chain::backing_store::db_key_value_format::create_secondary_key_pair(scope, table, sec_key, primary_key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(sec_key) + sizeof(primary_key), sec_key_pair.secondary_key.size());
   BOOST_CHECK_EQUAL(0, compare_composite_keys(composite_key, sec_key_pair.secondary_key).first);
   BOOST_CHECK_EQUAL(overhead_size + sizeof(primary_key) + sizeof(key_type) + sizeof(sec_key), sec_key_pair.primary_to_secondary_key.size());
   const auto primary_to_sec_key = eosio::chain::backing_store::db_key_value_format::create_primary_to_secondary_key(scope, table, primary_key, sec_key);
   BOOST_CHECK_EQUAL(overhead_size + sizeof(primary_key) + sizeof(key_type) + sizeof(sec_key), primary_to_sec_key.size());
   BOOST_CHECK_EQUAL(0, compare_composite_keys(sec_key_pair.primary_to_secondary_key, primary_to_sec_key).first);
   const key_type actual_kt = eosio::chain::backing_store::db_key_value_format::extract_key_type(sec_key_pair.secondary_key);
   BOOST_CHECK(eosio::chain::backing_store::db_key_value_format::derive_secondary_key_type<Key>() == actual_kt);


   // extract using the type prefix, the extended type prefix (with the indication of the secondary type), and from the prefix including the primary type
   uint64_t extracted_primary_key2 = 0xffff'ffff'ffff'ffff;
   // since this method derives primary_key, ensure we don't use a value in here that means we are not really checking that the key is extracted
   BOOST_REQUIRE_NE(extracted_primary_key2, primary_key);
   Key extracted_key2;
   auto primary_to_sec_prefix = eosio::chain::backing_store::db_key_value_format::prefix_type_slice(primary_to_sec_key);
   auto result = eosio::chain::backing_store::db_key_value_format::get_trailing_prim_sec_keys(
         primary_to_sec_key, primary_to_sec_prefix, extracted_primary_key2, extracted_key2);
   BOOST_REQUIRE(result);
   BOOST_CHECK_EQUAL(primary_key, extracted_primary_key2);
   BOOST_CHECK(sec_key == extracted_key2);

   verify_primary_to_sec_key(sec_key_pair, scope, table, primary_key, sec_key);

   uint64_t extracted_primary_key3 = 0xffff'ffff'ffff'ffff;
   Key extracted_key3;
   primary_to_sec_prefix = eosio::chain::backing_store::db_key_value_format::prefix_primary_to_secondary_slice(primary_to_sec_key, false);
   result = eosio::chain::backing_store::db_key_value_format::get_trailing_prim_sec_keys(
         primary_to_sec_key, primary_to_sec_prefix, extracted_primary_key3, extracted_key3);
   BOOST_REQUIRE(result);
   BOOST_CHECK_EQUAL(primary_key, extracted_primary_key3);
   BOOST_CHECK(sec_key == extracted_key3);

   Key extracted_key4;
   primary_to_sec_prefix = eosio::chain::backing_store::db_key_value_format::prefix_primary_to_secondary_slice(primary_to_sec_key, true);
   result = eosio::chain::backing_store::db_key_value_format::get_trailing_secondary_key(
         primary_to_sec_key, primary_to_sec_prefix, extracted_key4);
   BOOST_REQUIRE(result);
   BOOST_CHECK(sec_key == extracted_key4);

   const key_type kt = eosio::chain::backing_store::db_key_value_format::detail::determine_sec_type<Key>::kt;
   verify_secondary_key_pair_order(sec_key_pair, kt);
}

BOOST_AUTO_TEST_CASE(ui64_keys_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const uint64_t key = 0xdead87654321beef;
   const auto composite_key = eosio::chain::backing_store::db_key_value_format::create_primary_key(scope, table, key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key), composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   uint64_t decomposed_key = 0x0;
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_primary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());
   BOOST_CHECK_EQUAL(key, decomposed_key);
   verify_other_gets(composite_key, key_type::primary, scope, table);

   std::vector<uint64_t> keys;
   keys.push_back(0x0);
   keys.push_back(0x1);
   keys.push_back(0x7FFF'FFFF'FFFF'FFFF);
   keys.push_back(0x8000'0000'0000'0000);
   keys.push_back(0xFFFF'FFFF'FFFF'FFFF);
   const bool skip_trailing_prim_key = true;
   check_ordered_keys(keys, &prim_drop_extra_key, skip_trailing_prim_key);

   const name scope2 = N(myscope);
   const name table2 = N(thisscope);
   const uint64_t key2 = 0xdead87654321beef;
   const uint64_t primary_key2 = 0x0123456789abcdef;
   const auto composite_key2 = eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope2, table2, key2, primary_key2);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key) + sizeof(primary_key2), composite_key2.size());

   uint64_t decomposed_primary_key = 0x0;
   BOOST_CHECK(eosio::chain::backing_store::db_key_value_format::get_secondary_key(composite_key2, decomposed_scope, decomposed_table, decomposed_key, decomposed_primary_key));
   BOOST_CHECK_EQUAL(scope2.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table2.to_string(), decomposed_table.to_string());
   BOOST_CHECK_EQUAL(key2, decomposed_key);
   BOOST_CHECK_EQUAL(primary_key2, decomposed_primary_key);
   verify_other_gets(composite_key2, key_type::sec_i64, scope2, table2);

   verify_secondary_key_pair<uint64_t>(composite_key2);

   std::vector<uint64_t> keys2;
   keys2.push_back(0x0);
   keys2.push_back(0x1);
   keys2.push_back(0x7FFF'FFFF'FFFF'FFFF);
   keys2.push_back(0x8000'0000'0000'0000);
   keys2.push_back(0xFFFF'FFFF'FFFF'FFFF);
   check_ordered_keys(keys2, &eosio::chain::backing_store::db_key_value_format::create_secondary_key<uint64_t>);
}

BOOST_AUTO_TEST_CASE(ui128_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const eosio::chain::uint128_t key = create_uint128(0xdead12345678beef, 0x0123456789abcdef);
   const uint64_t primary_key = 0x0123456789abcdef;

   const auto composite_key = eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, key, primary_key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key) + sizeof(primary_key), composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   eosio::chain::uint128_t decomposed_key = 0x0;
   uint64_t decomposed_primary_key = 0x0;
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_secondary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key, decomposed_primary_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());
   BOOST_CHECK(key == decomposed_key);
   BOOST_CHECK_EQUAL(primary_key, decomposed_primary_key);

   verify_other_gets(composite_key, key_type::sec_i128, scope, table);

   verify_secondary_key_pair<eosio::chain::uint128_t>(composite_key);

   std::vector<eosio::chain::uint128_t> keys;
   keys.push_back(create_uint128(0x0, 0x0));
   keys.push_back(create_uint128(0x0, 0x1));
   keys.push_back(create_uint128(0x0, 0x7FFF'FFFF'FFFF'FFFF));
   keys.push_back(create_uint128(0x0, 0x8000'0000'0000'0000));
   keys.push_back(create_uint128(0x0, 0xFFFF'FFFF'FFFF'FFFF));
   keys.push_back(create_uint128(0x1, 0x0));
   keys.push_back(create_uint128(0x1, 0xFFFF'FFFF'FFFF'FFFF));
   keys.push_back(create_uint128(0x7FFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));
   keys.push_back(create_uint128(0x8000'0000'0000'0000, 0xFFFF'FFFF'FFFF'FFFF));
   keys.push_back(create_uint128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));
   check_ordered_keys(keys, &eosio::chain::backing_store::db_key_value_format::create_secondary_key<eosio::chain::uint128_t>);
}

BOOST_AUTO_TEST_CASE(ui256_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const eosio::chain::key256_t key = create_key256(create_uint128(0xdead12345678beef, 0x0123456789abcdef),
                                                    create_uint128(0xbadf2468013579ec, 0xf0e1d2c3b4a59687));
   const uint64_t primary_key = 0x0123456789abcdef;
   const auto composite_key = eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, key, primary_key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key) + sizeof(primary_key), composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   eosio::chain::key256_t decomposed_key = create_key256(create_uint128(0x0, 0x0), create_uint128(0x0, 0x0));
   uint64_t decomposed_primary_key = 0;
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_secondary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key, decomposed_primary_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());

   BOOST_CHECK(key == decomposed_key);
   BOOST_CHECK_EQUAL(primary_key, decomposed_primary_key);

   verify_other_gets(composite_key, key_type::sec_i256, scope, table);

   verify_secondary_key_pair<eosio::chain::key256_t>(composite_key);

   std::vector<eosio::chain::key256_t> keys;
   keys.push_back(create_key256(create_uint128(0x0, 0x0), create_uint128(0x0, 0x0)));
   keys.push_back(create_key256(create_uint128(0x0, 0x0), create_uint128(0x0, 0x01)));
   keys.push_back(create_key256(create_uint128(0x0, 0x0), create_uint128(0x0, 0x7FFF'FFFF'FFFF'FFFF)));
   keys.push_back(create_key256(create_uint128(0x0, 0x0), create_uint128(0x0, 0x8000'0000'0000'0000)));
   keys.push_back(create_key256(create_uint128(0x0, 0x0), create_uint128(0x0, 0xFFFF'FFFF'FFFF'FFFF)));
   keys.push_back(create_key256(create_uint128(0x0, 0x0), create_uint128(0x1, 0x0)));
   keys.push_back(create_key256(create_uint128(0x0, 0x0), create_uint128(0xFFFF'FFFF'FFFF'FFFF, 0x0)));
   keys.push_back(create_key256(create_uint128(0x0, 0x1), create_uint128(0x0, 0x0)));
   keys.push_back(create_key256(create_uint128(0x0, 0xFFFF'FFFF'FFFF'FFFF), create_uint128(0x0, 0x0)));
   keys.push_back(create_key256(create_uint128(0x1, 0x0), create_uint128(0x0, 0x0)));
   keys.push_back(create_key256(create_uint128(0xFFFF'FFFF'FFFF'FFFF, 0x0), create_uint128(0x0, 0x0)));
   check_ordered_keys(keys, &eosio::chain::backing_store::db_key_value_format::create_secondary_key<eosio::chain::key256_t>);
}

BOOST_AUTO_TEST_CASE(float64_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const float64_t key = to_softfloat64(6.0);
   const uint64_t primary_key = 0x0123456789abcdef;
   const auto composite_key = eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, key, primary_key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key) + sizeof(primary_key), composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   float64_t decomposed_key = to_softfloat64(0.0);
   uint64_t decomposed_primary_key = 0;
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_secondary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key, decomposed_primary_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());

   BOOST_CHECK_EQUAL(key, decomposed_key);
   BOOST_CHECK_EQUAL(primary_key, decomposed_primary_key);

   verify_other_gets(composite_key, key_type::sec_double, scope, table);

   verify_secondary_key_pair<float64_t>(composite_key);

   std::vector<float64_t> keys;
   keys.push_back(to_softfloat64(-100000.0));
   keys.push_back(to_softfloat64(-99999.9));
   keys.push_back(to_softfloat64(-1.9));
   keys.push_back(to_softfloat64(0.0));
   keys.push_back(to_softfloat64(0.1));
   keys.push_back(to_softfloat64(1.9));
   keys.push_back(to_softfloat64(99999.9));
   keys.push_back(to_softfloat64(100000.0));
   check_ordered_keys(keys, &eosio::chain::backing_store::db_key_value_format::create_secondary_key<float64_t>);
}

BOOST_AUTO_TEST_CASE(float128_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const float128_t key = to_softfloat128(99.99);
   const uint64_t primary_key = 0x0123456789abcdef;
   const auto composite_key = eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, key, primary_key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key) + sizeof(primary_key), composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   float128_t decomposed_key = to_softfloat128(0.0);
   uint64_t decomposed_primary_key = 0;
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_secondary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key, decomposed_primary_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());

   BOOST_CHECK_EQUAL(key, decomposed_key);
   BOOST_CHECK_EQUAL(primary_key, decomposed_primary_key);

   verify_other_gets(composite_key, key_type::sec_long_double, scope, table);

   verify_secondary_key_pair<float128_t>(composite_key);

   std::vector<float128_t> keys;
   keys.push_back(to_softfloat128(-100000.001));
   keys.push_back(to_softfloat128(-100000.0));
   keys.push_back(to_softfloat128(-99999.9));
   keys.push_back(to_softfloat128(-1.9));
   keys.push_back(to_softfloat128(0.0));
   keys.push_back(to_softfloat128(0.1));
   keys.push_back(to_softfloat128(1.9));
   keys.push_back(to_softfloat128(99999.9));
   keys.push_back(to_softfloat128(100000.0));
   keys.push_back(to_softfloat128(100000.001));
   check_ordered_keys(keys, &eosio::chain::backing_store::db_key_value_format::create_secondary_key<float128_t>);
}

BOOST_AUTO_TEST_CASE(compare_negative_and_positive_f64_0_are_equal_test) {
   std::vector<b1::chain_kv::bytes> composite_keys(2);
   b1::chain_kv::append_key(composite_keys[0], to_softfloat64(-0.0));
   b1::chain_kv::append_key(composite_keys[1], to_softfloat64(0.0));
   // composite key representation should treat -0.0 and -0.0 as the same value
   BOOST_CHECK_EQUAL(0, compare_composite_keys(composite_keys, 0).first);
}

BOOST_AUTO_TEST_CASE(compare_negative_and_positive_f128_0_are_equal_test) {
   std::vector<b1::chain_kv::bytes> composite_keys(2);
   b1::chain_kv::append_key(composite_keys[0], to_softfloat128(-0.0));
   b1::chain_kv::append_key(composite_keys[1], to_softfloat128(0.0));
   // composite key representation should treat -0.0 and -0.0 as the same value
   BOOST_CHECK_EQUAL(0, compare_composite_keys(composite_keys, 0).first);
}

BOOST_AUTO_TEST_CASE(compare_span_of_doubles_test) {
   std::vector<b1::chain_kv::bytes> composite_keys(4);
   uint64_t next = 0;
   const double neg_inf = -std::numeric_limits<double>::infinity();
   b1::chain_kv::append_key(composite_keys[next++], to_softfloat64(neg_inf));
   const double min = std::numeric_limits<double>::min();
   b1::chain_kv::append_key(composite_keys[next++], to_softfloat64(min));
   const double max = std::numeric_limits<double>::max();
   b1::chain_kv::append_key(composite_keys[next++], to_softfloat64(max));
   const double inf = std::numeric_limits<double>::infinity();
   b1::chain_kv::append_key(composite_keys[next++], to_softfloat64(inf));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, 0).first);
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, 1).first);
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, 2).first);
}

BOOST_AUTO_TEST_CASE(compare_span_of_long_doubles_test) {
   std::vector<b1::chain_kv::bytes> composite_keys(4);
   uint64_t next = 0;
   const double neg_inf = -std::numeric_limits<double>::infinity();
   b1::chain_kv::append_key(composite_keys[next++], to_softfloat128(neg_inf));
   const double min = std::numeric_limits<double>::min();
   b1::chain_kv::append_key(composite_keys[next++], to_softfloat128(min));
   const double max = std::numeric_limits<double>::max();
   b1::chain_kv::append_key(composite_keys[next++], to_softfloat128(max));
   const double inf = std::numeric_limits<double>::infinity();
   b1::chain_kv::append_key(composite_keys[next++], to_softfloat128(inf));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, 0).first);
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, 1).first);
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, 2).first);
}

BOOST_AUTO_TEST_CASE(table_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const auto composite_key = eosio::chain::backing_store::db_key_value_format::create_table_key(scope, table);
   BOOST_REQUIRE_EQUAL(overhead_size, composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_table_key(composite_key, decomposed_scope, decomposed_table));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());
   verify_other_gets(composite_key, key_type::table, scope, table);
}

BOOST_AUTO_TEST_CASE(prefix_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const auto composite_key = eosio::chain::backing_store::db_key_value_format::create_prefix_key(scope, table);
   BOOST_REQUIRE_EQUAL(overhead_size_without_type, composite_key.size());  //

   name decomposed_scope;
   name decomposed_table;
   BOOST_REQUIRE_NO_THROW(eosio::chain::backing_store::db_key_value_format::get_prefix_key(composite_key, decomposed_scope, decomposed_table));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());
}

BOOST_AUTO_TEST_CASE(compare_key_type_order_test) {
   std::vector<b1::chain_kv::bytes> composite_keys;
   // enforce the order for the different key types with the same scope and table
   const name scope;
   const name table;
   uint64_t next = 0;
   const uint64_t max = std::numeric_limits<uint64_t>::max();
   const eosio::chain::uint128_t max_i128 = create_uint128(max, max);
   const eosio::chain::key256_t max_i256 = create_key256(create_uint128(max, max), create_uint128(max, max));
   const float64_t max_f64 = to_softfloat64(std::numeric_limits<double>::max());
   const float128_t max_f128 = to_softfloat128(std::numeric_limits<double>::max()); // close enough
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_prefix_key(scope, table));
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_primary_key(scope, table, 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_primary_key(scope, table, max));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, uint64_t(0x0), 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, max, 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, create_uint128(0x0, 0x0), 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, max_i128, 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, create_key256(create_uint128(0x0, 0x0), create_uint128(0x0, 0x0)), 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, max_i256, 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, to_softfloat64(0.0), 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, max_f64, 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, to_softfloat128(0.0), 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_secondary_key(scope, table, max_f128, 0x0));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
   composite_keys.push_back(eosio::chain::backing_store::db_key_value_format::create_table_key(scope, table));
   BOOST_CHECK_EQUAL(-1, compare_composite_keys(composite_keys, next++).first);
}

template <typename Exception>
void verify_primary_exception(const b1::chain_kv::bytes& composite_key, const std::string& starts_with) {
   uint64_t decomposed_key;
   name decomposed_scope;
   name decomposed_table;
   BOOST_CHECK_EXCEPTION(
         eosio::chain::backing_store::db_key_value_format::get_primary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key),
         Exception,
         eosio::testing::fc_exception_message_starts_with(starts_with)
   );
}

template <typename Key, typename Exception>
void verify_secondary_exception(const b1::chain_kv::bytes& composite_key, const std::string& starts_with) {
   name decomposed_scope;
   name decomposed_table;
   Key decomposed_key = helper<Key>().init;
   uint64_t decomposed_primary_key = 0;
   BOOST_CHECK_EXCEPTION(
         eosio::chain::backing_store::db_key_value_format::get_secondary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key, decomposed_primary_key),
         Exception,
         eosio::testing::fc_exception_message_starts_with(starts_with)
   );
}

BOOST_AUTO_TEST_CASE(verify_no_type_in_key_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const auto composite_key = eosio::chain::backing_store::db_key_value_format::create_prefix_key(scope, table);
   const std::string starts_with = "DB intrinsic key-value store composite key is malformed, it does not contain an indication of the type of the db-key";
   verify_primary_exception<eosio::chain::bad_composite_key_exception>(composite_key, starts_with);
   verify_secondary_exception<uint64_t, eosio::chain::bad_composite_key_exception>(composite_key, starts_with);
   verify_secondary_exception<eosio::chain::uint128_t, eosio::chain::bad_composite_key_exception>(composite_key, starts_with);
   verify_secondary_exception<eosio::chain::key256_t, eosio::chain::bad_composite_key_exception>(composite_key, starts_with);
   verify_secondary_exception<float64_t, eosio::chain::bad_composite_key_exception>(composite_key, starts_with);
   verify_secondary_exception<float128_t, eosio::chain::bad_composite_key_exception>(composite_key, starts_with);
}

using namespace eosio::chain::backing_store;

template<typename T>
void verify_table(const T& table_cache, const unique_table& orig_table, int table_itr) {
   auto cached_table = table_cache.find_table_by_end_iterator(table_itr);
   BOOST_CHECK(&orig_table != cached_table); // verifying that table cache is creating its own objects
   BOOST_CHECK(orig_table.contract == cached_table->contract);
   BOOST_CHECK(orig_table.scope == cached_table->scope);
   BOOST_CHECK(orig_table.table == cached_table->table);
   unique_table table_copy = orig_table;
   auto table_copy_iter = table_cache.get_end_iterator_by_table(table_copy);
   BOOST_CHECK_EQUAL(table_itr, table_copy_iter);
}

BOOST_AUTO_TEST_CASE(table_cache_test) {
   db_key_value_iter_store<uint64_t> i64_cache;

   const unique_table t1 = { name{0}, name{0}, name{0} };
   const auto t1_itr = i64_cache.cache_table(t1);
   BOOST_CHECK_LT(t1_itr, i64_cache.invalid_iterator());
   verify_table(i64_cache, t1, t1_itr);

   const unique_table t2 = { name{0}, name{0}, name{1} };
   const auto t2_itr = i64_cache.cache_table(t2);
   BOOST_CHECK_LT(t2_itr, i64_cache.invalid_iterator());
   verify_table(i64_cache, t2, t2_itr);

   const unique_table t3 = { name{0}, name{1}, name{0} };
   const auto t3_itr = i64_cache.cache_table(t3);
   BOOST_CHECK_LT(t3_itr, i64_cache.invalid_iterator());
   verify_table(i64_cache, t3, t3_itr);

   const unique_table t4 = { name{1}, name{0}, name{0} };
   const auto t4_itr = i64_cache.cache_table(t4);
   BOOST_CHECK_LT(t4_itr, i64_cache.invalid_iterator());
   verify_table(i64_cache, t4, t4_itr);

   BOOST_CHECK_NE(t1_itr, t2_itr);
   BOOST_CHECK_NE(t1_itr, t3_itr);
   BOOST_CHECK_NE(t1_itr, t4_itr);
   BOOST_CHECK_NE(t2_itr, t3_itr);
   BOOST_CHECK_NE(t2_itr, t4_itr);
   BOOST_CHECK_NE(t3_itr, t4_itr);

   const unique_table t5 = { name{1}, name{0}, name{1} };
   BOOST_CHECK_THROW(i64_cache.get_end_iterator_by_table(t5), eosio::chain::table_not_in_cache);
   BOOST_CHECK_THROW(i64_cache.find_table_by_end_iterator(i64_cache.invalid_iterator()), eosio::chain::invalid_table_iterator);
   const auto non_existent_itr = t4_itr - 1;
   BOOST_CHECK_EQUAL(nullptr, i64_cache.find_table_by_end_iterator(non_existent_itr));
}

BOOST_AUTO_TEST_CASE(invalid_itr_cache_test) {
   db_key_value_iter_store<eosio::chain::uint128_t> i128_cache;
   BOOST_CHECK_EXCEPTION(
      i128_cache.get( i128_cache.invalid_iterator() ), eosio::chain::invalid_table_iterator,
      eosio::testing::fc_exception_message_is( "invalid iterator" )
   );
   BOOST_CHECK_EXCEPTION(
      i128_cache.get( i128_cache.invalid_iterator() - 1 ), eosio::chain::table_operation_not_permitted,
      eosio::testing::fc_exception_message_is( "dereference of end iterator" )
   );
   BOOST_CHECK_EXCEPTION(
      i128_cache.get( 0 ), eosio::chain::invalid_table_iterator,
      eosio::testing::fc_exception_message_is( "iterator out of range" )
   );

   BOOST_CHECK_EXCEPTION(
      i128_cache.remove( i128_cache.invalid_iterator() ), eosio::chain::invalid_table_iterator,
      eosio::testing::fc_exception_message_is( "invalid iterator" )
   );
   BOOST_CHECK_EXCEPTION(
      i128_cache.remove( i128_cache.invalid_iterator() - 1 ), eosio::chain::table_operation_not_permitted,
      eosio::testing::fc_exception_message_is( "cannot call remove on end iterators" )
   );
   BOOST_CHECK_EXCEPTION(
      i128_cache.remove( 0 ), eosio::chain::invalid_table_iterator,
      eosio::testing::fc_exception_message_is( "iterator out of range" )
   );

   eosio::chain::uint128_t key = 0;
   eosio::chain::account_name payer;
   BOOST_CHECK_EXCEPTION(
      i128_cache.swap( i128_cache.invalid_iterator(), key, payer ), eosio::chain::invalid_table_iterator,
      eosio::testing::fc_exception_message_is( "invalid iterator" )
   );
   BOOST_CHECK_EXCEPTION(
      i128_cache.swap( i128_cache.invalid_iterator() - 1, key, payer ), eosio::chain::table_operation_not_permitted,
      eosio::testing::fc_exception_message_is( "cannot call swap on end iterators" )
   );
   BOOST_CHECK_EXCEPTION(
      i128_cache.swap( 0, key, payer ), eosio::chain::invalid_table_iterator,
      eosio::testing::fc_exception_message_is( "iterator out of range" )
   );

   using i128_type = secondary_key<eosio::chain::uint128_t>;
   const i128_type i128_1 = { .table_ei = i128_cache.invalid_iterator(), .secondary = 0x0, .primary = 0x0, .payer = name{} };
   BOOST_CHECK_EXCEPTION(
      i128_cache.add( i128_1 ), eosio::chain::invalid_table_iterator,
      eosio::testing::fc_exception_message_is( "not an end iterator" )
   );
   const i128_type i128_2 = { .table_ei = i128_cache.invalid_iterator() - 1, .secondary = 0x0, .primary = 0x0, .payer = name{} };
   BOOST_CHECK_EXCEPTION(
      i128_cache.add( i128_2 ), eosio::chain::invalid_table_iterator,
      eosio::testing::fc_exception_message_is( "an invariant was broken, table should be in cache" )
   );
}

template<typename T>
void validate_get(const db_key_value_iter_store<T>& cache, const secondary_key<T>& orig, int itr, bool check_payer = true) {
   const auto& obj_in_cache = cache.get(itr);
   BOOST_CHECK_NE(&orig, &obj_in_cache);
   BOOST_CHECK_EQUAL(obj_in_cache.table_ei, orig.table_ei);
   BOOST_CHECK(obj_in_cache.secondary == orig.secondary);
   BOOST_CHECK_EQUAL(obj_in_cache.primary, orig.primary);
   const int found_itr = cache.find(orig);
   BOOST_CHECK_EQUAL(itr, found_itr);
   if (check_payer) {
      BOOST_CHECK_EQUAL(obj_in_cache.payer, orig.payer);
   }
   else {
      // invariant needed to ensure payer not required, if this fails either that has changed, or check_payer was incorrectly set to false
      BOOST_CHECK_NE(obj_in_cache.payer, orig.payer);
   }
}

struct i64_values {
   using key = uint64_t;
   key sec_val_1 = 0x0;
   key sec_val_2 = 0x1;
   key sec_val_3 = 0x2;
   key sec_val_4 = 0x3;
};

struct i128_values {
   using key = eosio::chain::uint128_t;
   key sec_val_1 = create_uint128(0x0, 0x0);
   key sec_val_2 = create_uint128(0x0, 0x1);
   key sec_val_3 = create_uint128(0x1, 0x0);
   key sec_val_4 = create_uint128(0x1, 0x1);
};

using key_suite = boost::mpl::list<i64_values, i128_values>;

BOOST_AUTO_TEST_CASE_TEMPLATE(itr_cache_test, KEY_SUITE, key_suite) {
   db_key_value_iter_store<typename KEY_SUITE::key> key_cache;
   KEY_SUITE keys;
   using key_type = typename db_key_value_iter_store<typename KEY_SUITE::key>::secondary_obj_type;
   const unique_table t1 = { name{0}, name{0}, name{0} };
   const unique_table t2 = { name{0}, name{0}, name{1} };
   const auto t1_itr = key_cache.cache_table(t1);
   const auto t2_itr = key_cache.cache_table(t2);
   const bool dont_check_payer = false;

   const key_type key_1 = { .table_ei = t1_itr, .secondary = keys.sec_val_1, .primary = 0x0, .payer = name{0x0} };
   const auto key_1_itr = key_cache.add(key_1);
   BOOST_CHECK_LT(key_cache.invalid_iterator(), key_1_itr);
   validate_get(key_cache, key_1, key_1_itr);

   // verify that payer isn't part of uniqueness
   const key_type key_1_diff_payer = { .table_ei = t1_itr, .secondary = keys.sec_val_1, .primary = 0x0, .payer = name{0x1} };
   validate_get(key_cache, key_1_diff_payer, key_1_itr, dont_check_payer);

   const key_type key_2 = { .table_ei = t1_itr, .secondary = keys.sec_val_1, .primary = 0x1, .payer = name{0x0} };
   const auto key_2_itr = key_cache.add(key_2);
   BOOST_CHECK_LT(key_cache.invalid_iterator(), key_2_itr);
   validate_get(key_cache, key_2, key_2_itr);
   BOOST_CHECK_LT(key_1_itr, key_2_itr);

   const key_type key_3 = { .table_ei = t1_itr, .secondary = keys.sec_val_2, .primary = 0x0, .payer = name{0x0} };
   const auto key_3_itr = key_cache.add(key_3);
   BOOST_CHECK_LT(key_cache.invalid_iterator(), key_3_itr);
   validate_get(key_cache, key_3, key_3_itr);
   BOOST_CHECK_LT(key_2_itr, key_3_itr);

   const key_type key_4 = { .table_ei = t2_itr, .secondary = keys.sec_val_1, .primary = 0x0, .payer = name{0x0} };
   const auto key_4_itr = key_cache.add(key_4);
   BOOST_CHECK_LT(key_cache.invalid_iterator(), key_4_itr);
   validate_get(key_cache, key_4, key_4_itr);
   BOOST_CHECK_LT(key_3_itr, key_4_itr);

   key_cache.remove(key_1_itr);
   BOOST_CHECK_EXCEPTION(
      key_cache.get(key_1_itr), eosio::chain::table_operation_not_permitted,
      eosio::testing::fc_exception_message_is( "dereference of deleted object" )
   );
   // verify remove of an iterator that previously existed is fine
   key_cache.remove(key_1_itr);
   // verify swap of an iterator that previously existed is fine
   key_cache.swap(key_1_itr, keys.sec_val_3, name{0x2});
   // validate that the repeat of key_1 w/ different payer can be added now
   const auto key_1_diff_payer_itr = key_cache.add(key_1_diff_payer);
   BOOST_CHECK_LT(key_4_itr, key_1_diff_payer_itr);
   validate_get(key_cache, key_1_diff_payer, key_1_diff_payer_itr);

   const key_type key_2_swap_payer = { .table_ei = key_2.table_ei, .secondary = key_2.secondary, .primary = key_2.primary, .payer = name{0x2} };
   key_cache.swap(key_2_itr, key_2.secondary, key_2_swap_payer.payer); // same key, different payer
   validate_get(key_cache, key_2_swap_payer, key_2_itr);
   const auto key_2_swap_payer_itr = key_cache.add(key_2_swap_payer);
   BOOST_CHECK_EQUAL(key_2_itr, key_2_swap_payer_itr);
   const key_type key_2_swap_key = { .table_ei = key_2.table_ei, .secondary = keys.sec_val_3, .primary = key_2.primary, .payer = key_2_swap_payer.payer };
   key_cache.swap(key_2_itr, key_2_swap_key.secondary, key_2_swap_key.payer); // same key, different payer
   validate_get(key_cache, key_2_swap_key, key_2_itr);
   const key_type key_2_swap_both = { .table_ei = key_2.table_ei, .secondary = keys.sec_val_4, .primary = key_2.primary, .payer = name{0x3} };
   key_cache.swap(key_2_itr, key_2_swap_both.secondary, key_2_swap_both.payer); // same key, different payer
   validate_get(key_cache, key_2_swap_both, key_2_itr);
   const auto key_2_reload_itr = key_cache.add(key_2);
   BOOST_CHECK_LT(key_1_diff_payer_itr, key_2_reload_itr);
   key_cache.remove(key_2_itr);
   key_cache.swap(key_2_itr, key_2.secondary, key_2_swap_payer.payer); // same key, different payer
}


BOOST_AUTO_TEST_SUITE_END()
