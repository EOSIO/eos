/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/transaction.hpp>
#include <eosiolib/action.hpp>

#include "test_api.hpp"

unsigned int test_transaction::send_message() {
   dummy_action payload = {DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), &payload, sizeof(dummy_action));
   message_send(msg);
   return WASM_TEST_PASS;
}

unsigned int test_transaction::send_message_empty() {
   auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "assert_true"), nullptr, 0);
   message_send(msg);
   return WASM_TEST_PASS;
}

/**
 * cause failure due to too many pending inline messages
 */
unsigned int test_transaction::send_message_max() {
   dummy_action payload = {DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   for (int i = 0; i < 10; i++) {
      message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), &payload, sizeof(dummy_action));
   }

   return WASM_TEST_FAIL;
}

/**
 * cause failure due to a large message payload
 */
unsigned int test_transaction::send_message_large() {
   char large_message[8 * 1024];
   message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), large_message, sizeof(large_message));
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
   return WASM_TEST_PASS;
}

/**
 * cause failure due to inline TX failure
 */
unsigned int test_transaction::send_message_inline_fail() {
   auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "assert_false"), nullptr, 0);
   message_send(msg);
   return WASM_TEST_PASS;
}

unsigned int test_transaction::send_transaction() {
   dummy_action payload = {DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), &payload, sizeof(dummy_action));
   

   auto trx = transaction_create();
   transaction_require_scope(trx, N(testapi));
   transaction_add_message(trx, msg);
   transaction_send(trx);
   return WASM_TEST_PASS;
}

unsigned int test_transaction::send_transaction_empty() {
   auto trx = transaction_create();
   transaction_require_scope(trx, N(testapi));
   transaction_send(trx);
   return WASM_TEST_FAIL;
}

/**
 * cause failure due to too many pending deferred transactions
 */
unsigned int test_transaction::send_transaction_max() {
   for (int i = 0; i < 10; i++) {
      transaction_create();
   }

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
   return WASM_TEST_FAIL;
}

