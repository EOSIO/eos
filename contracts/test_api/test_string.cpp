#include <eosiolib/string.hpp>
#include <eosiolib/eos.hpp>

#include "test_api.hpp"

unsigned int test_string::construct_with_size() {

   size_t size1 = 100;
   eosio::string str1(size1);

   WASM_ASSERT( str1.get_size() == size1,  "str1.get_size() == size1" );
   WASM_ASSERT( str1.is_own_memory() == true,  "str1.is_own_memory() == true" );

   size_t size2 = 0;
   eosio::string str2(size2);

   WASM_ASSERT( str2.get_size() == size2,  "str2.get_size() == size2" );
   WASM_ASSERT( str2.get_data() == nullptr,  "str2.get_data() == nullptr" );
   WASM_ASSERT( str2.is_own_memory() == false,  "str2.is_own_memory() == false" );

   return WASM_TEST_PASS;
}


unsigned int test_string::construct_with_data() {
  char data[] = "abcdefghij";
  size_t size = sizeof(data)/sizeof(char);

  eosio::string str1(data, size, false);

  WASM_ASSERT( str1.get_size() == size,  "str1.get_size() == size" );
  WASM_ASSERT( str1.get_data() == data,  "str1.get_data() == data" );
  WASM_ASSERT( str1.is_own_memory() == false,  "str1.is_own_memory() == false" );

  eosio::string str2(data, 0, false);

  WASM_ASSERT( str2.get_size() == 0,  "str2.get_size() == 0" );
  WASM_ASSERT( str2.get_data() == nullptr,  "str2.get_data() == nullptr" );
  WASM_ASSERT( str2.is_own_memory() == false,  "str2.is_own_memory() == false" );

  return WASM_TEST_PASS;
}

unsigned int test_string::construct_with_data_partially() {
  char data[] = "abcdefghij";
  size_t substr_size = 5;
  size_t offset = 2;

  eosio::string str(data + offset, substr_size, false);

  WASM_ASSERT( str.get_size() == substr_size,  "str.get_size() == substr_size" );
  WASM_ASSERT( str.get_data() == data + offset,  "str.get_data() == data + offset" );
  for (uint8_t i = offset; i < substr_size; i++) {
    WASM_ASSERT( str[i] == data[offset + i],  "str[i] == data[offset + i]" );
  }
  WASM_ASSERT( str.is_own_memory() == false,  "str.is_own_memory() == false" );

  return WASM_TEST_PASS;
}

unsigned int test_string::construct_with_data_copied() {
  char data[] = "abcdefghij";
  size_t size = sizeof(data)/sizeof(char);

  eosio::string str1(data, size, true);

  WASM_ASSERT( str1.get_size() == size,  "str1.get_size() == size" );
  WASM_ASSERT( str1.get_data() != data,  "str1.get_data() != data" );
  for (uint8_t i = 0; i < size; i++) {
    WASM_ASSERT( str1[i] == data[i],  "str1[i] == data[i]" );
  }
  WASM_ASSERT( str1.is_own_memory() == true,  "str.is_own_memory() == true" );

  eosio::string str2(data, 0, true);
  
  WASM_ASSERT( str2.get_size() == 0,  "str2.get_size() == 0" );
  WASM_ASSERT( str2.get_data() == nullptr,  "str2.get_data() == nullptr" );
  WASM_ASSERT( str2.is_own_memory() == false,  "str2.is_own_memory() == false" );

  return WASM_TEST_PASS;
}

unsigned int test_string::copy_constructor() {
  char data[] = "abcdefghij";
  size_t size = sizeof(data)/sizeof(char);

  eosio::string str1(data, size, true);

  eosio::string str2 = str1;

  WASM_ASSERT( str1.get_size() == str2.get_size(),  "str1.get_size() == str2.get_size()" );
  WASM_ASSERT( str1.get_data() == str2.get_data(),  "str1.get_data() == str2.getget_data_size()" );
  WASM_ASSERT( str1.is_own_memory() == str2.is_own_memory(),  "str1.is_own_memory() == str2.is_own_memory()" );
  WASM_ASSERT( str1.get_refcount() == str2.get_refcount(),  "str1.get_refcount() == str2.get_refcount()" );
  WASM_ASSERT( str1.get_refcount() == 2,  "str1.refcount() == 2" );

  return WASM_TEST_PASS;
}

