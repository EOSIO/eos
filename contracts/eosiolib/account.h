/**
 *  @file account.h
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/types.h>

extern "C" {
   /**
    * @brief Retrieve the balance for the provided account
    * @details Retrieve the balance for the provided account
    *
    * @param balance -  a pointer to a range of memory to store balance data
    * @param len     -  length of the range of memory to store balance data
    * @return true if account information is retrieved
    *
    * @pre `balance` is a valid pointer to a range of memory at least datalen bytes long
    * @pre `balance` is a pointer to a balance object
    * @pre *((uint64_t*)balance) stores the primary key
    *
    *  Example:
    *  @code
    *  balance b;
    *  b.account = N(myaccount);
    *  balance(b, sizeof(balance));
    *  @endcode
    *  
    *  @deprecated This is deprecated in Dawn 3.0
    */

   bool account_balance_get( void* balance, uint32_t len );
}
