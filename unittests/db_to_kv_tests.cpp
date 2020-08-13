#include <eosio/testing/tester.hpp>
#include <eosio/chain/db_key_value_format.hpp>

#include <boost/test/unit_test.hpp>
#include <cstring>

using key_type = eosio::chain::db_key_value_format::key_type;

BOOST_AUTO_TEST_SUITE(db_to_kv_tests)

const uint64_t overhead_size = sizeof(name) * 2 + 1; // 8 (scope) + 8 (table) + 1 (key type)

void print_bytes(const bytes& composite_key) {
   printf("byte contents: ");
   uint64_t i = 0;
   for (; i < overhead_size; ++i) {
      printf("%02x ", (uint8_t)composite_key[i]);
   }
   printf(" -> < ");
   for (; i < composite_key.size(); ++i) {
      printf("%02x ", (uint8_t)composite_key[i]);
   }
   printf("> \n");
}

void print_hex(uint8_t* num, uint64_t size) {
   printf("key ptr: %08lx\n", reinterpret_cast<uint64_t>(num));
   printf("key: ");
   for (uint64_t i = 0; i < size; ++i) {
      printf("%02x ", num[i]);
   }
   printf("\n");
}

template<typename Type>
void verify_secondary_wont_convert(const bytes& composite_key) {
   name scope;
   name table;
   Type key;
   BOOST_CHECK(!eosio::chain::db_key_value_format::get_secondary_key(composite_key, scope, table, key));
}

void verify_secondary_wont_convert(const bytes& composite_key, key_type except) {
   if (except != key_type::primary) {
      name scope;
      name table;
      uint64_t key = 0;
      BOOST_CHECK_THROW(eosio::chain::db_key_value_format::get_primary_key(composite_key, scope, table, key), bad_composite_key_exception);
   }
   if (except != key_type::sec_i64) { verify_secondary_wont_convert<uint64_t>(composite_key); }
   if (except != key_type::sec_i128) { verify_secondary_wont_convert<eosio::chain::uint128_t>(composite_key); }
   if (except != key_type::sec_i256) { verify_secondary_wont_convert<eosio::chain::key256_t>(composite_key); }
   if (except != key_type::sec_double) { verify_secondary_wont_convert<float64_t>(composite_key); }
   if (except != key_type::sec_long_double) { verify_secondary_wont_convert<float128_t>(composite_key); }
}

float128_t to_softfloat128( long double d ) {
   float128_t x;
   memcpy(&x, &d, sizeof(d));
   return x;
}

void compare_composite_keys(const std::vector<bytes>& keys, uint64_t lhs_index, std::string desc) {
   const bytes& lhs = keys[lhs_index];
   const bytes& rhs = keys[lhs_index + 1];
   bool found_less_than = false;
   printf("left :");
   for (unsigned int j = 0; j < lhs.size(); ++j) {
      const auto cur_char = uint64_t(static_cast<unsigned char>(lhs[j]));
      printf(" %02lx", cur_char);
   }
   printf("\nright:");
   for (unsigned int j = 0; j < lhs.size(); ++j) {
      const auto cur_char = uint64_t(static_cast<unsigned char>(rhs[j]));
      printf(" %02lx", cur_char);
   }
   printf("\n");
   for (unsigned int j = 0; j < lhs.size(); ++j) {
      const auto left = uint64_t(static_cast<unsigned char>(lhs[j]));
      const auto right = uint64_t(static_cast<unsigned char>(rhs[j]));
      const auto op = left < right ? "<" : (right < left ? ">" : "==");
      ilog("${j}:  ${k} ${l} ${m}", ("j", j)("k", std::to_string(left))("l", op)("m", std::to_string(right)));
      if (left != right) {
         BOOST_CHECK_MESSAGE(left < right, "expected " + std::to_string(lhs_index) +
                             "th composite key to be less than the " + std::to_string(lhs_index + 1) + "th, but the " + std::to_string(j) +
                             "th character is actually greater on the left than the right (" + std::to_string(left) + " > " +
                             std::to_string(right) + ").  Called from: " + desc);
         found_less_than = true;
         break;
      }
   }
   BOOST_CHECK_MESSAGE(found_less_than, "expected " + std::to_string(lhs_index) +
                                        "th composite key to be less than the " + std::to_string(lhs_index + 1) +
                                        "th, but they were identical.  Called from: " + desc);
}

