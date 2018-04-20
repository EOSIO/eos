/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include "../test_api/test_api.hpp"

#include "test_extended_memory.cpp"
#include "test_memory.cpp"

extern "C" {
   void apply( uint64_t /*receiver*/, uint64_t code, uint64_t action ) {
      require_auth(code);

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
      WASM_TEST_HANDLER(test_memory, test_outofbound_0);
      WASM_TEST_HANDLER(test_memory, test_outofbound_1);
      WASM_TEST_HANDLER(test_memory, test_outofbound_2);
      WASM_TEST_HANDLER(test_memory, test_outofbound_3);
      WASM_TEST_HANDLER(test_memory, test_outofbound_4);
      WASM_TEST_HANDLER(test_memory, test_outofbound_5);
      WASM_TEST_HANDLER(test_memory, test_outofbound_6);
      WASM_TEST_HANDLER(test_memory, test_outofbound_7);
      WASM_TEST_HANDLER(test_memory, test_outofbound_8);
      WASM_TEST_HANDLER(test_memory, test_outofbound_9);
      WASM_TEST_HANDLER(test_memory, test_outofbound_10);
      WASM_TEST_HANDLER(test_memory, test_outofbound_11);
      WASM_TEST_HANDLER(test_memory, test_outofbound_12);
      WASM_TEST_HANDLER(test_memory, test_outofbound_13);
      
      //unhandled test call
      eosio_assert(false, "Unknown Test");
   }

}
