/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/message.hpp>

#include "test_api.hpp"

unsigned int test_message::read_message_normal() {

   char buffer[100];
   uint32_t total = 0;

   WASM_ASSERT( current_code() == N(testapi),  "current_code() == N(testapi)" );

   WASM_ASSERT(message_size() == sizeof(dummy_message), "message_size() == sizeof(dummy_message)");

   total = read_message(buffer, 30);
   WASM_ASSERT(total == sizeof(dummy_message) , "read_message(30)" );

   total = read_message(buffer, 100);
   WASM_ASSERT(total == sizeof(dummy_message) , "read_message(100)" );

   total = read_message(buffer, 5);
   WASM_ASSERT(total == 5 , "read_message(5)" );

   total = read_message(buffer, sizeof(dummy_message) );
   WASM_ASSERT(total == sizeof(dummy_message), "read_message(sizeof(dummy_message))" );

   dummy_message *dummy13 = reinterpret_cast<dummy_message *>(buffer);
   WASM_ASSERT(dummy13->a == DUMMY_MESSAGE_DEFAULT_A, "dummy13->a == DUMMY_MESSAGE_DEFAULT_A");
   WASM_ASSERT(dummy13->b == DUMMY_MESSAGE_DEFAULT_B, "dummy13->b == DUMMY_MESSAGE_DEFAULT_B");
   WASM_ASSERT(dummy13->c == DUMMY_MESSAGE_DEFAULT_C, "dummy13->c == DUMMY_MESSAGE_DEFAULT_C");

   return WASM_TEST_PASS;
}

unsigned int test_message::read_message_to_0() {
   uint32_t total = read_message((void *)0, 0x7FFFFFFF);
   return WASM_TEST_PASS;
}

unsigned int test_message::read_message_to_64k() {
   uint32_t total = read_message( (void *)((1<<16)-1), 0x7FFFFFFF);
   return WASM_TEST_PASS;
}

unsigned int test_message::require_notice() {
   if( current_code() == N(testapi) ) {
      eosio::require_notice( N(acc1) );
      eosio::require_notice( N(acc2) );
      eosio::require_notice( N(acc1), N(acc2) );
      return WASM_TEST_FAIL;
   } else if ( current_code() == N(acc1) || current_code() == N(acc2) ) {
      return WASM_TEST_PASS;
   }
   return WASM_TEST_FAIL;
}

unsigned int test_message::require_auth() {
   eosio::require_auth( N(acc3) );
   eosio::require_auth( N(acc4) );
   return WASM_TEST_PASS;
}

unsigned int test_message::assert_false() {
   assert(false, "test_message::assert_false");
   return WASM_TEST_PASS;
}

unsigned int test_message::assert_true() {
   assert(true, "test_message::assert_true");
   return WASM_TEST_PASS;
}

unsigned int test_message::now() {
   uint32_t tmp = 0;
   uint32_t total = read_message(&tmp, sizeof(uint32_t));
   WASM_ASSERT( total == sizeof(uint32_t), "total == sizeof(uint32_t)");
   WASM_ASSERT( tmp == ::now(), "tmp == now()" );
   return WASM_TEST_PASS;
}

