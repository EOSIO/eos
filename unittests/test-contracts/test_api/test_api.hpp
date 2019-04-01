/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <string>

#include "test_api_common.hpp"

namespace eosio { class transaction; }

// NOTE: including eosiolib/transaction.hpp here causes !"unresolvable": env._ZNKSt3__120__vector_base_commonILb1EE20__throw_length_errorEv
//       errors in api_tests/memory_tests

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

#define WASM_TEST_ERROR_HANDLER(CALLED_CLASS_STR, CALLED_METHOD_STR, HANDLER_CLASS, HANDLER_METHOD) \
   if( error_action == name{WASM_TEST_ACTION(CALLED_CLASS_STR, CALLED_METHOD_STR)} ) { \
   HANDLER_CLASS::HANDLER_METHOD(error_trx); \
   return; \
}

struct test_types {
   static void types_size();
   static void char_to_symbol();
   static void string_to_name();
};

struct test_print {
   static void test_prints();
   static void test_prints_l();
   static void test_printi();
   static void test_printui();
   static void test_printi128();
   static void test_printui128();
   static void test_printn();
   static void test_printsf();
   static void test_printdf();
   static void test_printqf();
   static void test_print_simple();
};

struct test_action {
   static void read_action_normal();
   static void read_action_to_0();
   static void read_action_to_64k();
   static void test_dummy_action();
   static void test_cf_action();
   static void require_notice(uint64_t receiver, uint64_t code, uint64_t action);
   static void require_notice_tests(uint64_t receiver, uint64_t code, uint64_t action);
   static void require_auth();
   static void assert_false();
   static void assert_true();
   static void assert_true_cf();
   static void test_current_time();
   static void test_abort() __attribute__ ((noreturn)) ;
   static void test_current_receiver(uint64_t receiver, uint64_t code, uint64_t action);
   static void test_publication_time();
   static void test_assert_code();
   static void test_ram_billing_in_notify(uint64_t receiver, uint64_t code, uint64_t action);
};

struct test_db {
   static void primary_i64_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void primary_i64_lowerbound(uint64_t receiver, uint64_t code, uint64_t action);
   static void primary_i64_upperbound(uint64_t receiver, uint64_t code, uint64_t action);

   static void idx64_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_lowerbound(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_upperbound(uint64_t receiver, uint64_t code, uint64_t action);

   static void test_invalid_access(uint64_t receiver, uint64_t code, uint64_t action);

   static void idx_double_nan_create_fail(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx_double_nan_modify_fail(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx_double_nan_lookup_fail(uint64_t receiver, uint64_t code, uint64_t action);

   static void misaligned_secondary_key256_tests(uint64_t, uint64_t, uint64_t);
};

struct test_multi_index {
   static void idx64_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_store_only(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_check_without_storing(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_require_find_fail(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_require_find_fail_with_msg(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_require_find_sk_fail(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx64_require_find_sk_fail_with_msg(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_store_only(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_check_without_storing(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_autoincrement_test(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_autoincrement_test_part1(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx128_autoincrement_test_part2(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx256_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx_double_general(uint64_t receiver, uint64_t code, uint64_t action);
   static void idx_long_double_general(uint64_t receiver, uint64_t code, uint64_t action);
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
   static void send_transaction(uint64_t receiver, uint64_t code, uint64_t action);
   static void send_transaction_empty(uint64_t receiver, uint64_t code, uint64_t action);
   static void send_transaction_trigger_error_handler(uint64_t receiver, uint64_t code, uint64_t action);
   static void assert_false_error_handler(const eosio::transaction&);
   static void send_transaction_max();
   static void send_transaction_large(uint64_t receiver, uint64_t code, uint64_t action);
   static void send_action_sender(uint64_t receiver, uint64_t code, uint64_t action);
   static void deferred_print();
   static void send_deferred_transaction(uint64_t receiver, uint64_t code, uint64_t action);
   static void send_deferred_transaction_replace(uint64_t receiver, uint64_t code, uint64_t action);
   static void send_deferred_tx_with_dtt_action();
   static void cancel_deferred_transaction_success();
   static void cancel_deferred_transaction_not_found();
   static void send_cf_action();
   static void send_cf_action_fail();
   static void stateful_api();
   static void context_free_api();
   static void repeat_deferred_transaction(uint64_t receiver, uint64_t code, uint64_t action);
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
   static void test_outofbound_0();
   static void test_outofbound_1();
   static void test_outofbound_2();
   static void test_outofbound_3();
   static void test_outofbound_4();
   static void test_outofbound_5();
   static void test_outofbound_6();
   static void test_outofbound_7();
   static void test_outofbound_8();
   static void test_outofbound_9();
   static void test_outofbound_10();
   static void test_outofbound_11();
   static void test_outofbound_12();
   static void test_outofbound_13();
};

struct test_checktime {
   static void checktime_pass();
   static void checktime_failure();
   static void checktime_sha1_failure();
   static void checktime_assert_sha1_failure();
   static void checktime_sha256_failure();
   static void checktime_assert_sha256_failure();
   static void checktime_sha512_failure();
   static void checktime_assert_sha512_failure();
   static void checktime_ripemd160_failure();
   static void checktime_assert_ripemd160_failure();

   static int i;
};

struct test_permission {
   static void check_authorization(uint64_t receiver, uint64_t code, uint64_t action);
   static void test_permission_last_used(uint64_t receiver, uint64_t code, uint64_t action);
   static void test_account_creation_time(uint64_t receiver, uint64_t code, uint64_t action);
};

struct test_datastream {
   static void test_basic();
};