unsigned int test_string::assignment_operator() {
  char data[] = "abcdefghij";
  size_t size = sizeof(data)/sizeof(char);

  eosio::string str1(data, size, true);

  eosio::string str2;
  str2 = str1;

  WASM_ASSERT( str1.get_size() == str2.get_size(),  "str1.get_size() == str2.get_size()" );
  WASM_ASSERT( str1.get_data() == str2.get_data(),  "str1.get_data() == str2.getget_data_size()" );
  WASM_ASSERT( str1.is_own_memory() == str2.is_own_memory(),  "str1.is_own_memory() == str2.is_own_memory()" );
  WASM_ASSERT( str1.get_refcount() == str2.get_refcount(),  "str1.get_refcount() == str2.get_refcount()" );
  WASM_ASSERT( str1.get_refcount() == 2,  "str1.refcount() == 2" );

  return WASM_TEST_PASS;
}

unsigned int test_string::index_operator() {
  char data[] = "abcdefghij";
  size_t size = sizeof(data)/sizeof(char);

  eosio::string str(data, size, false);

  for (uint8_t i = 0; i < size; i++) {
    WASM_ASSERT( str[i] == data[i],  "str[i] == data[i]" );
  }

  return WASM_TEST_PASS;
}

unsigned int test_string::index_out_of_bound() {
  char data[] = "abcdefghij";
  size_t size = sizeof(data)/sizeof(char);

  eosio::string str(data, size, false);
  char c = str[size];

  return WASM_TEST_PASS;
}

unsigned int test_string::substring() {
  char data[] = "abcdefghij";
  size_t size = sizeof(data)/sizeof(char);

  eosio::string str(data, size, false);

  size_t substr_size = 5;
  size_t offset = 2;
  eosio::string substr = str.substr(offset, substr_size, false);

  WASM_ASSERT( substr.get_size() == substr_size,  "str.get_size() == substr_size" );
  WASM_ASSERT( substr.get_data() == str.get_data() + offset,  "substr.get_data() == str.get_data() + offset" );
  for (uint8_t i = offset; i < substr_size; i++) {
    WASM_ASSERT( substr[i] == str[offset + i],  "substr[i] == str[offset + i]" );
  }
  WASM_ASSERT( substr.is_own_memory() == false,  "substr.is_own_memory() == false" );

  return WASM_TEST_PASS;
}

unsigned int test_string::substring_out_of_bound() {
  char data[] = "abcdefghij";
  size_t size = sizeof(data)/sizeof(char);

  eosio::string str(data, size, false);

  size_t substr_size = size;
  size_t offset = 1;
  eosio::string substr = str.substr(offset, substr_size, false);

  return WASM_TEST_PASS;
}

unsigned int test_string::concatenation_null_terminated() {
  char data1[] = "abcdefghij";

  size_t size1 = sizeof(data1)/sizeof(char);
  eosio::string str1(data1, size1, false);

  char data2[] = "klmnoppqrst";
  size_t size2 = sizeof(data2)/sizeof(char);
  eosio::string str2(data2, size2, false);

  str1 += str2;

  WASM_ASSERT( str1.get_data() != data1,  "str1.get_data() != data1" );
  WASM_ASSERT( str1.get_size() == size1 + size2 - 1,  "str1.get_size == size1 + size2 - 1" );
  for (uint8_t i = 0; i < size1 - 1; i++) {
    WASM_ASSERT( str1[i] == data1[i],  "str1[i] == data1[i]" );
  }
  for (uint8_t i = 0; i < size2; i++) {
    WASM_ASSERT( str1[size1 - 1 + i] == data2[i],  "str1[i] == data2[i]" );
  }

  return WASM_TEST_PASS;
}

