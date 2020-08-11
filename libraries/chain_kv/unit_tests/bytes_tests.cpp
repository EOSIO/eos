#include <boost/test/unit_test.hpp>
#include <b1/session/boost_memory_allocator.hpp>
#include <b1/session/bytes.hpp>

using namespace b1::session;

BOOST_AUTO_TEST_SUITE(bytes_tests)

BOOST_AUTO_TEST_CASE(make_bytes_test) 
{
  static constexpr auto* char_value = "hello world";
  static constexpr auto int_value = int64_t{100000000};

  auto allocator = boost_memory_allocator::make();

  auto b1 = make_bytes(char_value, strlen(char_value), allocator);
  BOOST_REQUIRE(strcmp(reinterpret_cast<const char*>(b1.data()), char_value) == 0);
  BOOST_REQUIRE(b1.length() == strlen(char_value));

  auto b2 = make_bytes(reinterpret_cast<const void*>(char_value), strlen(char_value), allocator);
  BOOST_REQUIRE(strcmp(reinterpret_cast<const char*>(b2.data()), char_value) == 0);
  BOOST_REQUIRE(b2.length() == strlen(char_value));

  auto b3 = make_bytes(&int_value, sizeof(decltype(int_value)), allocator);
  BOOST_REQUIRE(*(reinterpret_cast<const decltype(int_value)*>(b3.data())) == int_value);
  BOOST_REQUIRE(b3.length() == sizeof(decltype(int_value)));

  auto b4 = make_bytes(char_value, strlen(char_value));
  BOOST_REQUIRE(strcmp(reinterpret_cast<const char*>(b4.data()), char_value) == 0);
  BOOST_REQUIRE(b4.length() == strlen(char_value));

  auto invalid = make_bytes(static_cast<void*>(nullptr), 0, allocator);
  BOOST_REQUIRE(invalid.data() == nullptr);
  BOOST_REQUIRE(b4.length() == 0);

  BOOST_REQUIRE(b1 == b2);
  BOOST_REQUIRE(b1 == b4);
  BOOST_REQUIRE(invalid == bytes::invalid);
  BOOST_REQUIRE(b1 != b3);

  auto b5 = b1;
  BOOST_REQUIRE(strcmp(reinterpret_cast<const char*>(b5.data()), char_value) == 0);
  BOOST_REQUIRE(b5.length() == strlen(char_value));

  auto b6{b1};
  BOOST_REQUIRE(strcmp(reinterpret_cast<const char*>(b6.data()), char_value) == 0);
  BOOST_REQUIRE(b6.length() == strlen(char_value));

  auto b7 = std::move(b1);
  BOOST_REQUIRE(strcmp(reinterpret_cast<const char*>(b7.data()), char_value) == 0);
  BOOST_REQUIRE(b7.length() == strlen(char_value));

  auto b8{std::move(b2)};
  BOOST_REQUIRE(strcmp(reinterpret_cast<const char*>(b8.data()), char_value) == 0);
  BOOST_REQUIRE(b8.length() == strlen(char_value));
}

BOOST_AUTO_TEST_SUITE_END();