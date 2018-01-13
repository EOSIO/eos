/**
 * @file math_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include "../test_api.hpp"

#include "test_math.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER( test_math, test_multeq );
		WASM_TEST_HANDLER( test_math, test_diveq );
		WASM_TEST_HANDLER( test_math, test_diveq_by_0 );
		WASM_TEST_HANDLER( test_math, test_double_api );
		WASM_TEST_HANDLER( test_math, test_double_api_div_0 );
	}
}
