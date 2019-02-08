/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>

#include "test_api.hpp"

#include "test_action.cpp"
#include "test_chain.cpp"
#include "test_checktime.cpp"
#include "test_crypto.cpp"
#include "test_datastream.cpp"
#include "test_permission.cpp"
#include "test_print.cpp"
#include "test_transaction.cpp"
#include "test_types.cpp"

name global_receiver;

extern "C" {
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
      if( code == "eosio"_n.value && action == "onerror"_n.value ) {
         auto error = eosio::onerror::from_current_action();
         eosio::print("onerror called\n");
         auto error_trx = error.unpack_sent_trx();
         auto error_action = error_trx.actions.at(0).name;

         // Error handlers for deferred transactions in these tests currently only support the first action

         WASM_TEST_ERROR_HANDLER( "test_action", "assert_false", test_transaction, assert_false_error_handler );


         return;
      }

      if ( action == name{"cfaction"}.value ) {
         test_action::test_cf_action();
         return;
      }
      WASM_TEST_HANDLER( test_action, assert_true_cf );

      if ( action != WASM_TEST_ACTION("test_transaction", "stateful_api") && action != WASM_TEST_ACTION("test_transaction", "context_free_api") )
         require_auth(code);

      //test_types
      WASM_TEST_HANDLER( test_types, types_size     );
      WASM_TEST_HANDLER( test_types, char_to_symbol );
      WASM_TEST_HANDLER( test_types, string_to_name );

      //test_action
      WASM_TEST_HANDLER   ( test_action, read_action_normal         );
      WASM_TEST_HANDLER   ( test_action, read_action_to_0           );
      WASM_TEST_HANDLER   ( test_action, read_action_to_64k         );
      WASM_TEST_HANDLER_EX( test_action, require_notice             );
      WASM_TEST_HANDLER_EX( test_action, require_notice_tests       );
      WASM_TEST_HANDLER   ( test_action, require_auth               );
      WASM_TEST_HANDLER   ( test_action, assert_false               );
      WASM_TEST_HANDLER   ( test_action, assert_true                );
      WASM_TEST_HANDLER   ( test_action, test_current_time          );
      WASM_TEST_HANDLER   ( test_action, test_abort                 );
      WASM_TEST_HANDLER_EX( test_action, test_current_receiver      );
      WASM_TEST_HANDLER   ( test_action, test_publication_time      );
      WASM_TEST_HANDLER   ( test_action, test_assert_code           );
      WASM_TEST_HANDLER_EX( test_action, test_ram_billing_in_notify );

      // test named actions
      // We enforce action name matches action data type name, so name mangling will not work for these tests.
      if ( action == name{"dummyaction"}.value ) {
         test_action::test_dummy_action();
         return;
      }
      //test_print
      WASM_TEST_HANDLER( test_print, test_prints     );
      WASM_TEST_HANDLER( test_print, test_prints_l   );
      WASM_TEST_HANDLER( test_print, test_printi     );
      WASM_TEST_HANDLER( test_print, test_printui    );
      WASM_TEST_HANDLER( test_print, test_printi128  );
      WASM_TEST_HANDLER( test_print, test_printui128 );
      WASM_TEST_HANDLER( test_print, test_printn     );
      WASM_TEST_HANDLER( test_print, test_printsf    );
      WASM_TEST_HANDLER( test_print, test_printdf    );
      WASM_TEST_HANDLER( test_print, test_printqf    );

      //test crypto
      WASM_TEST_HANDLER( test_crypto, test_recover_key              );
      WASM_TEST_HANDLER( test_crypto, test_recover_key_assert_true  );
      WASM_TEST_HANDLER( test_crypto, test_recover_key_assert_false );
      WASM_TEST_HANDLER( test_crypto, test_sha1                     );
      WASM_TEST_HANDLER( test_crypto, test_sha256                   );
      WASM_TEST_HANDLER( test_crypto, test_sha512                   );
      WASM_TEST_HANDLER( test_crypto, test_ripemd160                );
      WASM_TEST_HANDLER( test_crypto, sha1_no_data                  );
      WASM_TEST_HANDLER( test_crypto, sha256_no_data                );
      WASM_TEST_HANDLER( test_crypto, sha512_no_data                );
      WASM_TEST_HANDLER( test_crypto, ripemd160_no_data             );
      WASM_TEST_HANDLER( test_crypto, sha256_null                   );
      WASM_TEST_HANDLER( test_crypto, assert_sha256_false           );
      WASM_TEST_HANDLER( test_crypto, assert_sha256_true            );
      WASM_TEST_HANDLER( test_crypto, assert_sha1_false             );
      WASM_TEST_HANDLER( test_crypto, assert_sha1_true              );
      WASM_TEST_HANDLER( test_crypto, assert_sha512_false           );
      WASM_TEST_HANDLER( test_crypto, assert_sha512_true            );
      WASM_TEST_HANDLER( test_crypto, assert_ripemd160_false        );
      WASM_TEST_HANDLER( test_crypto, assert_ripemd160_true         );

      //test transaction
      WASM_TEST_HANDLER   ( test_transaction, test_tapos_block_num                   );
      WASM_TEST_HANDLER   ( test_transaction, test_tapos_block_prefix                );
      WASM_TEST_HANDLER   ( test_transaction, send_action                            );
      WASM_TEST_HANDLER   ( test_transaction, send_action_inline_fail                );
      WASM_TEST_HANDLER   ( test_transaction, send_action_empty                      );
      WASM_TEST_HANDLER   ( test_transaction, send_action_large                      );
      WASM_TEST_HANDLER   ( test_transaction, send_action_recurse                    );
      WASM_TEST_HANDLER   ( test_transaction, test_read_transaction                  );
      WASM_TEST_HANDLER   ( test_transaction, test_transaction_size                  );
      WASM_TEST_HANDLER_EX( test_transaction, send_transaction                       );
      WASM_TEST_HANDLER_EX( test_transaction, send_transaction_empty                 );
      WASM_TEST_HANDLER_EX( test_transaction, send_transaction_trigger_error_handler );
      WASM_TEST_HANDLER_EX( test_transaction, send_transaction_large                 );
      WASM_TEST_HANDLER_EX( test_transaction, send_action_sender                     );
      WASM_TEST_HANDLER   ( test_transaction, deferred_print                         );
      WASM_TEST_HANDLER_EX( test_transaction, send_deferred_transaction              );
      WASM_TEST_HANDLER_EX( test_transaction, send_deferred_transaction_replace      );
      WASM_TEST_HANDLER   ( test_transaction, send_deferred_tx_with_dtt_action       );
      WASM_TEST_HANDLER   ( test_transaction, cancel_deferred_transaction_success    );
      WASM_TEST_HANDLER   ( test_transaction, cancel_deferred_transaction_not_found  );
      WASM_TEST_HANDLER   ( test_transaction, send_cf_action                         );
      WASM_TEST_HANDLER   ( test_transaction, send_cf_action_fail                    );
      WASM_TEST_HANDLER   ( test_transaction, stateful_api                           );
      WASM_TEST_HANDLER   ( test_transaction, context_free_api                       );
      WASM_TEST_HANDLER_EX( test_transaction, repeat_deferred_transaction            );

      //test chain
      WASM_TEST_HANDLER( test_chain, test_activeprods );

      // test checktime
      WASM_TEST_HANDLER( test_checktime, checktime_pass                     );
      WASM_TEST_HANDLER( test_checktime, checktime_failure                  );
      WASM_TEST_HANDLER( test_checktime, checktime_sha1_failure             );
      WASM_TEST_HANDLER( test_checktime, checktime_assert_sha1_failure      );
      WASM_TEST_HANDLER( test_checktime, checktime_sha256_failure           );
      WASM_TEST_HANDLER( test_checktime, checktime_assert_sha256_failure    );
      WASM_TEST_HANDLER( test_checktime, checktime_sha512_failure           );
      WASM_TEST_HANDLER( test_checktime, checktime_assert_sha512_failure    );
      WASM_TEST_HANDLER( test_checktime, checktime_ripemd160_failure        );
      WASM_TEST_HANDLER( test_checktime, checktime_assert_ripemd160_failure );

      // test datastream
      WASM_TEST_HANDLER( test_datastream, test_basic );

      // test permission
      WASM_TEST_HANDLER_EX( test_permission, check_authorization        );
      WASM_TEST_HANDLER_EX( test_permission, test_permission_last_used  );
      WASM_TEST_HANDLER_EX( test_permission, test_account_creation_time );

      //unhandled test call
      eosio_assert( false, "Unknown Test" );

   }
}
