#include <boost/test/unit_test.hpp>
#include <chain_kv/shared_bytes.hpp>

using namespace eosio::chain_kv;

BOOST_AUTO_TEST_SUITE(shared_bytes_tests)

BOOST_AUTO_TEST_CASE(make_shared_bytes_test) {
   static constexpr auto* char_value  = "hello world";
   static const auto      char_length = strlen(char_value) - 1;
   static constexpr auto  int_value   = int64_t{ 100000000 };

   shared_bytes b1 = {char_value, char_length};
   BOOST_REQUIRE(memcmp(b1.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b1.size() == char_length);

   shared_bytes b2 = {reinterpret_cast<const int8_t*>(char_value), char_length};
   BOOST_REQUIRE(memcmp(b1.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b2.size() == char_length);

   shared_bytes b3 = {&int_value, 1};
   BOOST_REQUIRE(*(reinterpret_cast<const decltype(int_value)*>(b3.data())) == int_value);
   BOOST_REQUIRE(b3.size() == sizeof(decltype(int_value)));

   shared_bytes b4 = {char_value, char_length};
   BOOST_REQUIRE(memcmp(b4.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b4.size() == char_length);

   shared_bytes invalid = {static_cast<int8_t*>(nullptr), std::numeric_limits<size_t>::max()};
   BOOST_REQUIRE(invalid.data() == nullptr);
   BOOST_REQUIRE(invalid.size() == std::numeric_limits<size_t>::max());

   BOOST_REQUIRE(b1 == b2);
   BOOST_REQUIRE(b1 == b4);
   BOOST_REQUIRE(invalid == shared_bytes::invalid());
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

BOOST_AUTO_TEST_SUITE_END();
