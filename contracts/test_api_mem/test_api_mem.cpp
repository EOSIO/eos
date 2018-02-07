/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include "../test_api/test_api.hpp"

#include "test_extended_memory.cpp"
#include "test_memory.cpp"
#include "test_string.cpp"

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

      // test string
      WASM_TEST_HANDLER(test_string, construct_with_size);
      WASM_TEST_HANDLER(test_string, construct_with_data);
      WASM_TEST_HANDLER(test_string, construct_with_data_copied);
      WASM_TEST_HANDLER(test_string, construct_with_data_partially);
      WASM_TEST_HANDLER(test_string, copy_constructor);
      WASM_TEST_HANDLER(test_string, assignment_operator);
      WASM_TEST_HANDLER(test_string, index_operator);
      WASM_TEST_HANDLER(test_string, index_out_of_bound);
      WASM_TEST_HANDLER(test_string, substring);
      WASM_TEST_HANDLER(test_string, concatenation_null_terminated);
      WASM_TEST_HANDLER(test_string, substring_out_of_bound);
      WASM_TEST_HANDLER(test_string, concatenation_non_null_terminated);
      WASM_TEST_HANDLER(test_string, assign);
      WASM_TEST_HANDLER(test_string, comparison_operator);
      WASM_TEST_HANDLER(test_string, print_null_terminated);
      WASM_TEST_HANDLER(test_string, print_non_null_terminated);
      WASM_TEST_HANDLER(test_string, print_unicode);
      WASM_TEST_HANDLER(test_string, valid_utf8);
      WASM_TEST_HANDLER(test_string, invalid_utf8);
      WASM_TEST_HANDLER(test_string, string_literal);

      //unhandled test call
      eos_assert(false, "Unknown Test");
   }

}
