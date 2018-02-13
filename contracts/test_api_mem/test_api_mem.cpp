/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include "../test_api/test_api.hpp"

#include "test_extended_memory.cpp"
#include "test_memory.cpp"

extern "C" {

    void init()  {

    }

   void apply( unsigned long long code, unsigned long long action ) {

      //eosio::print("==> CONTRACT: ", code, " ", action, "\n");

      //test_extended_memory
      WASM_TEST_HANDLER(test_extended_memory, test_initial_buffer);
      WASM_TEST_HANDLER(test_extended_memory, test_page_memory);
      WASM_TEST_HANDLER(test_extended_memory, test_page_memory_exceeded);
      WASM_TEST_HANDLER(test_extended_memory, test_page_memory_negative_bytes);

      //test_memory
      WASM_TEST_HANDLER(test_memory, test_memory_allocs);
      WASM_TEST_HANDLER(test_memory, test_memory_hunk);
      WASM_TEST_HANDLER(test_memory, test_memory_hunks);
      WASM_TEST_HANDLER(test_memory, test_memory_hunks_disjoint);
      WASM_TEST_HANDLER(test_memory, test_memset_memcpy);
      WASM_TEST_HANDLER(test_memory, test_memcpy_overlap_start);
      WASM_TEST_HANDLER(test_memory, test_memcpy_overlap_end);
      WASM_TEST_HANDLER(test_memory, test_memcmp);

      //unhandled test call
      eosio_assert(false, "Unknown Test");
   }

}
