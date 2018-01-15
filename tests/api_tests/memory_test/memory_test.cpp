/**
 * @file extended_memory_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include "../test_api.hpp"

#include "test_memory.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER( test_memory, test_memory_allocs );
		WASM_TEST_HANDLER( test_memory, test_memory_hunk );
		WASM_TEST_HANDLER( test_memory, test_memory_hunks );
		WASM_TEST_HANDLER( test_memory, test_memory_hunks_disjoint );
		WASM_TEST_HANDLER( test_memory, test_memset_memcpy );
		WASM_TEST_HANDLER( test_memory, test_memcpy_overlap_start );
		WASM_TEST_HANDLER( test_memory, test_memcpy_overlap_end );
		WASM_TEST_HANDLER( test_memory, test_memcmp );
	}
}
