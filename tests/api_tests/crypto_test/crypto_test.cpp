/**
 * @file action_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include "../test_api.hpp"
#include "test_crypto.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER(test_crypto, test_sha256);
		WASM_TEST_HANDLER(test_crypto, sha256_no_data);
		WASM_TEST_HANDLER(test_crypto, asert_sha256_false);
		WASM_TEST_HANDLER(test_crypto, asert_sha256_true);
		WASM_TEST_HANDLER(test_crypto, asert_no_data);
	}
}
