/**
 *  @file test_account.cpp
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include <eoslib/account.hpp>
#include <eoslib/system.h>

#include "test_api.hpp"

/// must match #define in eos/types/asset.hpp
#define EOS_SYMBOL  (int64_t(4) | (uint64_t('E') << 8) | (uint64_t('O') << 16) | (uint64_t('S') << 24))

void test_account::test_balance_acc1() {
   eosio::print("test_balance_acc1\n");
   eosio::account::account_balance b;
   
/*
   b.account = N(acc1);
   assert(eosio::account::get(b), "account_balance_get should have found \"acc1\"");

   assert(b.account == N(acc1), "account_balance_get should not change the account");
   assert(b.eos_balance.amount == 24, "account_balance_get should have set a balance of 24");
   assert(b.eos_balance.symbol == EOS_SYMBOL, "account_balance_get should have a set a balance symbol of EOS");
*/

   /*
   assert(b.staked_balance.amount == 23, "account_balance_get should have set a staked balance of 23");
   assert(b.staked_balance.symbol == EOS_SYMBOL, "account_balance_get should have a set a staked balance symbol of EOS");
   assert(b.unstaking_balance.amount == 14, "account_balance_get should have set a unstaking balance of 14");
   assert(b.unstaking_balance.symbol == EOS_SYMBOL, "account_balance_get should have a set a unstaking balance symbol of EOS");
   assert(b.last_unstaking_time == 55, "account_balance_get should have set a last unstaking time of 55");
   */
}

/*
extern "C" {
	void init() {
	}

	void apply( unsigned long long code, unsigned long long action ) {
		WASM_TEST_HANDLER(test_account, test_balance_acc1);
	}
}
*/
