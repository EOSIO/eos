/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#define WASM_TEST_ERROR_CODE *((unsigned int *)((1<<16) - 2*sizeof(unsigned int)))
#define WASM_TEST_ERROR_MESSAGE *((unsigned int *)((1<<16) - 1*sizeof(unsigned int)))

#define WASM_TEST_FAIL 0xDEADBEEF
#define WASM_TEST_PASS 0xB0CABACA

#define WASM_ASSERT(m, message) if(!(m)) { WASM_TEST_ERROR_MESSAGE=(unsigned int)message; return WASM_TEST_FAIL; }

typedef unsigned long long u64;
typedef unsigned int u32;
static constexpr u32 DJBH(const char* cp)
{
  u32 hash = 5381;
  while (*cp)
      hash = 33 * hash ^ (unsigned char) *cp++;
  return hash;
}

static constexpr u64 WASM_TEST_ACTION(const char* cls, const char* method)
{
  return u64(DJBH(cls)) << 32 | u64(DJBH(method));
}

#define WASM_TEST_HANDLER(CLASS, METHOD) \
  if( action == WASM_TEST_ACTION(#CLASS, #METHOD) ) { \
     WASM_TEST_ERROR_CODE = CLASS::METHOD(); \
     return; \
  }

#pragma pack(push, 1)
struct dummy_message {
  char a; //1
  unsigned long long b; //8
  int  c; //4
};

struct u128_msg {
  unsigned __int128  values[3]; //16*3
};
#pragma pack(pop)

static_assert( sizeof(dummy_message) == 13 , "unexpected packing" );
static_assert( sizeof(u128_msg) == 16*3 , "unexpected packing" );

struct test_types {
  static unsigned int types_size();
  static unsigned int char_to_symbol();
  static unsigned int string_to_name();
  static unsigned int name_class();
};

struct test_print {
  static unsigned int test_prints();
  static unsigned int test_printi();
  static unsigned int test_printi128();
  static unsigned int test_printn();
};

#define DUMMY_MESSAGE_DEFAULT_A 0x45
#define DUMMY_MESSAGE_DEFAULT_B 0xab11cd1244556677
#define DUMMY_MESSAGE_DEFAULT_C 0x7451ae12

struct test_message {

  static unsigned int read_message_normal();
  static unsigned int read_message_to_0();
  static unsigned int read_message_to_64k();
  static unsigned int require_notice();
  static unsigned int require_auth();
  static unsigned int assert_false();
  static unsigned int assert_true();
  static unsigned int now();

};

struct test_math {
  static unsigned int test_multeq_i128();
  static unsigned int test_diveq_i128();
  static unsigned int test_diveq_i128_by_0();
  static unsigned int test_double_api();
  static unsigned int test_double_api_div_0();
};

struct test_db {
   static unsigned int key_i64_general();
   static unsigned int key_i64_remove_all();
   static unsigned int key_i64_small_load();
   static unsigned int key_i64_small_store();
   static unsigned int key_i64_store_scope();
   static unsigned int key_i64_remove_scope();
   static unsigned int key_i64_not_found();
   static unsigned int key_i64_front_back();

   static unsigned int key_i128i128_general();
   static unsigned int key_i64i64i64_general();
   static unsigned int key_str_general();
   static unsigned int key_str_table();

   static unsigned int key_str_setup_limit();
   static unsigned int key_str_min_exceed_limit();
   static unsigned int key_str_under_limit();
   static unsigned int key_str_available_space_exceed_limit();
   static unsigned int key_str_another_under_limit();

   static unsigned int key_i64_setup_limit();
   static unsigned int key_i64_min_exceed_limit();
   static unsigned int key_i64_under_limit();
   static unsigned int key_i64_available_space_exceed_limit();
   static unsigned int key_i64_another_under_limit();

   static unsigned int key_i128i128_setup_limit();
   static unsigned int key_i128i128_min_exceed_limit();
   static unsigned int key_i128i128_under_limit();
   static unsigned int key_i128i128_available_space_exceed_limit();
   static unsigned int key_i128i128_another_under_limit();

   static unsigned int key_i64i64i64_setup_limit();
   static unsigned int key_i64i64i64_min_exceed_limit();
   static unsigned int key_i64i64i64_under_limit();
   static unsigned int key_i64i64i64_available_space_exceed_limit();
   static unsigned int key_i64i64i64_another_under_limit();
};

struct test_crypto {
   static unsigned int test_sha256();
   static unsigned int sha256_no_data();
   static unsigned int asert_sha256_false();
   static unsigned int asert_sha256_true();
   static unsigned int asert_no_data();
};

struct test_transaction {

  static unsigned int send_message();
  static unsigned int send_message_empty();
  static unsigned int send_message_max();
  static unsigned int send_message_large();
  static unsigned int send_message_recurse();
  static unsigned int send_message_inline_fail();
  static unsigned int send_transaction();
  static unsigned int send_transaction_empty();
  static unsigned int send_transaction_max();
  static unsigned int send_transaction_large();
};

struct test_chain {
  static unsigned int test_activeprods();
};

struct test_string {
  static unsigned int construct_with_size();
  static unsigned int construct_with_data();
  static unsigned int construct_with_data_copied();
  static unsigned int construct_with_data_partially();
  static unsigned int copy_constructor();
  static unsigned int assignment_operator();
  static unsigned int index_operator();
  static unsigned int index_out_of_bound();
  static unsigned int substring();
  static unsigned int substring_out_of_bound();
  static unsigned int concatenation_null_terminated();
  static unsigned int concatenation_non_null_terminated();
  static unsigned int assign();
  static unsigned int comparison_operator();
  static unsigned int print_null_terminated();
  static unsigned int print_non_null_terminated();
  static unsigned int print_unicode();
  static unsigned int valid_utf8();
  static unsigned int invalid_utf8();
  static unsigned int string_literal();
};

struct test_fixedpoint {
   static unsigned int create_instances();
   static unsigned int test_addition();
   static unsigned int test_subtraction();
   static unsigned int test_multiplication();
   static unsigned int test_division();
};

struct test_real {
   static unsigned int create_instances();
   static unsigned int test_addition();
   static unsigned int test_multiplication();
   static unsigned int test_division();
};

struct test_account {
   static unsigned int test_balance_acc1();
};
