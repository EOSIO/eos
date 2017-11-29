/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>
#include "test_api.hpp"

//#include "test_account.cpp"
//#include "test_chain.cpp"
//#include "test_crypto.cpp"
//#include "test_db.cpp"
//#include "test_math.cpp"
#include "test_action.cpp"
#include "test_print.cpp"
//#include "test_string.cpp"
//#include "test_fixedpoint.cpp"
//#include "test_real.cpp"
//#include "test_transaction.cpp"
//#include "test_types.cpp"

extern "C" {

    void init()  {

    }

   void apply( unsigned long long code, unsigned long long action ) {

      //eosio::print("==> CONTRACT: ", code, " ", action, "\n");

      //test_types
//      WASM_TEST_HANDLER(test_types, types_size);
//      WASM_TEST_HANDLER(test_types, char_to_symbol);
//      WASM_TEST_HANDLER(test_types, string_to_name);
//      WASM_TEST_HANDLER(test_types, name_class);
//
//      //test_action
      WASM_TEST_HANDLER(test_action, read_action_normal);
      WASM_TEST_HANDLER(test_action, read_action_to_0);
      WASM_TEST_HANDLER(test_action, read_action_to_64k);
//      WASM_TEST_HANDLER(test_action, require_notice);
//      WASM_TEST_HANDLER(test_action, require_auth);
      WASM_TEST_HANDLER(test_action, assert_false);
      WASM_TEST_HANDLER(test_action, assert_true);
//      WASM_TEST_HANDLER(test_action, now);

      //test_print
      WASM_TEST_HANDLER(test_print, test_prints);
      WASM_TEST_HANDLER(test_print, test_printi);
      WASM_TEST_HANDLER(test_print, test_printi128);
      WASM_TEST_HANDLER(test_print, test_printn);

//      //test_math
//      WASM_TEST_HANDLER(test_math, test_multeq_i128);
//      WASM_TEST_HANDLER(test_math, test_diveq_i128);
//      WASM_TEST_HANDLER(test_math, test_diveq_i128_by_0);
//      WASM_TEST_HANDLER(test_math, test_double_api);
//      WASM_TEST_HANDLER(test_math, test_double_api_div_0);
//
//
//      //test db
//      WASM_TEST_HANDLER(test_db, key_i64_general);
//      WASM_TEST_HANDLER(test_db, key_i64_remove_all);
//      WASM_TEST_HANDLER(test_db, key_i64_small_load);
//      WASM_TEST_HANDLER(test_db, key_i64_small_store);
//      WASM_TEST_HANDLER(test_db, key_i64_store_scope);
//      WASM_TEST_HANDLER(test_db, key_i64_remove_scope);
//      WASM_TEST_HANDLER(test_db, key_i64_not_found);
//      WASM_TEST_HANDLER(test_db, key_i64_front_back);
//      WASM_TEST_HANDLER(test_db, key_i64i64i64_general);
//      WASM_TEST_HANDLER(test_db, key_i128i128_general);
//      WASM_TEST_HANDLER(test_db, key_str_general);
//      WASM_TEST_HANDLER(test_db, key_str_table);
//      WASM_TEST_HANDLER(test_db, key_str_setup_limit);
//      WASM_TEST_HANDLER(test_db, key_str_min_exceed_limit);
//      WASM_TEST_HANDLER(test_db, key_str_under_limit);
//      WASM_TEST_HANDLER(test_db, key_str_available_space_exceed_limit);
//      WASM_TEST_HANDLER(test_db, key_str_another_under_limit);
//      WASM_TEST_HANDLER(test_db, key_i64_setup_limit);
//      WASM_TEST_HANDLER(test_db, key_i64_min_exceed_limit);
//      WASM_TEST_HANDLER(test_db, key_i64_under_limit);
//      WASM_TEST_HANDLER(test_db, key_i64_available_space_exceed_limit);
//      WASM_TEST_HANDLER(test_db, key_i64_another_under_limit);
//      WASM_TEST_HANDLER(test_db, key_i128i128_setup_limit);
//      WASM_TEST_HANDLER(test_db, key_i128i128_min_exceed_limit);
//      WASM_TEST_HANDLER(test_db, key_i128i128_under_limit);
//      WASM_TEST_HANDLER(test_db, key_i128i128_available_space_exceed_limit);
//      WASM_TEST_HANDLER(test_db, key_i128i128_another_under_limit);
//      WASM_TEST_HANDLER(test_db, key_i64i64i64_setup_limit);
//      WASM_TEST_HANDLER(test_db, key_i64i64i64_min_exceed_limit);
//      WASM_TEST_HANDLER(test_db, key_i64i64i64_under_limit);
//      WASM_TEST_HANDLER(test_db, key_i64i64i64_available_space_exceed_limit);
//      WASM_TEST_HANDLER(test_db, key_i64i64i64_another_under_limit);
//
//      //test crypto
//      WASM_TEST_HANDLER(test_crypto, test_sha256);
//      WASM_TEST_HANDLER(test_crypto, sha256_no_data);
//      WASM_TEST_HANDLER(test_crypto, asert_sha256_false);
//      WASM_TEST_HANDLER(test_crypto, asert_sha256_true);
//      WASM_TEST_HANDLER(test_crypto, asert_no_data);
//
//      //test transaction
//      WASM_TEST_HANDLER(test_transaction, send_message);
//      WASM_TEST_HANDLER(test_transaction, send_message_empty);
//      WASM_TEST_HANDLER(test_transaction, send_message_max);
//      WASM_TEST_HANDLER(test_transaction, send_message_large);
//      WASM_TEST_HANDLER(test_transaction, send_message_recurse);
//      WASM_TEST_HANDLER(test_transaction, send_message_inline_fail);
//      WASM_TEST_HANDLER(test_transaction, send_transaction);
//      WASM_TEST_HANDLER(test_transaction, send_transaction_empty);
//      WASM_TEST_HANDLER(test_transaction, send_transaction_max);
//      WASM_TEST_HANDLER(test_transaction, send_transaction_large);
//
//      //test chain
//      WASM_TEST_HANDLER(test_chain, test_activeprods);
//
//      // test string
//      WASM_TEST_HANDLER(test_string, construct_with_size);
//      WASM_TEST_HANDLER(test_string, construct_with_data);
//      WASM_TEST_HANDLER(test_string, construct_with_data_copied);
//      WASM_TEST_HANDLER(test_string, construct_with_data_partially);
//      WASM_TEST_HANDLER(test_string, copy_constructor);
//      WASM_TEST_HANDLER(test_string, assignment_operator);
//      WASM_TEST_HANDLER(test_string, index_operator);
//      WASM_TEST_HANDLER(test_string, index_out_of_bound);
//      WASM_TEST_HANDLER(test_string, substring);
//      WASM_TEST_HANDLER(test_string, substring_out_of_bound);
//      WASM_TEST_HANDLER(test_string, concatenation_null_terminated);
//      WASM_TEST_HANDLER(test_string, concatenation_non_null_terminated);
//      WASM_TEST_HANDLER(test_string, assign);
//      WASM_TEST_HANDLER(test_string, comparison_operator);
//      WASM_TEST_HANDLER(test_string, print_null_terminated);
//      WASM_TEST_HANDLER(test_string, print_non_null_terminated);
//      WASM_TEST_HANDLER(test_string, print_unicode);
//      WASM_TEST_HANDLER(test_string, valid_utf8);
//      WASM_TEST_HANDLER(test_string, invalid_utf8);
//      WASM_TEST_HANDLER(test_string, string_literal);
//
//      // test fixed_point
//      WASM_TEST_HANDLER(test_fixedpoint, create_instances);
//      WASM_TEST_HANDLER(test_fixedpoint, test_addition);
//      WASM_TEST_HANDLER(test_fixedpoint, test_subtraction);
//      WASM_TEST_HANDLER(test_fixedpoint, test_multiplication);
//      WASM_TEST_HANDLER(test_fixedpoint, test_division);
//
//
//      // test double
//      WASM_TEST_HANDLER(test_real, create_instances);
//      WASM_TEST_HANDLER(test_real, test_addition);
//      WASM_TEST_HANDLER(test_real, test_multiplication);
//      WASM_TEST_HANDLER(test_real, test_division);
//
//      // test account
//      WASM_TEST_HANDLER(test_account, test_balance_acc1);
//
      //unhandled test call
      assert(false, "Unknown Test");
   }

}
