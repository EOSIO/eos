#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>

/**
 * Make it easy to change the account name the currency is deployed to.
 */
#ifndef TOKEN_NAME
#define TOKEN_NAME currency
#endif

namespace TOKEN_NAME {

   typedef eos::token<uint64_t,N(currency)> Tokens;

   /**
    *  Transfer requires that the sender and receiver be the first two
    *  accounts notified and that the sender has provided authorization.
    */
   struct Transfer {
      AccountName       from;
      AccountName       to;
      Tokens            quantity;
   };

   struct Account {
      Tokens           balance;

      bool  isEmpty()const  { return balance.quantity == 0; }
   };

   /**
    *  Accounts information for owner is stored:
    *
    *  owner/TOKEN_NAME/account/account -> Account
    *
    *  This API is made available for 3rd parties wanting read access to
    *  the users balance. If the account doesn't exist a default constructed
    *  account will be returned.
    */
   inline Account getAccount( AccountName owner ) {
      Account account;
      ///      scope, code, table,      key,       value        
      Db::get( owner, N(currency), N(account), N(account), account );
      return account;
   }

} /// namespace TOKEN_NAME

