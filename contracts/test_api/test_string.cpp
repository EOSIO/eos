#include <eoslib/string.hpp>
#include <eoslib/eos.hpp>

#include "test_api.hpp"

unsigned int test_string::construct_with_size() {

   uint32_t size = 100;
   eos::string str(size);

   WASM_ASSERT( str.get_size() == size,  "str.get_size() == size" );

   return WASM_TEST_PASS;
}


unsigned int test_string::construct_with_data() {
  char data[] = "abcdefghij";
  uint32_t size = sizeof(data)/sizeof(char);

  eos::string str(data, size, false);

  WASM_ASSERT( str.get_size() == size,  "str.get_size() == size" );
  WASM_ASSERT( str.get_data() == data,  "str.get_data() == data" );
  WASM_ASSERT( str.is_own_memory() == false,  "str.is_own_memory() == false" );

  return WASM_TEST_PASS;
}

unsigned int test_string::construct_with_data_partially() {
  char data[] = "abcdefghij";
  uint32_t substr_size = 5;
  uint32_t offset = 2;

  eos::string str(data + offset, substr_size, false);

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
  uint32_t size = sizeof(data)/sizeof(char);

  eos::string str(data, size, true);

  WASM_ASSERT( str.get_size() == size,  "str.get_size() == size" );
  WASM_ASSERT( str.get_data() != data,  "str.get_data() != data" );
  for (uint8_t i = 0; i < size; i++) {
    WASM_ASSERT( str[i] == data[i],  "str[i] == data[i]" );
  }
  WASM_ASSERT( str.is_own_memory() == true,  "str.is_own_memory() == true" );

  return WASM_TEST_PASS;
}

unsigned int test_string::copy_constructor() {
  char data[] = "abcdefghij";
  uint32_t size = sizeof(data)/sizeof(char);

  eos::string str1(data, size, true);

  eos::string str2 = str1;

  WASM_ASSERT( str1.get_size() == str2.get_size(),  "str1.get_size() == str2.get_size()" );
  WASM_ASSERT( str1.get_data() == str2.get_data(),  "str1.get_data() == str2.getget_data_size()" );
  WASM_ASSERT( str1.is_own_memory() == str2.is_own_memory(),  "str1.is_own_memory() == str2.is_own_memory()" );
  WASM_ASSERT( str1.get_refcount() == str2.get_refcount(),  "str1.get_refcount() == str2.get_refcount()" );
  WASM_ASSERT( str1.get_refcount() == 2,  "str1.refcount() == 2" );

  return WASM_TEST_PASS;
}

unsigned int test_string::assignment_operator() {
  char data[] = "abcdefghij";
  uint32_t size = sizeof(data)/sizeof(char);

  eos::string str1(data, size, true);

  eos::string str2;
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
  uint32_t size = sizeof(data)/sizeof(char);

  eos::string str(data, size, false);

  for (uint8_t i = 0; i < size; i++) {
    WASM_ASSERT( str[i] == data[i],  "str[i] == data[i]" );
  }

  return WASM_TEST_PASS;
}

unsigned int test_string::index_out_of_bound() {
  char data[] = "abcdefghij";
  uint32_t size = sizeof(data)/sizeof(char);

  eos::string str(data, size, false);
  char c = str[size];

  return WASM_TEST_PASS;
}

unsigned int test_string::substring() {
  char data[] = "abcdefghij";
  uint32_t size = sizeof(data)/sizeof(char);

  eos::string str(data, size, false);

  uint32_t substr_size = 5;
  uint32_t offset = 2;
  eos::string substr = str.substr(offset, substr_size, false);

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
  uint32_t size = sizeof(data)/sizeof(char);

  eos::string str(data, size, false);

  uint32_t substr_size = size;
  uint32_t offset = 1;
  eos::string substr = str.substr(offset, substr_size, false);

  return WASM_TEST_PASS;
}

unsigned int test_string::concatenation_null_terminated() {
  char data1[] = "abcdefghij";
  uint32_t size1 = sizeof(data1)/sizeof(char);
  eos::string str1(data1, size1, false);

  char data2[] = "klmnoppqrst";
  uint32_t size2 = sizeof(data2)/sizeof(char);
  eos::string str2(data2, size2, false);

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
  uint32_t size1 = sizeof(data1)/sizeof(char);
  eos::string str1(data1, size1, false);

  char data2[] = {'k','l','m','n','o','p','q','r','s','t'};
  uint32_t size2 = sizeof(data2)/sizeof(char);
  eos::string str2(data2, size2, false);

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
  uint32_t size = sizeof(data)/sizeof(char);

  eos::string str(100);
  str.assign(data, size, true);

  WASM_ASSERT( str.get_size() == size,  "str.get_size() == size" );
  WASM_ASSERT( str.get_data() != data,  "str.get_data() != data" );
  for (uint8_t i = 0; i < size; i++) {
    WASM_ASSERT( str[i] == data[i],  "str[i] == data[i]" );
  }
  WASM_ASSERT( str.is_own_memory() == true,  "str.is_own_memory() == true" );

  return WASM_TEST_PASS;
}


unsigned int test_string::comparison_operator() {
  char data1[] = "abcdefghij";
  uint32_t size1 = sizeof(data1)/sizeof(char);
  eos::string str1(data1, size1, false);

  char data2[] = "abcdefghij";
  uint32_t size2 = sizeof(data2)/sizeof(char);
  eos::string str2(data2, size2, false);

  char data3[] = "klmno";
  uint32_t size3 = sizeof(data3)/sizeof(char);
  eos::string str3(data3, size3, false);

  char data4[] = "aaaaaaaaaaaaaaa";
  uint32_t size4 = sizeof(data4)/sizeof(char);
  eos::string str4(data4, size4, false);

  WASM_ASSERT( str1 == str2,  "str1 == str2" );
  WASM_ASSERT( str1 != str3,  "str1 != str3" );
  WASM_ASSERT( str1 < str3,  "str1 < str3" );
  WASM_ASSERT( str2 > str4,  "str2 > str4" );

  return WASM_TEST_PASS;
}

unsigned int test_string::print_null_terminated() {
  char data[] = "Hello World!";
  uint32_t size = sizeof(data)/sizeof(char);
  eos::string str(data, size, false);

  eos::print(str);

  return WASM_TEST_PASS;
}

unsigned int test_string::print_non_null_terminated() {
  char data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
  uint32_t size = sizeof(data)/sizeof(char);
  eos::string str(data, size, false);

  eos::print(str);

  return WASM_TEST_PASS;
}

unsigned int test_string::print_unicode() {
  char data[] = "你好，世界！";
  uint32_t size = sizeof(data)/sizeof(char);
  eos::string str(data, size, false);

  eos::print(str);

  return WASM_TEST_PASS;
}
