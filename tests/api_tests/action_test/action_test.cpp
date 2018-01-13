/**
 * @file action_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include "../test_api.hpp"
#include "test_action.cpp"

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
