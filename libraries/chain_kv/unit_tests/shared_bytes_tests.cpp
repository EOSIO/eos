#include <b1/session/shared_bytes.hpp>
#include <boost/test/unit_test.hpp>

using namespace eosio::session;

BOOST_AUTO_TEST_SUITE(shared_bytes_tests)

BOOST_AUTO_TEST_CASE(make_shared_bytes_test) {
   static constexpr auto* char_value  = "hello world";
   static const auto      char_length = strlen(char_value) - 1;
   static constexpr auto  int_value   = int64_t{ 100000000 };

   auto b1 = shared_bytes(char_value, char_length);
   BOOST_REQUIRE(memcmp(b1.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b1.size() == char_length);

   auto b2 = shared_bytes(reinterpret_cast<const int8_t*>(char_value), char_length);
   BOOST_REQUIRE(memcmp(b1.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b2.size() == char_length);

   auto b3 = shared_bytes(&int_value, 1);
   BOOST_REQUIRE(*(reinterpret_cast<const decltype(int_value)*>(b3.data())) == int_value);
   BOOST_REQUIRE(b3.size() == sizeof(decltype(int_value)));

   auto b4 = shared_bytes(char_value, char_length);
   BOOST_REQUIRE(memcmp(b4.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b4.size() == char_length);
   BOOST_REQUIRE(!b4.empty());

   auto invalid = shared_bytes(static_cast<int8_t*>(nullptr), 0);
   BOOST_REQUIRE(invalid.data() == nullptr);
   BOOST_REQUIRE(invalid.size() == 0);

   BOOST_REQUIRE(b1 == b2);
   BOOST_REQUIRE(b1 == b4);
   BOOST_REQUIRE(invalid == shared_bytes{});
   BOOST_REQUIRE(b1 != b3);

   auto b5 = b1;
   BOOST_REQUIRE(memcmp(b5.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b5.size() == char_length);
   BOOST_REQUIRE(b1 == b5);

   auto b6{ b1 };
   BOOST_REQUIRE(memcmp(b6.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b6.size() == char_length);
   BOOST_REQUIRE(b1 == b6);

   auto b7 = std::move(b1);
   BOOST_REQUIRE(memcmp(b7.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b7.size() == char_length);

   auto b8{ std::move(b2) };
   BOOST_REQUIRE(memcmp(b8.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b8.size() == char_length);
}

BOOST_AUTO_TEST_CASE(next_test) {
   static constexpr auto* a_value  = "a";
   static constexpr auto* a_next_expected = "b";
   auto a = shared_bytes(a_value, 1);
   BOOST_REQUIRE(memcmp(a.next().data(), reinterpret_cast<const int8_t*>(a_next_expected), 1) == 0);
   BOOST_REQUIRE(a.next().size() == 1);

   static constexpr auto* aa_value  = "aa";
   static constexpr auto* aa_next_expected = "ab";
   auto aa = shared_bytes(aa_value, 2);
   BOOST_REQUIRE(memcmp(aa.next().data(), reinterpret_cast<const int8_t*>(aa_next_expected), 2) == 0);
   BOOST_REQUIRE(aa.next().size() == 2);

   static constexpr auto* empty_value  = "";
   auto empty = shared_bytes(empty_value, 0);
   BOOST_CHECK_THROW(empty.next().size(), eosio::chain::chain_exception);

   // next of a sequence of 0xFFs is empty
   char single_last_value[1] = {static_cast<char>(0xFF)};
   auto single_last = shared_bytes(single_last_value, 1);
   BOOST_CHECK_THROW(single_last.next().size(), eosio::chain::chain_exception);

   char double_last_value[2] = {static_cast<char>(0xFF), static_cast<char>(0xFF)};
   auto double_last = shared_bytes(double_last_value, 2);
   BOOST_CHECK_THROW(double_last.next().size(), eosio::chain::chain_exception);

   char mixed_last_value[2] = {static_cast<char>(0xFE), static_cast<char>(0xFF)};
   char mixed_last_next_expected[1] = {static_cast<char>(0xFF)};
   auto mixed_last = shared_bytes(mixed_last_value, 2);
   BOOST_REQUIRE(memcmp(mixed_last.next().data(), reinterpret_cast<const int8_t*>(mixed_last_next_expected), 1) == 0);
   BOOST_REQUIRE(mixed_last.next().size() == 1);
}

BOOST_AUTO_TEST_CASE(truncate_key_test) {
   static constexpr auto* a_value  = "a";
   auto a = shared_bytes(a_value, 1);
   auto trunc_key_a = shared_bytes::truncate_key(a);
   BOOST_REQUIRE(trunc_key_a.size() == 0);

   static constexpr auto* aa_value  = "aa";
   static constexpr auto* aa_truncated_expected = "a";
   auto aa = shared_bytes(aa_value, 2);
   auto trunc_key_aa = shared_bytes::truncate_key(aa);
   BOOST_REQUIRE(memcmp(trunc_key_aa.data(), reinterpret_cast<const int8_t*>(aa_truncated_expected), 1) == 0);
   BOOST_REQUIRE(trunc_key_aa.size() == 1);

   static constexpr auto* empty_value  = "";
   auto empty = shared_bytes(empty_value, 0);
   BOOST_CHECK_THROW(shared_bytes::truncate_key(empty), eosio::chain::chain_exception);
}

BOOST_AUTO_TEST_SUITE_END();
