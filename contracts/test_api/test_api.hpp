/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/serialize.hpp>
#include <string>

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
     CLASS::METHOD(); \
     return; \
  }

#define WASM_TEST_HANDLER_EX(CLASS, METHOD) \
  if( action == WASM_TEST_ACTION(#CLASS, #METHOD) ) { \
     CLASS::METHOD(receiver, code, action); \
     return; \
  }

#pragma pack(push, 1)
struct dummy_action {
   static uint64_t get_name() {
      return N(dummy_action);
   }
   static uint64_t get_account() {
      return N(testapi);
   }

  char a; //1
  uint64_t b; //8
  int32_t  c; //4

  EOSLIB_SERIALIZE( dummy_action, (a)(b)(c) )
};

struct u128_action {
  unsigned __int128  values[3]; //16*3
};

struct cf_action {
   static uint64_t get_name() {
      return N(cf_action);
   }
   static uint64_t get_account() {
      return N(testapi);
   }

   uint32_t       payload = 100;
   uint32_t       cfd_idx = 0; // context free data index

   EOSLIB_SERIALIZE( cf_action, (payload)(cfd_idx) )
};
#pragma pack(pop)

static_assert( sizeof(dummy_action) == 13 , "unexpected packing" );
static_assert( sizeof(u128_action) == 16*3 , "unexpected packing" );

struct test_types {
  static void types_size();
  static void char_to_symbol();
  static void string_to_name();
  static void name_class();
};

struct test_print {
  static void test_prints();
  static void test_prints_l();
  static void test_printi();
  static void test_printui();
  static void test_printi128();
  static void test_printn();
};

#define DUMMY_ACTION_DEFAULT_A 0x45
#define DUMMY_ACTION_DEFAULT_B 0xab11cd1244556677
#define DUMMY_ACTION_DEFAULT_C 0x7451ae12

struct test_action {

  static void read_action_normal();
  static void read_action_to_0();
  static void read_action_to_64k();
  static void test_dummy_action();
  static void test_cf_action();
  static void require_notice(uint64_t receiver, uint64_t code, uint64_t action);
  static void require_auth();
  static void assert_false();
  static void assert_true();
  static void now();
  static void test_abort() __attribute__ ((noreturn)) ;
  static void test_current_receiver(uint64_t receiver, uint64_t code, uint64_t action);
  static void test_current_sender();
  static void test_publication_time();
};

struct test_math {
  static void test_multeq();
  static void test_diveq();
  static void test_diveq_by_0();
  static void test_double_api();
  static void test_double_api_div_0();
  static void test_i64_to_double();
  static void test_double_to_i64();
};

struct test_db {
   static void primary_i64_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void primary_i64_lowerbound(uint64_t receiver, uint64_t code, uint64_t action);
   static void primary_i64_upperbound(uint64_t receiver, uint64_t code, uint64_t action);

   static void idx64_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_lowerbound(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_upperbound(uint64_t receiver, uint64_t code, uint64_t action);
};

struct test_multi_index {

   static void idx64_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_store_only(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_check_without_storing(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_store_only(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_check_without_storing(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_autoincrement_test(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_autoincrement_test_part1(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_autoincrement_test_part2(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx256_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx_double_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pk_iterator_exceed_end(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_sk_iterator_exceed_end(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pk_iterator_exceed_begin(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_sk_iterator_exceed_begin(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pass_pk_ref_to_other_table(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pass_sk_ref_to_other_table(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pass_pk_end_itr_to_iterator_to(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pass_pk_end_itr_to_modify(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pass_pk_end_itr_to_erase(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pass_sk_end_itr_to_iterator_to(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pass_sk_end_itr_to_modify(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pass_sk_end_itr_to_erase(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_modify_primary_key(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_run_out_of_avl_pk(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_sk_cache_pk_lookup(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_pk_cache_sk_lookup(uint64_t receiver, uint64_t code, uint64_t action);

};

struct test_crypto {
   static void test_recover_key();
   static void test_recover_key_assert_true();
   static void test_recover_key_assert_false();
   static void test_sha1();
   static void test_sha256();
   static void test_sha512();
   static void test_ripemd160();
   static void sha1_no_data();
   static void sha256_no_data();
   static void sha512_no_data();
   static void ripemd160_no_data();
   static void sha256_null();
   static void assert_sha256_false();
   static void assert_sha1_false();
   static void assert_sha512_false();
   static void assert_ripemd160_false();
   static void assert_sha256_true();
   static void assert_sha1_true();
   static void assert_sha512_true();
   static void assert_ripemd160_true();
};

struct test_transaction {
  static void test_tapos_block_num();
  static void test_tapos_block_prefix();
  static void send_action();
  static void send_action_empty();
  static void send_action_max();
  static void send_action_large();
  static void send_action_recurse();
  static void send_action_inline_fail();
  static void test_read_transaction();
  static void test_transaction_size();
  static void send_transaction();
  static void send_transaction_empty();
  static void send_transaction_max();
  static void send_transaction_large();
  static void send_action_sender();
  static void deferred_print();
  static void send_deferred_transaction();
  static void cancel_deferred_transaction();
  static void send_cf_action();
  static void send_cf_action_fail();
  static void read_inline_action();
  static void read_inline_cf_action();
};

struct test_chain {
  static void test_activeprods();
};

struct test_fixedpoint {
   static void create_instances();
   static void test_addition();
   static void test_subtraction();
   static void test_multiplication();
   static void test_division();
   static void test_division_by_0();
};

struct test_real {
   static void create_instances();
   static void test_addition();
   static void test_multiplication();
   static void test_division();
   static void test_division_by_0();
};

struct test_compiler_builtins {
   static void test_multi3();
   static void test_divti3();
   static void test_divti3_by_0();
   static void test_udivti3();
   static void test_udivti3_by_0();
   static void test_modti3();
   static void test_modti3_by_0();
   static void test_umodti3();
   static void test_umodti3_by_0();
   static void test_lshlti3();
   static void test_lshrti3();
   static void test_ashlti3();
   static void test_ashrti3();
};

struct test_extended_memory {
	static void test_initial_buffer();
	static void test_page_memory();
	static void test_page_memory_exceeded();
	static void test_page_memory_negative_bytes();
};

struct test_memory {
   static void test_memory_allocs();
   static void test_memory_hunk();
   static void test_memory_hunks();
   static void test_memory_hunks_disjoint();
   static void test_memset_memcpy();
   static void test_memcpy_overlap_start();
   static void test_memcpy_overlap_end();
   static void test_memcmp();
};

struct test_checktime {
   static void checktime_pass();
   static void checktime_failure();
};
/*
struct test_softfloat {
   static void test_f32_add();
   static void test_f32_sub();
   static void test_f32_mul();
   static void test_f32_div();
   static void test_f32_min();
};
*/

struct test_permission {
  static void check_authorization(uint64_t receiver, uint64_t code, uint64_t action);
};
