/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eos.hpp>
#include <eosiolib/token.hpp>
#include <eosiolib/db.hpp>

namespace infinite {

   typedef eosio::token<uint64_t,N(currency)> currency_tokens;

   /**
    *  transfer requires that the sender and receiver be the first two
    *  accounts notified and that the sender has provided authorization.
    */
   struct transfer {
      account_name       from;
      account_name       to;
      currency_tokens    quantity;
   };

   /**
    *  @brief row in account table stored within each scope
    */
   struct account {
      account( currency_tokens b = currency_tokens() ):balance(b){}

      /**
       *  The key is constant because there is only one record per scope/currency/accounts
       */
      const uint64_t     key = N(account);
      currency_tokens    balance;

      bool  is_empty()const  { return balance.quantity == 0; }
   };
   static_assert( sizeof(account) == sizeof(uint64_t)+sizeof(currency_tokens), "unexpected packing" );

   using accounts = eosio::table<N(currency),N(currency),N(account),account,uint64_t>;

   /**
    *  accounts information for owner is stored:
    *
    *  owner/infinite/account/account -> account
    *
    *  This API is made available for 3rd parties wanting read access to
    *  the users balance. If the account doesn't exist a default constructed
    *  account will be returned.
    */
   inline account get_account( account_name owner ) {
      account owned_account;
      ///      scope, record
      accounts::get( owned_account, owner );
      return owned_account;
   }

} /// namespace infinite

