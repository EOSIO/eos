/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/message.h>
#include <eoslib/print.hpp>

namespace eosio {

   /**
    * @defgroup messagecppapi Message C++ API
    * @ingroup messageapi
    * @brief Type-safe C++ wrapers for Message C API
    *
    * @note There are some methods from the @ref messagecapi that can be used directly from C++
    *
    * @{
    */

   /**
    *
    *  This method attempts to reinterpret the message body as type T. This will only work
    *  if the message has no dynamic fields and the struct packing on type T is properly defined.
    *
    *  @brief Interpret the message body as type T
    *  
    *  Example:
    *  @code
    *  struct dummy_message {
    *    char a; //1
    *    unsigned long long b; //8
    *    int  c; //4
    *  };
    *  dummy_message msg = current_message<dummy_message>();
    *  @endcode
    */
   template<typename T>
   T current_message() {
      T value;
      auto read = read_message( &value, sizeof(value) );
      assert( read >= sizeof(value), "message shorter than expected" );
      return value;
   }

   using ::require_auth;
   using ::require_notice;

   /**
    *  All of the listed accounts will be added to the set of accounts to be notified
    *
    *  This helper method enables you to add multiple accounts to accounts to be notified list with a single
    *  call rather than having to call the similar C API multiple times.
    *
    *  @note message.code is also considered as part of the set of notified accounts
    *
    *  @brief Verify specified accounts exist in the set of notified accounts
    *
    *  Example:
    *  @code
    *  require_notice(N(Account1), N(Account2), N(Account3)); // throws exception if any of them not in set.
    *  @endcode
    */
   template<typename... Accounts>
   void require_notice( account_name name, Accounts... remaining_accounts ){
      require_notice( name );
      require_notice( remaining_accounts... );
   }


 ///@} messagecpp api

} // namespace eos
