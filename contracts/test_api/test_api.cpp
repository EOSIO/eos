/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>

#include "test_api.hpp"
#include "test_action.cpp"
#include "test_print.cpp"
#include "test_types.cpp"
#include "test_fixedpoint.cpp"
#include "test_math.cpp"
#include "test_compiler_builtins.cpp"
#include "test_real.cpp"
#include "test_crypto.cpp"
#include "test_chain.cpp"
#include "test_transaction.cpp"
#include "test_checktime.cpp"
#include "test_permission.cpp"
#include "test_datastream.cpp"

account_name global_receiver;

extern "C" {
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
      if( code == N(eosio) && action == N(onerror) ) {
         auto error_dtrx = eosio::deferred_transaction::from_current_action();
         eosio::print("onerror called\n");
         auto error_action = error_dtrx.actions.at(0).name;

         // Error handlers for deferred transactions in these tests currently only support the first action

         WASM_TEST_ERROR_HANDLER("test_action", "assert_false", test_transaction, assert_false_error_handler );


         return;
      }

      if ( action == N(cf_action) ) {
         test_action::test_cf_action();
         return;
      }
      WASM_TEST_HANDLER(test_action, assert_true_cf);

      require_auth(code);

      //test_types
      WASM_TEST_HANDLER(test_types, types_size);
      WASM_TEST_HANDLER(test_types, char_to_symbol);
      WASM_TEST_HANDLER(test_types, string_to_name);
      WASM_TEST_HANDLER(test_types, name_class);

      //test_compiler_builtins
      WASM_TEST_HANDLER(test_compiler_builtins, test_multi3);
      WASM_TEST_HANDLER(test_compiler_builtins, test_divti3);
      WASM_TEST_HANDLER(test_compiler_builtins, test_divti3_by_0);
      WASM_TEST_HANDLER(test_compiler_builtins, test_udivti3);
      WASM_TEST_HANDLER(test_compiler_builtins, test_udivti3_by_0);
      WASM_TEST_HANDLER(test_compiler_builtins, test_modti3);
      WASM_TEST_HANDLER(test_compiler_builtins, test_modti3_by_0);
      WASM_TEST_HANDLER(test_compiler_builtins, test_umodti3);
      WASM_TEST_HANDLER(test_compiler_builtins, test_umodti3_by_0);
      WASM_TEST_HANDLER(test_compiler_builtins, test_lshlti3);
      WASM_TEST_HANDLER(test_compiler_builtins, test_lshrti3);
      WASM_TEST_HANDLER(test_compiler_builtins, test_ashlti3);
      WASM_TEST_HANDLER(test_compiler_builtins, test_ashrti3);

      //test_action
      WASM_TEST_HANDLER(test_action, read_action_normal);
      WASM_TEST_HANDLER(test_action, read_action_to_0);
      WASM_TEST_HANDLER(test_action, read_action_to_64k);
      WASM_TEST_HANDLER_EX(test_action, require_notice);
      WASM_TEST_HANDLER(test_action, require_auth);
      WASM_TEST_HANDLER(test_action, assert_false);
      WASM_TEST_HANDLER(test_action, assert_true);
      WASM_TEST_HANDLER(test_action, now);
      WASM_TEST_HANDLER(test_action, test_abort);
      WASM_TEST_HANDLER_EX(test_action, test_current_receiver);
      WASM_TEST_HANDLER(test_action, test_current_sender);
      WASM_TEST_HANDLER(test_action, test_publication_time);

      // test named actions
      // We enforce action name matches action data type name, so name mangling will not work for these tests.
      if ( action == N(dummy_action) ) {
         test_action::test_dummy_action();
         return;
      }
      //test_print
      WASM_TEST_HANDLER(test_print, test_prints);
      WASM_TEST_HANDLER(test_print, test_prints_l);
      WASM_TEST_HANDLER(test_print, test_printi);
      WASM_TEST_HANDLER(test_print, test_printui);
      WASM_TEST_HANDLER(test_print, test_printi128);
      WASM_TEST_HANDLER(test_print, test_printn);

      //test_math
      WASM_TEST_HANDLER(test_math, test_multeq);
      WASM_TEST_HANDLER(test_math, test_diveq);
      WASM_TEST_HANDLER(test_math, test_i64_to_double);
      WASM_TEST_HANDLER(test_math, test_double_to_i64);
      WASM_TEST_HANDLER(test_math, test_diveq_by_0);
      WASM_TEST_HANDLER(test_math, test_double_api);
      WASM_TEST_HANDLER(test_math, test_double_api_div_0);

      //test crypto
      WASM_TEST_HANDLER(test_crypto, test_recover_key);
      WASM_TEST_HANDLER(test_crypto, test_recover_key_assert_true);
      WASM_TEST_HANDLER(test_crypto, test_recover_key_assert_false);
      WASM_TEST_HANDLER(test_crypto, test_sha1);
      WASM_TEST_HANDLER(test_crypto, test_sha256);
      WASM_TEST_HANDLER(test_crypto, test_sha512);
      WASM_TEST_HANDLER(test_crypto, test_ripemd160);
      WASM_TEST_HANDLER(test_crypto, sha1_no_data);
      WASM_TEST_HANDLER(test_crypto, sha256_no_data);
      WASM_TEST_HANDLER(test_crypto, sha512_no_data);
      WASM_TEST_HANDLER(test_crypto, ripemd160_no_data);
      WASM_TEST_HANDLER(test_crypto, sha256_null);
      WASM_TEST_HANDLER(test_crypto, assert_sha256_false);
      WASM_TEST_HANDLER(test_crypto, assert_sha256_true);
      WASM_TEST_HANDLER(test_crypto, assert_sha1_false);
      WASM_TEST_HANDLER(test_crypto, assert_sha1_true);
      WASM_TEST_HANDLER(test_crypto, assert_sha512_false);
      WASM_TEST_HANDLER(test_crypto, assert_sha512_true);
      WASM_TEST_HANDLER(test_crypto, assert_ripemd160_false);
      WASM_TEST_HANDLER(test_crypto, assert_ripemd160_true);

      //test transaction
      WASM_TEST_HANDLER(test_transaction, test_tapos_block_num);
      WASM_TEST_HANDLER(test_transaction, test_tapos_block_prefix);
      WASM_TEST_HANDLER(test_transaction, send_action);
      WASM_TEST_HANDLER(test_transaction, send_action_inline_fail);
      WASM_TEST_HANDLER(test_transaction, send_action_empty);
      WASM_TEST_HANDLER(test_transaction, send_action_large);
      WASM_TEST_HANDLER(test_transaction, send_action_recurse);
      WASM_TEST_HANDLER(test_transaction, test_read_transaction);
      WASM_TEST_HANDLER(test_transaction, test_transaction_size);
      WASM_TEST_HANDLER_EX(test_transaction, send_transaction);
      WASM_TEST_HANDLER_EX(test_transaction, send_transaction_empty);
      WASM_TEST_HANDLER_EX(test_transaction, send_transaction_trigger_error_handler);
      WASM_TEST_HANDLER_EX(test_transaction, send_transaction_large);
      WASM_TEST_HANDLER_EX(test_transaction, send_action_sender);
      WASM_TEST_HANDLER_EX(test_transaction, send_transaction_expiring_late);
      WASM_TEST_HANDLER(test_transaction, deferred_print);
      WASM_TEST_HANDLER_EX(test_transaction, send_deferred_transaction);
      WASM_TEST_HANDLER(test_transaction, cancel_deferred_transaction);
      WASM_TEST_HANDLER(test_transaction, send_cf_action);
      WASM_TEST_HANDLER(test_transaction, send_cf_action_fail);
      WASM_TEST_HANDLER(test_transaction, read_inline_action);
      WASM_TEST_HANDLER(test_transaction, read_inline_cf_action);

      //test chain
      WASM_TEST_HANDLER(test_chain, test_activeprods);

      // test fixed_point
      WASM_TEST_HANDLER(test_fixedpoint, create_instances);
      WASM_TEST_HANDLER(test_fixedpoint, test_addition);
      WASM_TEST_HANDLER(test_fixedpoint, test_subtraction);
      WASM_TEST_HANDLER(test_fixedpoint, test_multiplication);
      WASM_TEST_HANDLER(test_fixedpoint, test_division);
      WASM_TEST_HANDLER(test_fixedpoint, test_division_by_0);

      // test double
      WASM_TEST_HANDLER(test_real, create_instances);
      WASM_TEST_HANDLER(test_real, test_addition);
      WASM_TEST_HANDLER(test_real, test_multiplication);
      WASM_TEST_HANDLER(test_real, test_division);
      WASM_TEST_HANDLER(test_real, test_division_by_0);

      // test checktime
      WASM_TEST_HANDLER(test_checktime, checktime_pass);
      WASM_TEST_HANDLER(test_checktime, checktime_failure);

      // test datastream
      WASM_TEST_HANDLER(test_datastream, test_basic);

      // test permission
      WASM_TEST_HANDLER_EX(test_permission, check_authorization);

      //unhandled test call
      eosio_assert(false, "Unknown Test");

   }
}
