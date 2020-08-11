#include <boost/test/unit_test.hpp>
#include <b1/session/boost_memory_allocator.hpp>
#include <b1/session/key_value.hpp>

using namespace b1::session;

BOOST_AUTO_TEST_SUITE(key_value_tests)

BOOST_AUTO_TEST_CASE(make_key_value_test) 
{
  static constexpr auto* char_key = "hello";
  static constexpr auto* char_value = "world";
  static constexpr auto int_key = uint64_t{100000000};
  static constexpr auto int_value = uint64_t{123456789};

  static const auto char_key_bytes = make_bytes(char_key, strlen(char_key));
  static const auto char_value_bytes = make_bytes(char_value, strlen(char_value));
  static const auto int_key_bytes = make_bytes(&int_key, sizeof(int_key));
  static const auto int_value_bytes = make_bytes(&int_value, sizeof(int_value));

  auto allocator = boost_memory_allocator::make();

  auto kv1 = make_kv(make_bytes(char_key, strlen(char_key)), 
                     make_bytes(char_value, strlen(char_value)));
  BOOST_REQUIRE(kv1.key() == char_key_bytes);
  BOOST_REQUIRE(kv1.value() == char_value_bytes);

  auto kv2 = make_kv(make_bytes(&int_key, sizeof(int_key)), 
                     make_bytes(&int_value, sizeof(int_value)));
  BOOST_REQUIRE(kv2.key() == int_key_bytes);
  BOOST_REQUIRE(kv2.value() == int_value_bytes);

  auto kv3 = make_kv(make_bytes(char_key, strlen(char_key), allocator), 
                     make_bytes(char_value, strlen(char_value), allocator));
  BOOST_REQUIRE(kv3.key() == char_key_bytes);
  BOOST_REQUIRE(kv3.value() == char_value_bytes);

  auto kv4 = make_kv(make_bytes(&int_key, sizeof(int_key), allocator), 
                     make_bytes(&int_value, sizeof(int_value), allocator));
  BOOST_REQUIRE(kv4.key() == int_key_bytes);
  BOOST_REQUIRE(kv4.value() == int_value_bytes);

  auto kv5 = make_kv(char_key, strlen(char_key), char_value, strlen(char_value), allocator);
  BOOST_REQUIRE(kv5.key() == char_key_bytes);
  BOOST_REQUIRE(kv5.value() == char_value_bytes);

  auto kv6 = make_kv(&int_key, sizeof(int_key), &int_value, sizeof(int_value), allocator); 
  BOOST_REQUIRE(kv6.key() == int_key_bytes);
  BOOST_REQUIRE(kv6.value() == int_value_bytes);        

  auto kv7 = make_kv(reinterpret_cast<const void*>(char_key), strlen(char_key), 
                     reinterpret_cast<const void*>(char_value), strlen(char_value), allocator);
  BOOST_REQUIRE(kv7.key() == char_key_bytes);
  BOOST_REQUIRE(kv7.value() == char_value_bytes);

  auto kv8 = make_kv(reinterpret_cast<const void*>(&int_key), sizeof(int_key), 
                     reinterpret_cast<const void*>(&int_value), sizeof(int_value), allocator);  
  BOOST_REQUIRE(kv8.key() == int_key_bytes);
  BOOST_REQUIRE(kv8.value() == int_value_bytes);          

  auto kv9 = make_kv(reinterpret_cast<const void*>(char_key), strlen(char_key), 
                     reinterpret_cast<const void*>(char_value), strlen(char_value));
  BOOST_REQUIRE(kv9.key() == char_key_bytes);
  BOOST_REQUIRE(kv9.value() == char_value_bytes);

  auto kv10 = make_kv(reinterpret_cast<const void*>(&int_key), sizeof(int_key), 
                      reinterpret_cast<const void*>(&int_value), sizeof(int_value));  
  BOOST_REQUIRE(kv10.key() == int_key_bytes);
  BOOST_REQUIRE(kv10.value() == int_value_bytes);      

  BOOST_REQUIRE(kv1 == kv3);    
  BOOST_REQUIRE(kv1 == kv5);
  BOOST_REQUIRE(kv1 == kv7);
  BOOST_REQUIRE(kv1 == kv9);
  BOOST_REQUIRE(kv2 == kv4);
  BOOST_REQUIRE(kv2 == kv6);
  BOOST_REQUIRE(kv2 == kv8);
  BOOST_REQUIRE(kv2 == kv10);
  BOOST_REQUIRE(kv1 != kv2);

  auto invalid = make_kv<void*, void*>(nullptr, 0, nullptr, 0);
  BOOST_REQUIRE(key_value::invalid == invalid);
}

BOOST_AUTO_TEST_SUITE_END();