unsigned int test_string::concatenation_non_null_terminated() {
  char data1[] = {'a','b','c','d','e','f','g','h','i','j'};

  size_t size1 = sizeof(data1)/sizeof(char);
  eosio::string str1(data1, size1, false);

  char data2[] = {'k','l','m','n','o','p','q','r','s','t'};
  size_t size2 = sizeof(data2)/sizeof(char);
  eosio::string str2(data2, size2, false);

  str1 += str2;

  WASM_ASSERT( str1.get_data() != data1,  "str1.get_data() != data1" );
  WASM_ASSERT( str1.get_size() == size1 + size2,  "str1.get_size == size1 + size2" );
  for (uint8_t i = 0; i < size1; i++) {
    WASM_ASSERT( str1[i] == data1[i],  "str1[i] == data1[i]" );
  }
  for (uint8_t i = 0; i < size2; i++) {
    WASM_ASSERT( str1[size1 + i] == data2[i],  "str1[i] == data2[i]" );
  }

  return WASM_TEST_PASS;
}

unsigned int test_string::assign() {
  char data[] = "abcdefghij";
  size_t size = sizeof(data)/sizeof(char);

  eosio::string str(100);
  str.assign(data, size, true);

  WASM_ASSERT( str.get_size() == size,  "str.get_size() == size" );
  WASM_ASSERT( str.get_data() != data,  "str.get_data() != data" );
  for (uint8_t i = 0; i < size; i++) {
    WASM_ASSERT( str[i] == data[i],  "str[i] == data[i]" );
  }
  WASM_ASSERT( str.is_own_memory() == true,  "str.is_own_memory() == true" );

  str.assign(data, 0, true);

  WASM_ASSERT( str.get_size() == 0,  "str.get_size() == 0" );
  WASM_ASSERT( str.get_data() == nullptr,  "str.get_data() == nullptr" );
  WASM_ASSERT( str.is_own_memory() == false,  "str.is_own_memory() == false" );

  return WASM_TEST_PASS;
}


unsigned int test_string::comparison_operator() {
  char data1[] = "abcdefghij";
  size_t size1 = sizeof(data1)/sizeof(char);
  eosio::string str1(data1, size1, false);

  char data2[] = "abcdefghij";
  size_t size2 = sizeof(data2)/sizeof(char);
  eosio::string str2(data2, size2, false);

  char data3[] = "klmno";
  size_t size3 = sizeof(data3)/sizeof(char);
  eosio::string str3(data3, size3, false);

  char data4[] = "aaaaaaaaaaaaaaa";
  size_t size4 = sizeof(data4)/sizeof(char);
  eosio::string str4(data4, size4, false);

  char data5[] = "你好";
  size_t size5 = sizeof(data5)/sizeof(char);
  eosio::string str5(data5, size5, false);

  char data6[] = "你好嗎？";
  size_t size6 = sizeof(data6)/sizeof(char);
  eosio::string str6(data6, size6, false);

  char data7[] = {'a', 'b', 'c', 'd', 'e'};
  size_t size7 = sizeof(data7)/sizeof(char);
  eosio::string str7(data7, size7, false);

  char data8[] = {'a', 'b', 'c'};
  size_t size8 = sizeof(data8)/sizeof(char);
  eosio::string str8(data8, size8, false);

  WASM_ASSERT( str1 == str2,  "str1 == str2" );
  WASM_ASSERT( str1 != str3,  "str1 != str3" );
  WASM_ASSERT( str1 < str3,  "str1 < str3" );
  WASM_ASSERT( str2 > str4,  "str2 > str4" );
  WASM_ASSERT( str1.compare(str2) == 0,  "str1.compare(str2) == 0" );
  WASM_ASSERT( str1.compare(str3) < 0,  "str1.compare(str3) < 0" );
  WASM_ASSERT( str1.compare(str4) > 0,  "str1.compare(str4) > 0" );
  WASM_ASSERT( str5.compare(str6) < 0,  "st5.compare(str6) < 0" );
  WASM_ASSERT( str7.compare(str8) > 0,  "str7.compare(str8) > 0" );

  return WASM_TEST_PASS;
}

