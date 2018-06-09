/**
* @file account.hpp
* @copyright defined in eos/LICENSE.txt
* @brief Defines types and ABI for account API interactions
*
*/
#pragma once
#include <eosiolib/account.h>
#include <eosiolib/print.hpp>
#include <eosiolib/asset.hpp>


namespace eosio { namespace account {

/**
*  @brief The binary structure expected and populated by native balance function.
*  @details
*  Example:
*
*  @code
*  account_balance test1_balance;
*  test1_balance.account = N(test1);
*  if (account_api::get(test1_balance))
*  {
*     eosio::print("test1 balance=", test1_balance.eos_balance, "\n");
*  }
*  @endcode
*
*  @deprecated This is deprecated in Dawn 3.0
*/
struct PACKED(account_balance) {
  /**
  * Name of the account who's balance this is
  * 
  * @brief Name of the account who's balance this is
  */
  account_name account;

  /**
  * Balance for this account
  * 
  * @brief Balance for this account
  */
  asset eos_balance;

  /**
  * Staked balance for this account
  * 
  * @brief Staked balance for this account
  */
  asset staked_balance;

  /**
  * Unstaking balance for this account
  * 
  * @brief Unstaking balance for this account
  */
  asset unstaking_balance;

  /**
  * Time at which last unstaking occurred for this account
  * 
  * @brief Time at which last unstaking occurred for this account
  */
  time last_unstaking_time;
};

/**
 *  Retrieve a populated balance structure
 * 
 *  @brief Retrieve a populated balance structure
 *  @param acnt - account
 *  @return true if account's balance is found
 * 
 *  @deprecated This is deprecated in Dawn 3.0
 */
bool get(account_balance& acnt)
{
   return account_balance_get(&acnt, sizeof(account_balance));
}

} }
