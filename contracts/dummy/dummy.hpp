/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>

/**
 *  @defgroup examplecontract Example Contract
 *  @brief Example Contract
 *  @ingroup contractapi
 *
 */


/**
 * Make it easy to change the account name the currency is deployed to.
 */
#ifndef TOKEN_NAME
#define TOKEN_NAME currency
#endif

namespace TOKEN_NAME {

  /**
   *  @defgroup dummyapi Malicious Contract
   *  @brief Defines the dummy contract example
   *  @ingroup examplecontract
   *
   *  @{
   */

   /**
   * Defines a currency token
   */
   typedef eosio::token<uint64_t,N(currency)> dummy_tokens;

    /**
       *  transfer requires that the sender and receiver be the first two
       *  accounts notified and that the sender has provided authorization.
       */
    struct transfer {
        /**
        * account to transfer from
        */
        account_name       from;
        /**
        * account to transfer to
        */
        account_name       to;
        /**
        *  quantity to transfer
        */
        dummy_tokens    quantity;
    };
    struct dummy {};
   /**
    *  @brief row in account table stored within each scope
    */
   struct account {
      /**
      Constructor with default zero quantity (balance).
      */
      account( dummy_tokens b = dummy_tokens() ):balance(b){}

      /**
       *  The key is constant because there is only one record per scope/dummy/accounts
       */
      const uint64_t     key = N(account);

      /**
      * Balance number of tokens in account
      **/
      dummy_tokens     balance;

      /**
      Method to check if accoutn is empty.
      @return true if account balance is zero.
      **/
      bool  is_empty()const  { return balance.quantity == 0; }
   };

   /**
   Assert statement to verify structure packing for account
   **/
   static_assert( sizeof(account) == sizeof(uint64_t)+sizeof(dummy_tokens), "unexpected packing" );

   /**
   Defines the database table for account information
   **/
   using accounts = eosio::table<N(dummy),N(dummy),N(account),account,uint64_t>;

   /**
    *  accounts information for owner is stored:
    *
    *  owner/TOKEN_NAME/account/account -> account
    *
    *  This API is made available for 3rd parties wanting read access to
    *  the users balance. If the account doesn't exist a default constructed
    *  account will be returned.
    *  @param owner The account owner
    *  @return account instance
    */
   inline account get_account( account_name owner ) {
      account owned_account;
      ///      scope, record
      accounts::get( owned_account, owner );
      return owned_account;
   }

} /// @} /// dummyapi
