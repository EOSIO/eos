#include <b1/session/shared_bytes.hpp>
#include <boost/test/unit_test.hpp>

using namespace eosio::session;

BOOST_AUTO_TEST_SUITE(shared_bytes_tests)

BOOST_AUTO_TEST_CASE(make_shared_bytes_test) {
   static constexpr auto* char_value  = "hello world";
   static const auto      char_length = strlen(char_value) - 1;
   static constexpr auto  int_value   = int64_t{ 100000000 };

   auto b1     = shared_bytes(char_value, char_length);
   auto buffer = std::vector<shared_bytes::underlying_type_t>{ std::begin(b1), std::end(b1) };
   BOOST_REQUIRE(memcmp(buffer.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b1.size() == char_length);

   auto b2 = shared_bytes(reinterpret_cast<const int8_t*>(char_value), char_length);
   buffer  = std::vector<shared_bytes::underlying_type_t>{ std::begin(b2), std::end(b2) };
   BOOST_REQUIRE(memcmp(buffer.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b2.size() == char_length);

   auto b3 = shared_bytes(&int_value, 1);
   buffer  = std::vector<shared_bytes::underlying_type_t>{ std::begin(b3), std::end(b3) };
   BOOST_REQUIRE(*(reinterpret_cast<const decltype(int_value)*>(buffer.data())) == int_value);
   BOOST_REQUIRE(b3.size() == sizeof(decltype(int_value)));

   auto b4 = shared_bytes(char_value, char_length);
   buffer  = std::vector<shared_bytes::underlying_type_t>{ std::begin(b4), std::end(b4) };
   BOOST_REQUIRE(memcmp(buffer.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b4.size() == char_length);
   BOOST_REQUIRE(!b4.empty());

   auto invalid = shared_bytes(static_cast<int8_t*>(nullptr), 0);
   BOOST_REQUIRE(std::begin(invalid) == std::end(invalid));
   BOOST_REQUIRE(invalid.size() == 0);

   BOOST_REQUIRE(b1 == b2);
   BOOST_REQUIRE(b1 == b4);
   BOOST_REQUIRE(invalid == shared_bytes{});
   BOOST_REQUIRE(b1 != b3);

   auto b5 = b1;
   buffer  = std::vector<shared_bytes::underlying_type_t>{ std::begin(b5), std::end(b5) };
   BOOST_REQUIRE(memcmp(buffer.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b5.size() == char_length);
   BOOST_REQUIRE(b1 == b5);

   auto b6{ b1 };
   buffer = std::vector<shared_bytes::underlying_type_t>{ std::begin(b6), std::end(b6) };
   BOOST_REQUIRE(memcmp(buffer.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b6.size() == char_length);
   BOOST_REQUIRE(b1 == b6);

   auto b7 = std::move(b1);
   buffer  = std::vector<shared_bytes::underlying_type_t>{ std::begin(b7), std::end(b7) };
   BOOST_REQUIRE(memcmp(buffer.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b7.size() == char_length);

   auto b8{ std::move(b2) };
   buffer = std::vector<shared_bytes::underlying_type_t>{ std::begin(b8), std::end(b8) };
   BOOST_REQUIRE(memcmp(buffer.data(), reinterpret_cast<const int8_t*>(char_value), char_length) == 0);
   BOOST_REQUIRE(b8.size() == char_length);
}

BOOST_AUTO_TEST_CASE(iterator_test) {
   auto s      = std::string{ "Hello world foobar" };
   auto bytes  = eosio::session::shared_bytes{ s.data(), s.size() };
   auto result = std::string{ std::begin(bytes), std::end(bytes) };
   BOOST_REQUIRE(s == result);
}

BOOST_AUTO_TEST_CASE(std_copy) {
   auto bytes  = eosio::session::shared_bytes{ 18 };
   auto parts  = std::vector<std::string>{ "Hello ", "world ", "foobar" };
   auto offset = size_t{ 0 };
   for (const auto& part : parts) {
      std::copy(std::begin(part), std::end(part), std::begin(bytes) + offset);
      offset += part.size();
   }
   auto expected_result = std::string{ "Hello world foobar" };
   auto result          = std::string{ std::begin(bytes), std::end(bytes) };
   BOOST_REQUIRE(expected_result == result);

   result = "";
   for (const auto& ch : bytes) { result.push_back(ch); }
   BOOST_REQUIRE(expected_result == result);

   auto extracted_parts = std::vector<std::string>{};
   offset               = size_t{ 0 };
   for (const auto& part : parts) {
      extracted_parts.emplace_back(part.size(), '\0');
      std::copy(std::begin(bytes) + offset, std::begin(bytes) + offset + part.size(),
                std::begin(extracted_parts.back()));
      offset += part.size();
   }
   BOOST_REQUIRE(parts == extracted_parts);
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

   char test_value_1[3] = {static_cast<char>(0x00), static_cast<char>(0x00), static_cast<char>(0xFF)};
   char test_value_1_next_expected[2] = {static_cast<char>(0x00), static_cast<char>(0x01)};
   auto test_value_1_sb = shared_bytes(test_value_1, 3);
   BOOST_REQUIRE(test_value_1_sb.next().size() == 2);
   BOOST_REQUIRE(memcmp(test_value_1_sb.next().data(), reinterpret_cast<const int8_t*>(test_value_1_next_expected), 2) == 0);

   char test_value_2[3] = {static_cast<char>(0x00), static_cast<char>(0xFF), static_cast<char>(0xFF)};
   char test_value_2_next_expected[1] = {static_cast<char>(0x01)};
   auto test_value_2_sb = shared_bytes(test_value_2, 3);
   BOOST_REQUIRE(test_value_2_sb.next().size() == 1);
   BOOST_REQUIRE(memcmp(test_value_2_sb.next().data(), reinterpret_cast<const int8_t*>(test_value_2_next_expected), 1) == 0);

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
