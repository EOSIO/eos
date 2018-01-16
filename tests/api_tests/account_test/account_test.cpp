/**
 * @file account_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include "../test_api.hpp"
#include "test_account_sc.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER(test_account, test_balance_acc1);
	}
}
