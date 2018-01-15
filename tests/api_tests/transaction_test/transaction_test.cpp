/**
 * @file extended_memory_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include "../test_api.hpp"

#include "test_transaction.cpp"

extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER( test_transaction, send_message );
		WASM_TEST_HANDLER( test_transaction, send_message_empty );
	}
}
