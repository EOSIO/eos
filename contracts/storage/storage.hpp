/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eos.hpp>
//#include <eosiolib/token.hpp>
#include <eosiolib/db.hpp>

/**
 *  @defgroup examplecontract Example Storage
 *  @brief Example Contract
 *  @ingroup contractapi
 *
 */

/**
 * Make it easy to change the account name the storage is deployed to.
 */
#ifndef TOKEN_NAME
#define TOKEN_NAME TOK
#endif

namespace TOKEN_NAME {

    /**
    * @defgroup storageapi Storage Contract
    * @brief Defines the storage contract example
    * @ingroup examplecontract
    *
    * @{
    */

    /**
    * Defines a storage token
    */
    typedef eosio::token<uint64_t, N(storage)> storage_tokens;

   /**
    * transfer requires that the sender and receiver be the first two
    * accounts notified and that the sender has provided authorization.
    */

   struct transfer {
        /**
        * account to transfer from
        */
        account_name from;

        /**
        * account to transfer to
        */
        account_name to;

        /**
        * quantity to transfer
        */
        storage_tokens quantity;
   };

   /**
    *  @brief row in account table stored within each scope
    */
   struct PACKED(account) {
      /**
      Constructor with default zero quantity (balance).
      */
      account( storage_tokens b = storage_tokens() ):balance(b),quotaused(0){}

      /**
       *  The key is constant because there is only one record per scope/currency/accounts
       */
      const uint64_t key = N(account);

      /**
      * Balance number of tokens in account
      **/
      storage_tokens balance;

      /**
      * Quota of storage Used
      **/
      uint64_t quotaused;
      /**
      Method to check if accoutn is empty.
      @return true if account balance is zero and there is quota used.
      **/
      bool  is_empty()const  { return balance.quantity == 0 && quotaused == 0; }
   };

   /**
   Assert statement to verify structure packing for account
   **/
   static_assert( sizeof(account) == sizeof(uint64_t)+sizeof(storage_tokens)+sizeof(uint64_t), "unexpected packing" );

   /**
   Defines the database table for account information
   **/
   using accounts = eosio::table<N(storage),N(storage),N(account),account,uint64_t>;

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
      accounts::get( owned_account, owner );
      return owned_account;
   }

   /**
    * This API is made available for 3rd parties wanting read access to
    * the users quota of storage used. IF the account doesn't exist a default
    * constructed account will be returned.
    * @param owner The account owner
    * @return uint64_t quota used
    */
   inline uint64_t get_quota_used( account_name owner ) {
      account owned_account;
      accounts::get( owned_account, owner );
      return owned_account.quotaused;
   }

  /**
   Used to set a link to a file
  **/
  struct PACKED( link ) {
      /**
      * account owner
      **/
      account_name owner;

      /**
      * eos path
      **/
      char* eospath;

      /**
      * ipfs file path
      **/
      char* ipfspath;

      /**
      * size of file
      **/
      uint32_t size;

      /**
      * a flag that signals producers to cache file
      **/
      uint8_t store;

      /**
      * a flag that signals producers have accepted storage and cached file
      **/
      uint8_t accept;

      /**
      * tokens staked per file
      **/
      uint64_t stake;

      /**
      * name of producer who cached file
      **/
      account_name producer;
  };
} /// @} /// storageapi
