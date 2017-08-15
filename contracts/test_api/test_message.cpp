#include <eoslib/message.hpp>

#include "test_api.hpp"

unsigned int test_message::read_message() {

   char buffer[100];
   uint32_t total = 0;

   WASM_ASSERT( currentCode() == N(testapi),  "currentCode() == N(testapi)" );

   WASM_ASSERT(messageSize() == sizeof(dummy_message), "messageSize() == sizeof(dummy_message)");

   total = readMessage(buffer, 30);
   WASM_ASSERT(total == sizeof(dummy_message) , "readMessage(30)" );

   total = readMessage(buffer, 100);
   WASM_ASSERT(total == sizeof(dummy_message) , "readMessage(100)" );

   total = readMessage(buffer, 5);
   WASM_ASSERT(total == 5 , "readMessage(5)" );

   total = readMessage(buffer, sizeof(dummy_message) );
   WASM_ASSERT(total == sizeof(dummy_message), "readMessage(sizeof(dummy_message))" );

   dummy_message *dummy13 = reinterpret_cast<dummy_message *>(buffer);
   WASM_ASSERT(dummy13->a == DUMMY_MESSAGE_DEFAULT_A, "dummy13->a == DUMMY_MESSAGE_DEFAULT_A");
   WASM_ASSERT(dummy13->b == DUMMY_MESSAGE_DEFAULT_B, "dummy13->b == DUMMY_MESSAGE_DEFAULT_B");
   WASM_ASSERT(dummy13->c == DUMMY_MESSAGE_DEFAULT_C, "dummy13->c == DUMMY_MESSAGE_DEFAULT_C");

   return WASM_TEST_PASS;
}

unsigned int test_message::read_message_to_0() {
   uint32_t total = readMessage((void *)0, 0x7FFFFFFF);
   return WASM_TEST_PASS;
}

unsigned int test_message::read_message_to_64k() {
   uint32_t total = readMessage( (void *)((1<<16)-1), 0x7FFFFFFF);
   return WASM_TEST_PASS;
}

unsigned int test_message::require_notice() {
   if( currentCode() == N(testapi) ) {
      eos::requireNotice( N(acc1) );
      eos::requireNotice( N(acc2) );
      eos::requireNotice( N(acc1), N(acc2) );
      return WASM_TEST_FAIL;
   } else if ( currentCode() == N(acc1) || currentCode() == N(acc2) ) {
      return WASM_TEST_PASS;
   }
   return WASM_TEST_FAIL;
}

unsigned int test_message::require_auth() {
   eos::requireAuth( N(acc3) );
   eos::requireAuth( N(acc4) );
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
   uint32_t total = readMessage(&tmp, sizeof(uint32_t));
   WASM_ASSERT( total == sizeof(uint32_t), "total == sizeof(uint32_t)");
   WASM_ASSERT( tmp == ::now(), "tmp == now()" );
   return WASM_TEST_PASS;
}

