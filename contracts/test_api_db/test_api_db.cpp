/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>
#include "../test_api/test_api.hpp"

#include "test_db.cpp"

extern "C" {

    void init()  {

    }

   void apply( unsigned long long code, unsigned long long action ) {

      //test db
      WASM_TEST_HANDLER(test_db, key_i64_general);
      WASM_TEST_HANDLER(test_db, key_i64_remove_all);
      WASM_TEST_HANDLER(test_db, key_i64_small_load);
      WASM_TEST_HANDLER(test_db, key_i64_small_store);
      WASM_TEST_HANDLER(test_db, key_i64_store_scope);
      WASM_TEST_HANDLER(test_db, key_i64_remove_scope);
      WASM_TEST_HANDLER(test_db, key_i64_not_found);
      WASM_TEST_HANDLER(test_db, key_i64_front_back);
      WASM_TEST_HANDLER(test_db, key_i128i128_general);
#if 0
      WASM_TEST_HANDLER(test_db, key_str_general);
      WASM_TEST_HANDLER(test_db, key_str_table);
      WASM_TEST_HANDLER(test_db, key_str_setup_limit);
      WASM_TEST_HANDLER(test_db, key_str_min_exceed_limit);
      WASM_TEST_HANDLER(test_db, key_str_under_limit);
      WASM_TEST_HANDLER(test_db, key_str_available_space_exceed_limit);
      WASM_TEST_HANDLER(test_db, key_str_another_under_limit);
      WASM_TEST_HANDLER(test_db, key_i64_setup_limit);
      WASM_TEST_HANDLER(test_db, key_i64_min_exceed_limit);
      WASM_TEST_HANDLER(test_db, key_i64_under_limit);
      WASM_TEST_HANDLER(test_db, key_i64_available_space_exceed_limit);
      WASM_TEST_HANDLER(test_db, key_i64_another_under_limit);
      WASM_TEST_HANDLER(test_db, key_i128i128_setup_limit);
      WASM_TEST_HANDLER(test_db, key_i128i128_min_exceed_limit);
      WASM_TEST_HANDLER(test_db, key_i128i128_under_limit);
      WASM_TEST_HANDLER(test_db, key_i128i128_available_space_exceed_limit);
      WASM_TEST_HANDLER(test_db, key_i128i128_another_under_limit);
      WASM_TEST_HANDLER(test_db, key_i64i64i64_setup_limit);
      WASM_TEST_HANDLER(test_db, key_i64i64i64_min_exceed_limit);
      WASM_TEST_HANDLER(test_db, key_i64i64i64_under_limit);
      WASM_TEST_HANDLER(test_db, key_i64i64i64_available_space_exceed_limit);
      WASM_TEST_HANDLER(test_db, key_i64i64i64_another_under_limit);
      WASM_TEST_HANDLER(test_db, key_i64i64i64_general);
#endif

      //unhandled test call
      assert(false, "Unknown Test");
   }

}
