/**
 * @file action_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include "../test_api.hpp"
//#include "test_action.cpp"

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
   prints("made it here\n");
   uint32_t total = read_action((void *)0,  0x7FFFFFFF); //0x7FFFFFFF);
   prints("made it out\n");
//   assert(false, "WHY YOU NO FAIL!");
}

void test_action::read_action_to_64k() {
   uint32_t total = read_action( (void *)((1<<16)-1), 0x7FFFFFFF);
}

unsigned int test_action::require_notice() {
   if( current_receiver() == N(testapi) ) {
      eosio::require_recipient( N(acc1) );
      eosio::require_recipient( N(acc2) );
      eosio::require_recipient( N(acc1), N(acc2) );
      assert(false, "Should've failed");
      return 1;
   } else if ( current_receiver() == N(acc1) || current_receiver() == N(acc2) ) {
      return 0;
   }
   assert(false, "Should've failed");
   return 1;
}

unsigned int test_action::require_auth() {
   prints("require_auth");
   eosio::require_auth( N(acc3) );
   eosio::require_auth( N(acc4) );
   return 0;
}

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

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER(test_action, read_action_normal);
      WASM_TEST_HANDLER(test_action, read_action_to_0);
      WASM_TEST_HANDLER(test_action, require_auth);
		WASM_TEST_HANDLER(test_action, assert_false);
		WASM_TEST_HANDLER(test_action, assert_true);
	}
}
