/**
 * @file types_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include "../test_api.hpp"
#include "test_types.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER(test_types, types_size);
		WASM_TEST_HANDLER(test_types, char_to_symbol);
		WASM_TEST_HANDLER(test_types, string_to_name);
		WASM_TEST_HANDLER(test_types, name_class);
	}
}
