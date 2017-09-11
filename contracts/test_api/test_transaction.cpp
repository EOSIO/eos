#include <eoslib/transaction.hpp>
#include <eoslib/message.hpp>

#include "test_api.hpp"

unsigned int test_transaction::send_message() {
   dummy_message payload = {DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   auto msg = messageCreate(N(testapi), WASM_TEST_ACTION("test_message", "read_message"), &payload, sizeof(dummy_message));
   messageSend(msg);
   return WASM_TEST_PASS;
}

unsigned int test_transaction::send_message_empty() {
   auto msg = messageCreate(N(testapi), WASM_TEST_ACTION("test_message", "assert_true"), nullptr, 0);
   messageSend(msg);
   return WASM_TEST_PASS;
}

/**
 * cause failure due to too many pending inline messages
 */
unsigned int test_transaction::send_message_max() {
   dummy_message payload = {DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   for (int i = 0; i < 10; i++) {
      messageCreate(N(testapi), WASM_TEST_ACTION("test_message", "read_message"), &payload, sizeof(dummy_message));
   }

   return WASM_TEST_FAIL;
}

/**
 * cause failure due to a large message payload
 */
unsigned int test_transaction::send_message_large() {
   char large_message[8 * 1024];
   messageCreate(N(testapi), WASM_TEST_ACTION("test_message", "read_message"), large_message, sizeof(large_message));
   return WASM_TEST_FAIL;
}

/**
 * cause failure due recursive loop
 */
unsigned int test_transaction::send_message_recurse() {
   char buffer[1024];
   uint32_t size = readMessage(buffer, 1024);
   auto msg = messageCreate(N(testapi), WASM_TEST_ACTION("test_transaction", "send_message_recurse"), buffer, size);
   messageSend(msg);
   return WASM_TEST_PASS;
}

/**
 * cause failure due to inline TX failure
 */
unsigned int test_transaction::send_message_inline_fail() {
   auto msg = messageCreate(N(testapi), WASM_TEST_ACTION("test_message", "assert_false"), nullptr, 0);
   messageSend(msg);
   return WASM_TEST_PASS;
}

unsigned int test_transaction::send_transaction() {
   dummy_message payload = {DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   auto msg = messageCreate(N(testapi), WASM_TEST_ACTION("test_message", "read_message"), &payload, sizeof(dummy_message));
   

   auto trx = transactionCreate();
   transactionRequireScope(trx, N(testapi));
   transactionAddMessage(trx, msg);
   transactionSend(trx);
   return WASM_TEST_PASS;
}

unsigned int test_transaction::send_transaction_empty() {
   auto trx = transactionCreate();
   transactionRequireScope(trx, N(testapi));
   transactionSend(trx);
   return WASM_TEST_FAIL;
}

/**
 * cause failure due to too many pending deferred transactions
 */
unsigned int test_transaction::send_transaction_max() {
   for (int i = 0; i < 10; i++) {
      transactionCreate();
   }

   return WASM_TEST_FAIL;
}

/**
 * cause failure due to a large transaction size
 */
unsigned int test_transaction::send_transaction_large() {
   auto trx = transactionCreate();
   transactionRequireScope(trx, N(testapi));
   for (int i = 0; i < 32; i ++) {
      char large_message[4 * 1024];
      auto msg = messageCreate(N(testapi), WASM_TEST_ACTION("test_message", "read_message"), large_message, sizeof(large_message));
      transactionAddMessage(trx, msg);
   }

   transactionSend(trx);
   return WASM_TEST_FAIL;
}

