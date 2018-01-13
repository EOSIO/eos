/**
 * @file extended_memory_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include "../test_api.hpp"

#include "test_extended_memory.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER( test_extended_memory, test_initial_buffer );
		WASM_TEST_HANDLER( test_extended_memory, test_page_memory );
		WASM_TEST_HANDLER( test_extended_memory, test_page_memory_exceeded );
		WASM_TEST_HANDLER( test_extended_memory, test_page_memory_negative_bytes );
	}
}
