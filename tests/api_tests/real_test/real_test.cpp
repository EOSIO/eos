/**
 * @file extended_memory_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include "../test_api.hpp"

#include "test_real.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER( test_real, create_instances );
		WASM_TEST_HANDLER( test_real, test_addition );
		WASM_TEST_HANDLER( test_real, test_multiplication );
		WASM_TEST_HANDLER( test_real, test_division );
	}
}
