/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>
#include <eoslib/reflect.hpp>

#include <currency/transfer.hpp>
#include <eosio.system/transfer.hpp>

namespace bancor {

  /**
   *  @defgroup eosio.system EOSIO System Contract
   *  @brief Defines the wasm components of the system contract
   *
   */

   /**
   * We create the native EOSIO token type
   */
   typedef eosio::token<uint64_t,N(bnt)> bancor_tokens;
   const account_name bancor_code       = N(bancor);
   const table_name   account_table     = N(account);
   const table_name   bancor_supply     = N(supply);

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
      bancor_tokens      quantity;
   };

   struct transfer_memo : public transfer {
      string memo;
   };

   struct transfer_args {
      name       to_token_type;
      uint64_t   min_to_token;
   };



   /**
    *   This table is used to track an individual user's token balance and vote stake. 
    *
    *
    *   Location:
    *
    *   { 
    *     code: system_code
    *     scope: ${owner_account_name}
    *     table: N(singlton)
    *     key: N(account)
    *   }
    */
   struct account {
      /**
      Constructor with default zero quantity (balance).
      */
      account( bancor_tokens b = bancor_tokens() ):balance(b){}

      /**
       *  The key is constant because there is only one record per scope/currency/accounts
       */
      const uint64_t     key = N(account);

      /**
      * Balance number of tokens in account
      **/
      bancor_tokens     balance;

      /**
      Method to check if accoutn is empty.
      @return true if account balance is zero.
      **/
      bool  is_empty()const  { return balance.quantity == 0; }
   };

} /// bancor

EOSLIB_REFLECT( bancor::transfer_args, (to_token_type)(min_to_token) )
EOSLIB_REFLECT( bancor::account, (balance) )
EOSLIB_REFLECT( bancor::transfer, (from)(to)(quantity) )
EOSLIB_REFLECT_DERIVED( bancor::transfer_memo, (bancor::transfer), (memo) )
