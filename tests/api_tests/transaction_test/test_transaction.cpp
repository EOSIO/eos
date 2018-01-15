/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/transaction.hpp>
#include <eoslib/action.hpp>

#include "../test_api.hpp"

#define WASM_TEST_FAIL 1

unsigned int test_transaction::send_message() {
   dummy_action payload = {DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), &payload, sizeof(dummy_action));
   message_send(msg);
   return 0;
}

unsigned int test_transaction::send_message_empty() {
   auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "assert_true"), nullptr, 0);
   message_send(msg);
   return 0;
}

/**
 * cause failure due to too many pending inline messages
 */
unsigned int test_transaction::send_message_max() {
   dummy_action payload = {DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   for (int i = 0; i < 10; i++) {
      message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), &payload, sizeof(dummy_action));
   }
   assert(false, "send_message_max() should've thrown an error");
   return WASM_TEST_FAIL;
}

/**
 * cause failure due to a large message payload
 */
unsigned int test_transaction::send_message_large() {
   char large_message[8 * 1024];
   message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), large_message, sizeof(large_message));

   assert(false, "send_message_large() should've thrown an error");
   return WASM_TEST_FAIL;
}

/**
 * cause failure due recursive loop
 */
unsigned int test_transaction::send_message_recurse() {
   char buffer[1024];
   uint32_t size = read_action(buffer, 1024);
   auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_transaction", "send_message_recurse"), buffer, size);
   message_send(msg);
   return 0;
}

/**
 * cause failure due to inline TX failure
 */
unsigned int test_transaction::send_message_inline_fail() {
   auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "assert_false"), nullptr, 0);
   message_send(msg);
   return 0;
}

unsigned int test_transaction::send_transaction() {
   dummy_action payload = {DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), &payload, sizeof(dummy_action));
   

   auto trx = transaction_create();
   transaction_require_scope(trx, N(testapi));
   transaction_add_message(trx, msg);
   transaction_send(trx);
   return 0;
}

unsigned int test_transaction::send_transaction_empty() {
   auto trx = transaction_create();
   transaction_require_scope(trx, N(testapi));
   transaction_send(trx);

   assert(false, "send_transaction_empty() should've thrown an error");
   return WASM_TEST_FAIL;
}

/**
 * cause failure due to too many pending deferred transactions
 */
unsigned int test_transaction::send_transaction_max() {
   for (int i = 0; i < 10; i++) {
      transaction_create();
   }

   assert(false, "send_transaction_max() should've thrown an error");
   return WASM_TEST_FAIL;
}

/**
 * cause failure due to a large transaction size
 */
unsigned int test_transaction::send_transaction_large() {
   auto trx = transaction_create();
   transaction_require_scope(trx, N(testapi));
   for (int i = 0; i < 32; i ++) {
      char large_message[4 * 1024];
      auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), large_message, sizeof(large_message));
      transaction_add_message(trx, msg);
   }

   transaction_send(trx);

   assert(false, "send_transaction_large() should've thrown an error");
   return WASM_TEST_FAIL;
}

