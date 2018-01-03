/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>

namespace eosiosystem {

  /**
   *  @defgroup eosio.system EOSIO System Contract
   *  @brief Defines the wasm components of the system contract
   *
   *  @{
   */

   /**
   * We create the native EOSIO token type
   */
   typedef eosio::token<uint64_t,N(eosio)> native_tokens;
   const account_name system_code   = N(eosio.system);
   const table_name   account_table = N(account);

   /**
    *  transfer requires that the sender and receiver be the first two
    *  accounts notified and that the sender has provided authorization.
    *  @abi action
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
      native_tokens    quantity;
   };



   /**
    *  @brief row in account table stored within each scope
    *  @abi table
    */
   struct account {
      /**
      Constructor with default zero quantity (balance).
      */
      account( native_tokens b = native_tokens() ):balance(b){}

      /**
       *  The key is constant because there is only one record per scope/currency/accounts
       */
      const uint64_t     key = N(account);

      /**
      * Balance number of tokens in account
      **/
      native_tokens     balance;
      native_tokens     vote_stake;
      native_tokens     proxied_vote_stake;
      uint64_t          last_vote_weight = 0;
      //time_point        last_stake_withdraw;
      account_name      proxy;


      /**
      Method to check if accoutn is empty.
      @return true if account balance is zero.
      **/
      bool  is_empty()const  { return balance.quantity == 0; }
   };

   struct producer {
      account_name key; /// producer name
      uint64_t     votes; /// total votes received by producer
      /// producer config... 
   };

   struct producer_vote {
      account_name voter;
      account_name producer;
      uint64_t     voteweight = 0;
   };

   /**
   Defines the database table for account information
   **/
   using accounts = eosio::table<N(unused),system_code,account_table,account,uint64_t>;

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

} /// @} /// currencyapi
