#include <eoslib/eos.hpp>
#include "test_api.hpp"

extern "C" {

    void init()  {

    }

   void apply( unsigned long long code, unsigned long long action ) {

      //eos::print("==> CONTRACT: ", code, " ", action, "\n");

      //test_types
      WASM_TEST_HANDLER(test_types, types_size);
      WASM_TEST_HANDLER(test_types, char_to_symbol);
      WASM_TEST_HANDLER(test_types, string_to_name);
      WASM_TEST_HANDLER(test_types, name_class);

      //test_message
      WASM_TEST_HANDLER(test_message, read_message);
      WASM_TEST_HANDLER(test_message, read_message_to_0);
      WASM_TEST_HANDLER(test_message, read_message_to_64k);
      WASM_TEST_HANDLER(test_message, require_notice);
      WASM_TEST_HANDLER(test_message, require_auth);
      WASM_TEST_HANDLER(test_message, assert_false);
      WASM_TEST_HANDLER(test_message, assert_true);
      WASM_TEST_HANDLER(test_message, now);

      //test_print
      WASM_TEST_HANDLER(test_print, test_prints);
      WASM_TEST_HANDLER(test_print, test_printi);
      WASM_TEST_HANDLER(test_print, test_printi128);
      WASM_TEST_HANDLER(test_print, test_printn);

      //test_math
      WASM_TEST_HANDLER(test_math, test_multeq_i128);
      WASM_TEST_HANDLER(test_math, test_diveq_i128);
      WASM_TEST_HANDLER(test_math, test_diveq_i128_by_0);
      WASM_TEST_HANDLER(test_math, test_double_api);
      WASM_TEST_HANDLER(test_math, test_double_api_div_0);


      //test db
      WASM_TEST_HANDLER(test_db, key_i64_general);
      WASM_TEST_HANDLER(test_db, key_i64_remove_all);
      WASM_TEST_HANDLER(test_db, key_i64_small_load);
      WASM_TEST_HANDLER(test_db, key_i64_small_store);
      WASM_TEST_HANDLER(test_db, key_i64_store_scope);
      WASM_TEST_HANDLER(test_db, key_i64_remove_scope);
      WASM_TEST_HANDLER(test_db, key_i64_not_found);
      WASM_TEST_HANDLER(test_db, key_i64_front_back);
      WASM_TEST_HANDLER(test_db, key_i64i64i64_general);
      WASM_TEST_HANDLER(test_db, key_i128i128_general);

      //test crypto
      WASM_TEST_HANDLER(test_crypto, test_sha256);
      WASM_TEST_HANDLER(test_crypto, sha256_no_data);
      WASM_TEST_HANDLER(test_crypto, asert_sha256_false);
      WASM_TEST_HANDLER(test_crypto, asert_sha256_true);
      WASM_TEST_HANDLER(test_crypto, asert_no_data);

      //test transaction
      WASM_TEST_HANDLER(test_transaction, send_message);
      WASM_TEST_HANDLER(test_transaction, send_message_empty);
      WASM_TEST_HANDLER(test_transaction, send_message_max);
      WASM_TEST_HANDLER(test_transaction, send_message_large);
      WASM_TEST_HANDLER(test_transaction, send_message_recurse);
      WASM_TEST_HANDLER(test_transaction, send_message_inline_fail);
      WASM_TEST_HANDLER(test_transaction, send_transaction);
      WASM_TEST_HANDLER(test_transaction, send_transaction_empty);
      WASM_TEST_HANDLER(test_transaction, send_transaction_max);
      WASM_TEST_HANDLER(test_transaction, send_transaction_large);
      
      //unhandled test call
      WASM_TEST_ERROR_CODE = WASM_TEST_FAIL;
   }

}
