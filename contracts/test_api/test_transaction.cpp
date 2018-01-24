/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/transaction.hpp>
#include <eoslib/action.hpp>

#include "test_api.hpp"

#define WASM_TEST_FAIL 1

void test_transaction::send_action() {
   dummy_action payload = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   //auto msg = message_create(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), &payload, sizeof(dummy_action));

   action<> act(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), payload);
   char act_buff[sizeof(act)];
   act.pack(act_buff, sizeof(act_buff));
   send_inline(act_buff, sizeof(act_buff));
}

void test_transaction::send_action_empty() {
   action<> act(N(testapi), WASM_TEST_ACTION("test_action", "assert_true"), nullptr);
   char act_buff[sizeof(act)];
   act.pack(act_buff, sizeof(act_buff));
   send_inline(act_buff, sizeof(act_buff));
}

/**
 * cause failure due to a large action payload
 */
void test_transaction::send_action_large() {
   char large_message[8 * 1024];
   action<> act(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), large_message);
   assert(false, "send_message_large() should've thrown an error");
}

/**
 * cause failure due recursive loop
 */
void test_transaction::send_action_recurse() {
   char buffer[1024];
   uint32_t size = read_action(buffer, 1024);
   action<> act(N(testapi), WASM_TEST_ACTION("test_transaction", "send_message_recurse"), buffer);
   char act_buff[sizeof(act)];
   act.pack(act_buff, sizeof(act_buff));
   send_inline(act_buff, sizeof(act_buff));
}

/**
 * cause failure due to inline TX failure
 */
void test_transaction::send_action_inline_fail() {
   action<> act(N(testapi), WASM_TEST_ACTION("test_action", "assert_false"), nullptr);
   char act_buff[sizeof(act)];
   act.pack(act_buff, sizeof(act_buff));
   send_inline(act_buff, sizeof(act_buff));
}

void test_transaction::send_transaction() {
   dummy_action payload = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   action<> act(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), payload);
   
   auto trx = transaction<256, 2, 4>();
   trx.add_read_scope(N(testapi));
   trx.add_action(act);
   trx.send(0);
}

void test_transaction::send_transaction_empty() {
   auto trx = transaction<256, 2, 4>();
   trx.add_read_scope(N(testapi));
   trx.send(0);

   assert(false, "send_transaction_empty() should've thrown an error");
}

/**
 * cause failure due to a large transaction size
 */
void test_transaction::send_transaction_large() {
   auto trx = transaction<256, 2, 4>();
   trx.add_read_scope(N(testapi));
   for (int i = 0; i < 32; i ++) {
      char large_message[1024];
      action<> act(N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal"), large_message);
      trx.add_action(act);
   }

   trx.send(0);

   assert(false, "send_transaction_large() should've thrown an error");
}
