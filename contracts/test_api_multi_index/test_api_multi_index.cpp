/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include "../test_api/test_api.hpp"

#include "test_multi_index.cpp"

extern "C" {

    void init()  {

    }

   void apply( unsigned long long code, unsigned long long action ) {

      WASM_TEST_HANDLER(test_multi_index, idx64_general);
      WASM_TEST_HANDLER(test_multi_index, idx64_store_only);
      WASM_TEST_HANDLER(test_multi_index, idx64_check_without_storing);
      WASM_TEST_HANDLER(test_multi_index, idx128_autoincrement_test);
      WASM_TEST_HANDLER(test_multi_index, idx128_autoincrement_test_part1);
      WASM_TEST_HANDLER(test_multi_index, idx128_autoincrement_test_part2);
      WASM_TEST_HANDLER(test_multi_index, idx256_general);
      WASM_TEST_HANDLER(test_multi_index, idx_double_general);

      //unhandled test call
      eosio_assert(false, "Unknown Test");
   }

}
