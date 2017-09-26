#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>

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
    typedef eos::token<uint64_t, N(storage)> StorageTokens;

   /**
    * Transfer requires that the sender and receiver be the first two 
    * accounts notified and that the sender has provided authorization.
    */

   struct Transfer {
        /**
        * Account to transfer from
        */
        AccountName from;

        /**
        * Account to transfer to
        */
        AccountName to;

        /**
        * quantity to transfer
        */
        StorageTokens quantity; 
   };

   /**
    *  @brief row in Account table stored within each scope
    */
   struct PACKED(Account) {
      /**
      Constructor with default zero quantity (balance).
      */
      Account( StorageTokens b = StorageTokens() ):balance(b),quotaused(0){}

      /**
       *  The key is constant because there is only one record per scope/currency/accounts
       */
      const uint64_t key = N(account);

      /**
      * Balance number of tokens in account
      **/
      StorageTokens balance;

      /**
      * Quota of storage Used
      **/
      uint64_t quotaused;
      /**
      Method to check if accoutn is empty.
      @return true if account balance is zero and there is quota used.
      **/
      bool  isEmpty()const  { return balance.quantity == 0 && quotaused == 0; }
   };

   /**
   Assert statement to verify structure packing for Account
   **/ 
   static_assert( sizeof(Account) == sizeof(uint64_t)+sizeof(StorageTokens)+sizeof(uint64_t), "unexpected packing" ); 

   /**
   Defines the database table for Account information
   **/
   using Accounts = Table<N(storage),N(storage),N(account),Account,uint64_t>;

   /**
    *  Accounts information for owner is stored:
    *
    *  owner/TOKEN_NAME/account/account -> Account
    *
    *  This API is made available for 3rd parties wanting read access to
    *  the users balance. If the account doesn't exist a default constructed
    *  account will be returned.
    *  @param owner The account owner
    *  @return Account instance
    */
   inline Account getAccount( AccountName owner ) {
      Account account;
      Accounts::get( account, owner );
      return account;
   }

   /**
    * This API is made available for 3rd parties wanting read access to
    * the users quota of storage used. IF the account doesn't exist a default
    * constructed account will be returned.
    * @param owner The account owner
    * @return uint64_t quota used
    */
   inline uint64_t getQuotaUsed( AccountName owner ) {
      Account account;
      Accounts::get( account, owner );
      return account.quotaused;
   }

  /**
   Used to set a link to a file
  **/
  struct PACKED( Link ) {
      /**
      * account owner
      **/
      AccountName owner;

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
      AccountName producer;
  };
} /// @} /// storageapi