unsigned int test_string::print_null_terminated() {
  char data[] = "Hello World!";

  size_t size = sizeof(data)/sizeof(char);
  eosio::string str(data, size, false);

  eosio::print(str);

  return WASM_TEST_PASS;
}

unsigned int test_string::print_non_null_terminated() {
  char data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};

  size_t size = sizeof(data)/sizeof(char);
  eosio::string str(data, size, false);

  eosio::print(str);

  return WASM_TEST_PASS;
}

unsigned int test_string::print_unicode() {
  char data[] = "你好，世界！";

  size_t size = sizeof(data)/sizeof(char);
  eosio::string str(data, size, false);

  eosio::print(str);

  return WASM_TEST_PASS;
}

unsigned int test_string::valid_utf8() {
  // Roman alphabet is 1 byte UTF-8
  char data[] = "abcdefghij";
  uint32_t size = sizeof(data)/sizeof(char);
  eosio::string str(data, size, false);
  assert_is_utf8(str.get_data(), str.get_size(), "the string should be a valid utf8 string");

  // Greek character is 2 bytes UTF-8
  char greek_str_data[] = "γειά σου κόσμος";
  uint32_t greek_str_size = sizeof(greek_str_data)/sizeof(char);
  eosio::string valid_greek_str(greek_str_data, greek_str_size, false);
  assert_is_utf8(valid_greek_str.get_data(), valid_greek_str.get_size(), "the string should be a valid utf8 string");

  // Common chinese character is 3 bytes UTF-8
  char chinese_str_data1[] = "你好，世界！";
  uint32_t chinese_str_size1 = sizeof(chinese_str_data1)/sizeof(char);
  eosio::string valid_chinese_str1(chinese_str_data1, chinese_str_size1, false);
  assert_is_utf8(valid_chinese_str1.get_data(), valid_chinese_str1.get_size(), "the string should be a valid utf8 string");

  // The following chinese character is 4 bytes UTF-8
  char chinese_str_data2[] = "𥄫";
  uint32_t chinese_str_size2 = sizeof(chinese_str_data2)/sizeof(char);
  eosio::string valid_chinese_str2(chinese_str_data2, chinese_str_size2, false);
  assert_is_utf8(valid_chinese_str2.get_data(), valid_chinese_str2.get_size(), "the string should be a valid utf8 string");

  return WASM_TEST_PASS;
}

unsigned int test_string::invalid_utf8() {
  // The following common chinese character is 3 bytes UTF-8
  char chinese_str_data[] = "你好，世界！";
  uint32_t chinese_str_size = sizeof(chinese_str_data)/sizeof(char);
  // If it is not multiple of 3, then it is invalid
  eosio::string invalid_chinese_str(chinese_str_data, 5, false);
  assert_is_utf8(invalid_chinese_str.get_data(), invalid_chinese_str.get_size(), "the string should be a valid utf8 string");

  return WASM_TEST_PASS;
}


unsigned int test_string::string_literal() {
  // Construct
  char data1[] = "abcdefghij";
  char data2[] = "klmnopqrstuvwxyz";

  eosio::string str = "abcdefghij";

  WASM_ASSERT( str.get_size() == 11,  "data1 str.get_size() == 11" );
  for (uint8_t i = 0; i < 11; i++) {
    WASM_ASSERT( str[i] == data1[i],  "data1 str[i] == data1[i]" );
  }
  WASM_ASSERT( str.is_own_memory() == true,  "data1 str.is_own_memory() == true" );

  str = "klmnopqrstuvwxyz";
  
  WASM_ASSERT( str.get_size() == 17,  "data2 str.get_size() == 17" );
  for (uint8_t i = 0; i < 17; i++) {
    WASM_ASSERT( str[i] == data2[i],  "data2 str[i] == data2[i]" );
  }
  WASM_ASSERT( str.is_own_memory() == true,  "data2 str.is_own_memory() == true" );

  return WASM_TEST_PASS;
}
