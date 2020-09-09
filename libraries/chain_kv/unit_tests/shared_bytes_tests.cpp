#include <b1/session/boost_memory_allocator.hpp>
#include <b1/session/shared_bytes.hpp>
#include <boost/test/unit_test.hpp>

using namespace eosio::session;

BOOST_AUTO_TEST_SUITE(bytes_tests)

BOOST_AUTO_TEST_CASE(make_shared_bytes_test) {
   static constexpr auto* char_value  = "hello world";
   static const auto      char_length = strlen(char_value) - 1;
   static constexpr auto  int_value   = int64_t{ 100000000 };

   auto allocator = boost_memory_allocator::make();

   auto b1 = make_shared_bytes(char_value, char_length, allocator);
   BOOST_REQUIRE(memcmp(b1.data(), reinterpret_cast<const void*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b1.length() == char_length);

   auto b2 = make_shared_bytes(reinterpret_cast<const void*>(char_value), char_length, allocator);
   BOOST_REQUIRE(memcmp(b1.data(), reinterpret_cast<const void*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b2.length() == char_length);

   auto b3 = make_shared_bytes(&int_value, 1, allocator);
   BOOST_REQUIRE(*(reinterpret_cast<const decltype(int_value)*>(b3.data())) == int_value);
   BOOST_REQUIRE(b3.length() == sizeof(decltype(int_value)));

   auto b4 = make_shared_bytes(char_value, char_length);
   BOOST_REQUIRE(memcmp(b4.data(), reinterpret_cast<const void*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b4.length() == char_length);

   auto invalid = make_shared_bytes(static_cast<void*>(nullptr), 0, allocator);
   BOOST_REQUIRE(invalid.data() == nullptr);
   BOOST_REQUIRE(invalid.length() == 0);

   BOOST_REQUIRE(b1 == b2);
   BOOST_REQUIRE(b1 == b4);
   BOOST_REQUIRE(invalid == shared_bytes::invalid);
   BOOST_REQUIRE(b1 != b3);

   auto b5 = b1;
   BOOST_REQUIRE(memcmp(b5.data(), reinterpret_cast<const void*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b5.length() == char_length);
   BOOST_REQUIRE(b1 == b5);

   auto b6{ b1 };
   BOOST_REQUIRE(memcmp(b6.data(), reinterpret_cast<const void*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b6.length() == char_length);
   BOOST_REQUIRE(b1 == b6);

   auto b7 = std::move(b1);
   BOOST_REQUIRE(memcmp(b7.data(), reinterpret_cast<const void*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b7.length() == char_length);

   auto b8{ std::move(b2) };
   BOOST_REQUIRE(memcmp(b8.data(), reinterpret_cast<const void*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b8.length() == char_length);
}

BOOST_AUTO_TEST_SUITE_END();