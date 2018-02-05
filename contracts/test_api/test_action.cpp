/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/action.hpp>

#include "test_api.hpp"

void test_action::read_action_normal() {

   char buffer[100];
   uint32_t total = 0;

   assert( current_receiver() == N(testapi),  "current_receiver() == N(testapi)" );

   assert(action_size() == sizeof(dummy_action), "action_size() == sizeof(dummy_action)");

   total = read_action(buffer, 30);
   assert(total == sizeof(dummy_action) , "read_action(30)" );

   total = read_action(buffer, 100);
   assert(total == sizeof(dummy_action) , "read_action(100)" );

   total = read_action(buffer, 5);
   assert(total == 5 , "read_action(5)" );

   total = read_action(buffer, sizeof(dummy_action) );
   assert(total == sizeof(dummy_action), "read_action(sizeof(dummy_action))" );

   dummy_action *dummy13 = reinterpret_cast<dummy_action *>(buffer);
   assert(dummy13->a == DUMMY_MESSAGE_DEFAULT_A, "dummy13->a == DUMMY_MESSAGE_DEFAULT_A");
   assert(dummy13->b == DUMMY_MESSAGE_DEFAULT_B, "dummy13->b == DUMMY_MESSAGE_DEFAULT_B");
   assert(dummy13->c == DUMMY_MESSAGE_DEFAULT_C, "dummy13->c == DUMMY_MESSAGE_DEFAULT_C");

}

void test_action::read_action_to_0() {
   uint32_t total = read_action((void *)0, 0x7FFFFFFF);
}

void test_action::read_action_to_64k() {
   uint32_t total = read_action( (void *)((1<<16)-1), 0x7FFFFFFF);
}

//unsigned int test_action::require_notice() {
//   if( current_receiver() == N(testapi) ) {
//      eosio::require_recipient( N(acc1) );
//      eosio::require_recipient( N(acc2) );
//      eosio::require_recipient( N(acc1), N(acc2) );
//      return WASM_TEST_FAIL;
//   } else if ( current_receiver() == N(acc1) || current_receiver() == N(acc2) ) {
//      return WASM_TEST_PASS;
//   }
//   return WASM_TEST_FAIL;
//}
//
//unsigned int test_action::require_auth() {
//   eosio::require_auth( N(acc3) );
//   eosio::require_auth( N(acc4) );
//   return WASM_TEST_PASS;
//}

void test_action::assert_false() {
   assert(false, "test_action::assert_false");
}

void test_action::assert_true() {
   assert(true, "test_action::assert_true");
}

//unsigned int test_action::now() {
//   uint32_t tmp = 0;
//   uint32_t total = read_action(&tmp, sizeof(uint32_t));
//   WASM_ASSERT( total == sizeof(uint32_t), "total == sizeof(uint32_t)");
//   WASM_ASSERT( tmp == ::now(), "tmp == now()" );
//   return WASM_TEST_PASS;
//}
//
