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
   *  @defgroup currencyapi Currency Contract
   *  @brief Defines the curency contract example
   *  @ingroup examplecontract
   *
   *  @{
   */

   /**
   * Defines a currency token
   */
   typedef eos::token<uint64_t,N(currency)> CurrencyTokens;

   /**
    *  Transfer requires that the sender and receiver be the first two
    *  accounts notified and that the sender has provided authorization.
    */
   struct Transfer {
      /**
      * Account to transfer from
      */
      AccountName       from;
      /**
      * Account to transfer to
      */
      AccountName       to;
      /**
      *  quantity to transfer
      */
      CurrencyTokens    quantity;
   };

   /**
    *  @brief row in Account table stored within each scope
    */
   struct Account {
      /**
      Constructor with default zero quantity (balance).
      */
      Account( CurrencyTokens b = CurrencyTokens() ):balance(b){}

      /**
       *  The key is constant because there is only one record per scope/currency/accounts
       */
      const uint64_t     key = N(account);

      /**
      * Balance number of tokens in account
      **/
      CurrencyTokens     balance;

      /**
      Method to check if accoutn is empty.
      @return true if account balance is zero.
      **/
      bool  isEmpty()const  { return balance.quantity == 0; }
   };

   /**
   Assert statement to verify structure packing for Account
   **/
   static_assert( sizeof(Account) == sizeof(uint64_t)+sizeof(CurrencyTokens), "unexpected packing" );

   /**
   Defines the database table for Account information
   **/
   using Accounts = Table<N(currency),N(currency),N(account),Account,uint64_t>;

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
      ///      scope, record
      Accounts::get( account, owner );
      return account;
   }

} /// @} /// currencyapi
