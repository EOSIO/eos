/**
 * @file fixedpoint_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include "../test_api.hpp"

#include "test_fixedpoint.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER( test_fixedpoint, create_instances );
		WASM_TEST_HANDLER( test_fixedpoint, test_addition );
		WASM_TEST_HANDLER( test_fixedpoint, test_subtraction );
		WASM_TEST_HANDLER( test_fixedpoint, test_multiplication );
		WASM_TEST_HANDLER( test_fixedpoint, test_division );
	}
}
