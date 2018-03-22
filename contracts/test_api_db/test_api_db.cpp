/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include "../test_api/test_api.hpp"

#include "test_db.cpp"

extern "C" {

    void init()  {

    }

   void apply( unsigned long long, unsigned long long action ) {

      WASM_TEST_HANDLER(test_db, primary_i64_general);
      WASM_TEST_HANDLER(test_db, primary_i64_lowerbound);
      WASM_TEST_HANDLER(test_db, primary_i64_upperbound);
      WASM_TEST_HANDLER(test_db, idx64_general);
      WASM_TEST_HANDLER(test_db, idx64_lowerbound);
      WASM_TEST_HANDLER(test_db, idx64_upperbound);

      //unhandled test call
      eosio_assert(false, "Unknown Test");
   }

}