template<typename T, typename KeyFunc>
void verify_table_scope_order(T low_key, T high_key, KeyFunc key_func) {
   std::vector<bytes> comp_keys;
   comp_keys.push_back(key_func(name{0}, name{0}, low_key));
   comp_keys.push_back(key_func(name{1}, name{0}, low_key));
   comp_keys.push_back(key_func(name{1}, name{1}, low_key));
   comp_keys.push_back(key_func(name{1}, name{1}, high_key));
   comp_keys.push_back(key_func(name{1}, name{2}, low_key));
   comp_keys.push_back(key_func(name{1}, name{0xffff'ffff'ffff'ffff}, low_key));
   comp_keys.push_back(key_func(name{0xffff'ffff'ffff'ffff}, name{0}, low_key));
   comp_keys.push_back(key_func(name{0xffff'ffff'ffff'ffff}, name{0xffff'ffff'ffff'ffff}, low_key));
   for (unsigned int i = 0; i < comp_keys.size() - 1; ++i) {
      compare_composite_keys(comp_keys, i, "verify_table_scope_order");
   }
}

template<typename T, typename KeyFunc>
void check_ordered_keys(const std::vector<T> ordered_keys, KeyFunc key_func) {
   verify_table_scope_order(ordered_keys[0], ordered_keys[1], key_func);
   name scope{0};
   name table{0};
   std::vector<bytes> composite_keys;
   for (const auto key: ordered_keys) {
      auto comp_key = key_func(scope, table, key);
      comp_key.push_back('\0');
      composite_keys.push_back(comp_key);
   }
   for (unsigned int i = 0; i < composite_keys.size() - 1; ++i) {
      compare_composite_keys(composite_keys, i, "check_ordered_keys");
   }
}

BOOST_AUTO_TEST_CASE(ui64_keys_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const uint64_t key = 0xdead87654321beef;
   const auto composite_key = eosio::chain::db_key_value_format::create_primary_key(scope, table, key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key), composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   uint64_t decomposed_key = 0x0;
   BOOST_REQUIRE_NO_THROW(eosio::chain::db_key_value_format::get_primary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());
   BOOST_CHECK_EQUAL(key, decomposed_key);
   verify_secondary_wont_convert(composite_key, key_type::primary);

   std::vector<uint64_t> keys;
   keys.push_back(0x0);
   keys.push_back(0x1);
   keys.push_back(0x7FFF'FFFF'FFFF'FFFF);
   keys.push_back(0x8000'0000'0000'0000);
   keys.push_back(0xFFFF'FFFF'FFFF'FFFF);
   check_ordered_keys(keys, &eosio::chain::db_key_value_format::create_primary_key);

   const name scope2 = N(myscope);
   const name table2 = N(thisscope);
   const uint64_t key2 = 0xdead87654321beef;
   const auto composite_key2 = eosio::chain::db_key_value_format::create_secondary_key(scope2, table2, key2);
   BOOST_REQUIRE_EQUAL(25, composite_key2.size());

   BOOST_CHECK(eosio::chain::db_key_value_format::get_secondary_key(composite_key2, decomposed_scope, decomposed_table, decomposed_key));
   BOOST_CHECK_EQUAL(scope2.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table2.to_string(), decomposed_table.to_string());
   BOOST_CHECK_EQUAL(key2, decomposed_key);
   verify_secondary_wont_convert(composite_key2, key_type::sec_i64);

   std::vector<uint64_t> keys2;
   keys2.push_back(0x0);
   keys2.push_back(0x1);
   keys2.push_back(0x7FFF'FFFF'FFFF'FFFF);
   keys2.push_back(0x8000'0000'0000'0000);
   keys2.push_back(0xFFFF'FFFF'FFFF'FFFF);
   check_ordered_keys(keys2, &eosio::chain::db_key_value_format::create_secondary_key<uint64_t>);
}

eosio::chain::uint128_t create_uint128(uint64_t upper, uint64_t lower) {
   return (eosio::chain::uint128_t(upper) << 64) + lower;
}

BOOST_AUTO_TEST_CASE(ui128_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const eosio::chain::uint128_t key = create_uint128(0xdead12345678beef, 0x0123456789abcdef);

   const auto composite_key = eosio::chain::db_key_value_format::create_secondary_key(scope, table, key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key), composite_key.size());
   ilog("key size= ${ks}",("ks", composite_key.size()));

   name decomposed_scope;
   name decomposed_table;
   eosio::chain::uint128_t decomposed_key = 0x0;
   BOOST_REQUIRE_NO_THROW(eosio::chain::db_key_value_format::get_secondary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());
   BOOST_CHECK(key == decomposed_key);

   verify_secondary_wont_convert(composite_key, key_type::sec_i128);

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
   check_ordered_keys(keys, &eosio::chain::db_key_value_format::create_secondary_key<eosio::chain::uint128_t>);
}

eosio::chain::key256_t create_key256(eosio::chain::uint128_t upper, eosio::chain::uint128_t lower) {
   return { upper, lower };
}

BOOST_AUTO_TEST_CASE(ui256_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const eosio::chain::key256_t key = create_key256(create_uint128(0xdead12345678beef, 0x0123456789abcdef),
                                                    create_uint128(0xbadf2468013579ec, 0xf0e1d2c3b4a59687));
   const auto composite_key = eosio::chain::db_key_value_format::create_secondary_key(scope, table, key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key), composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   eosio::chain::key256_t decomposed_key = create_key256(create_uint128(0x0, 0x0), create_uint128(0x0, 0x0));
   BOOST_REQUIRE_NO_THROW(eosio::chain::db_key_value_format::get_secondary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());

   BOOST_CHECK(key == decomposed_key);

   verify_secondary_wont_convert(composite_key, key_type::sec_i256);

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
   check_ordered_keys(keys, &eosio::chain::db_key_value_format::create_secondary_key<eosio::chain::key256_t>);
}

BOOST_AUTO_TEST_CASE(test_test) {
   const auto key1 = to_softfloat64(0.0);
   const auto key2 = to_softfloat64(0.1);
   const auto key3 = to_softfloat64(0.2);
   const auto composite_key1 = eosio::chain::db_key_value_format::create_secondary_key(name{2}, name{4}, key1);
   const auto composite_key2 = eosio::chain::db_key_value_format::create_secondary_key(name{2}, name{4}, key2);
   const auto composite_key3 = eosio::chain::db_key_value_format::create_secondary_key(name{2}, name{4}, key3);
   print_hex((uint8_t*)composite_key1.data(), composite_key1.size());
   print_hex((uint8_t*)composite_key2.data(), composite_key2.size());
   print_hex((uint8_t*)composite_key3.data(), composite_key3.size());
}

BOOST_AUTO_TEST_CASE(float64_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const float64_t key = to_softfloat64(6.0);
   const auto composite_key = eosio::chain::db_key_value_format::create_secondary_key(scope, table, key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key), composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   float64_t decomposed_key = to_softfloat64(0.0);
   BOOST_REQUIRE_NO_THROW(eosio::chain::db_key_value_format::get_secondary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());

   BOOST_CHECK_EQUAL(key, decomposed_key);

   verify_secondary_wont_convert(composite_key, key_type::sec_double);

   std::vector<float64_t> keys;
   keys.push_back(to_softfloat64(-100000.0));
   keys.push_back(to_softfloat64(-99999.9));
   keys.push_back(to_softfloat64(-1.9));
   keys.push_back(to_softfloat64(0.0));
   keys.push_back(to_softfloat64(0.1));
   keys.push_back(to_softfloat64(1.9));
   keys.push_back(to_softfloat64(99999.9));
   keys.push_back(to_softfloat64(100000.0));
   check_ordered_keys(keys, &eosio::chain::db_key_value_format::create_secondary_key<float64_t>);
}

BOOST_AUTO_TEST_CASE(float128_key_conversions_test) {
   const name scope = N(myscope);
   const name table = N(thisscope);
   const float128_t key = to_softfloat128(99.99);
   const auto composite_key = eosio::chain::db_key_value_format::create_secondary_key(scope, table, key);
   BOOST_REQUIRE_EQUAL(overhead_size + sizeof(key), composite_key.size());

   name decomposed_scope;
   name decomposed_table;
   float128_t decomposed_key = to_softfloat128(0.0);
   BOOST_REQUIRE_NO_THROW(eosio::chain::db_key_value_format::get_secondary_key(composite_key, decomposed_scope, decomposed_table, decomposed_key));
   BOOST_CHECK_EQUAL(scope.to_string(), decomposed_scope.to_string());
   BOOST_CHECK_EQUAL(table.to_string(), decomposed_table.to_string());

   BOOST_CHECK_EQUAL(key, decomposed_key);

   verify_secondary_wont_convert(composite_key, key_type::sec_long_double);

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
   check_ordered_keys(keys, &eosio::chain::db_key_value_format::create_secondary_key<float128_t>);
}

BOOST_AUTO_TEST_SUITE_END()
