/**
 * @file print_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include "../test_api.hpp"

#include "test_print.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER( test_print, test_prints );
		WASM_TEST_HANDLER( test_print, test_printi );
		WASM_TEST_HANDLER( test_print, test_printi128 );
		WASM_TEST_HANDLER( test_print, test_printn );
	}
}
