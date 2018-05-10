/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/transaction.h>
#include <eosiolib/action.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/serialize.hpp>
#include <vector>

namespace eosio {

   /**
    * @defgroup transactioncppapi Transaction C++ API
    * @ingroup transactionapi
    * @brief Defines type-safe C++ wrappers for sending transactions
    *
    * @note There are some methods from the @ref transactioncapi that can be used directly from C++
    *
    * @{
    */

   /**
    * The transaction header contains the fixed-sized data associated with each transaction. It is separated from
    * the transaction body to facilitate partial parsing of transactions without requiring dynamic memory allocation.
    * 
    * @brief Fixed size header of a transaction
    * 
    */
   class transaction_header {
   public:

      /**
       * Construct a new transaction header from expiration time and region
       * 
       * @brief Construct a new transaction header object
       * @param exp - Expiration time of the transaction
       * @param r - Region where this transaction will be executed
       */
      transaction_header( time exp = now() + 60, region_id r = 0 )
         :expiration(exp),region(r)
      {}
      
      /**
       * The time at which a transaction expires
       * 
       * @brief The time at which a transaction expires
       */
      time            expiration;

      /**
       * The computational memory region this transaction applies to.
       * 
       * @brief The computational memory region this transaction applies to.
       */
      region_id       region;

      /**
       * Specifies a block num in the last 2^16 blocks.
       * 
       * @brief Specifies a block num in the last 2^16 blocks.
       */
      uint16_t        ref_block_num;

      /**
       * Specifies the lower 32 bits of the blockid at get_ref_blocknum
       * 
       * @brief Specifies the lower 32 bits of the blockid at get_ref_blocknum
       */
      uint32_t        ref_block_prefix;

      /**
       * Upper limit on total network bandwidth (in 8 byte words) billed for this transaction
       * 
       * @brief Upper limit on total network bandwidth (in 8 byte words) billed for this transaction
       */
      unsigned_int    net_usage_words = 0UL; 

      /**
       * Upper limit on the total number of kilo CPU usage units billed for this transaction
       * 
       * @brief Upper limit on the total number of kilo CPU usage units billed for this transaction
       */
      unsigned_int    kcpu_usage = 0UL;

      /**
       * Number of seconds to delay this transaction for during which it may be canceled.
       * 
       * @brief Number of seconds to delay this transaction for during which it may be canceled.
       */
      unsigned_int    delay_sec = 0UL;

      EOSLIB_SERIALIZE( transaction_header, (expiration)(region)(ref_block_num)(ref_block_prefix)(net_usage_words)(kcpu_usage)(delay_sec) )
   };

   /**
    * A transaction consists of a set of actions and context free actions which must all be applied or all are rejected. 
    * 
    * @brief A transaction consists of a set of actions and context free actions which must all be applied or all are rejected. 
    */

   class transaction : public transaction_header {
   public:
      
      transaction(time exp = now() + 60, region_id r = 0) : transaction_header( exp, r ) {}

      /**
       * Send this transaction. Since this transaction will need to be keep before its execution in the future, there is a need for storage, 
       * abd we need to specify who is responsible as the payer. This storage will be 
       * released when the transaction executed later on.
       * 
       * @brief Send the deferred transaction
       * @param sender_id - The identifier to identify this deferred transaction later on
       * @param payer - The account responsible to pay for the transaction storage
       */
      void send(uint64_t sender_id, account_name payer) const {
         auto serialize = pack(*this);
         send_deferred(sender_id, payer, serialize.data(), serialize.size());
      }

      /**
       * List of context free actions
       * 
       * @brief List of context free actions
       */
      vector<action>  context_free_actions;

       /**
       * List of actions
       * 
       * @brief List of actions
       */
      vector<action>  actions;

      EOSLIB_SERIALIZE_DERIVED( transaction, transaction_header, (context_free_actions)(actions) )
   };
 
    /**
     * A transaction that will be executed in the future
     * 
     * @brief A transaction that will be executed in the future
     */
   class deferred_transaction : public transaction {
      public:
         /**
          * Identifier for the deferred transaction
          * 
          * @brief Identifier for the deferred transaction
          */
         uint128_t     sender_id;

         /**
          *  Sender of the deferred transaction
          * 
          * @brief Sender of the deferred transaction
          */
         account_name  sender;

         /**
          * The account responsible to pay for the temporary storage for the deferred transaction
          * 
          * @brief The account responsible to pay for the temporary storage for the deferred transaction
          */
         account_name  payer;

         /**
          * Delay before the transaction can be executed
          * 
          * @brief Delay before the transaction can be executed
          */
         time          execute_after;

         /**
          * Unpack current action as defrred transaction
          * 
          * @brief Unpack current action as defrred transaction
          * @return deferred_transaction - The unpacked deferred transaction
          */
         static deferred_transaction from_current_action() {
            return unpack_action_data<deferred_transaction>();
         }

         EOSLIB_SERIALIZE_DERIVED( deferred_transaction, transaction, (sender_id)(sender)(payer)(execute_after) )
   };

   /**
    * Retrieve the indicated action from the active transaction.
    * 
    * @brief Retrieve the indicated action from the active transaction.
    * @param type - 0 for context free action, 1 for action
    * @param index - the index of the requested action
    * @return the indicated action
    */
   inline action get_action( uint32_t type, uint32_t index ) {
      auto size = ::get_action(type, index, nullptr, 0);
      eosio_assert( size > 0, "get_action size failed" );
      char buf[size];
      auto size2 = ::get_action(type, index, &buf[0], static_cast<size_t>(size) );
      eosio_assert( size == size2, "get_action failed" );
      return eosio::unpack<eosio::action>(&buf[0], static_cast<size_t>(size));
   }

   /**
    * Check if the given list of permissions satisfy the packed transaction 
    * 
    * @brief Check if the given list of permissions satisfy the packed transaction 
    * @param trx_packed - The packed transaction to be checked
    * @param permissions - The list of permissions to check the transaction with
    */
   inline void check_auth(const bytes& trx_packed, const vector<permission_level>& permissions) {
      auto perm_packed = pack(permissions);
      ::check_auth( trx_packed.data(), trx_packed.size(), perm_packed.data(), perm_packed.size() );
   }

   /**
    * Check if the given list of permissions satisfy the serialized transaction 
    * 
    * @brief Check if the given list of permissions satisfy the serialized transaction 
    * @param serialized_transaction - The pointer to the serialized transaction
    * @param size - The size of the serialized transaction
    * @param permissions - The list of permissions to check the transaction with
    */
   inline void check_auth(const char *serialized_transaction, size_t size, const vector<permission_level>& permissions) {
      auto perm_packed = pack(permissions);
      ::check_auth( serialized_transaction, size, perm_packed.data(), perm_packed.size() );
   }

   /**
    * Check if the given list of permissions satisfy the transaction 
    * 
    * @brief Check if the given list of permissions satisfy the transaction 
    * @param trx - The transaction to be checked
    * @param permissions - The list of permissions to check the transaction with
    */
   inline void check_auth(const transaction& trx, const vector<permission_level>& permissions) {
      auto trx_packed = pack(trx);
      check_auth( trx_packed, permissions );
      //return res > 0;
   }

   ///@} transactioncpp api

} // namespace eos
