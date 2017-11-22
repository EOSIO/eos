/**
* @file account.hpp
* @copyright defined in eos/LICENSE.txt
* @brief Defines types and ABI for account API interactions
*
*/
#pragma once
#include <eoslib/account.h>
#include <eoslib/print.hpp>


namespace eosio { namespace account {
  /**
  *  @defgroup account Account API
  *  @brief Defines the ABI for interfacing with account balances.
  *  @ingroup contractdev
  *
  * @{
  */

/**
*  @struct eosio::account_balance (must match account_balance defined in wasm_interface.cpp)
*  @brief The binary structure expected and populated by native balance function.
*  @ingroup account
*
*  @details
*  Example:
*  @code
*  account_balance test1_balance;
*  test1_balance.account = N(test1);
*  if (account_api::get(test1_balance))
*  {
*     eosio::print("test1 balance=", test1_balance.eos_balance, "\n");
*  }
*  @endcode
*  @{
*/
struct PACKED (account_balance) {
  /**
  * Name of the account who's balance this is
  */
  account_name account;

  /**
  * Balance for this account
  */
  asset eos_balance;

  /**
  * Staked balance for this account
  */
  asset staked_balance;

  /**
  * Unstaking balance for this account
  */
  asset unstaking_balance;

  /**
  * Time at which last unstaking occurred for this account
  */
  time last_unstaking_time;
};

/**
 *  Retrieve a populated balance structure
 *  @param account_balance    stream to write
 *  @ret true if account's balance is found
 */

bool get(account_balance& b)
{
   return account_balance_get(&b, sizeof(account_balance));
}

} }
