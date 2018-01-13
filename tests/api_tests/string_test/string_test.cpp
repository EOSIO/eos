/**
 * @file extended_memory_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include "../test_api.hpp"

#include "test_string.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER( test_string, construct_with_size );
		WASM_TEST_HANDLER( test_string, construct_with_data );
		WASM_TEST_HANDLER( test_string, construct_with_data_partially );
		WASM_TEST_HANDLER( test_string, construct_with_data_copied );
		WASM_TEST_HANDLER( test_string, copy_constructor );
		WASM_TEST_HANDLER( test_string, assignment_operator );
		WASM_TEST_HANDLER( test_string, index_operator );
		WASM_TEST_HANDLER( test_string, index_out_of_bound );
		WASM_TEST_HANDLER( test_string, substring );
		WASM_TEST_HANDLER( test_string, substring_out_of_bound );
		WASM_TEST_HANDLER( test_string, concatenation_null_terminated );
		WASM_TEST_HANDLER( test_string, concatenation_non_null_terminated );
		WASM_TEST_HANDLER( test_string, assign );
		WASM_TEST_HANDLER( test_string, comparison_operator );
		WASM_TEST_HANDLER( test_string, print_null_terminated );
		WASM_TEST_HANDLER( test_string, print_non_null_terminated );
		WASM_TEST_HANDLER( test_string, print_unicode );
		WASM_TEST_HANDLER( test_string, string_literal );
		/*
		WASM_TEST_HANDLER( test_string, valid_utf8 );
		WASM_TEST_HANDLER( test_string, invalid_utf8 );
		*/
	}
}
