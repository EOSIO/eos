/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/types.h>

extern "C" {
   /**
    * @defgroup transactionapi Transaction API
    * @ingroup contractdev
    * @brief Define API for sending transactions and inline messages
    *
    * A EOS.IO transaction has the following abstract structure:
    *
    * ```
    *   struct Transaction {
    *     Name scope[]; ///< accounts whose data may be read or written
    *     Name readScope[]; ///< accounts whose data may be read
    *     Message messages[]; ///< accounts that have approved this message
    *   };
    * ```
    * 
    * This API enables your contract to construct and send transactions
    *
    * Deferred transactions will not be processed until a future block.  They
    * can therefore have no effect on the success of failure of their parent 
    * transaction so long as they appear well formed.  If any other condition
    * causes the parent transaction to be marked as failing, then the deferred
    * transaction will never be processed. 
    *
    * Deferred transactions must adhere to the permissions available to the 
    * parent transaction or, in the future, delegated to the contract account 
    * for future use.
    * 
    * An inline message allows one contract to send another contract a message
    * which is processed immediately after the current message's processing
    * ends such that the success or failure of the parent transaction is 
    * dependent on the success of the message. If an inline message fails in 
    * processing then the whole tree of transactions and messages rooted in the
    * block will me marked as failing and none of effects on the database will
    * persist.  
    *
    * Because of this and the parallel nature of transaction application, 
    * inline messages may not affect any `scope` which is not listed in 
    * their parent transaction's `scope`.  They also may not read any `scope`
    * not listed in either their parent transaction's `scope` or `readScope`.
    *
    * Inline messages and Deferred transactions must adhere to the permissions
    * available to the parent transaction or, in the future, delegated to the 
    * contract account for future use.
    */

   /** 
    * @defgroup transactioncapi Transaction C API
    * @ingroup transactionapi
    * @brief Define API for sending transactions 
    *
    * @{
    */

   typedef uint32_t TransactionHandle;
   #define InvalidTransactionHandle (0xFFFFFFFFUL)
   
   typedef uint32_t MessageHandle;
   #define InvalidMessageHandle (0xFFFFFFFFUL)

   /**
    * @brief create a pending transaction
    *
    * This function allocates a native transaction and returns a handle to the 
    * caller.  Within the context of a single transaction there is a soft limit
    * to the number of pending transactions that can be open at a time.  This
    * limit is at the descretion of the Producers and exceding it will result
    * in the current transaction context failing. 
    * 
    * @return handle used to refer to this transaction in future API calls
    */
   TransactionHandle transactionCreate();

   /**
    * @brief require a scope to process a pending transaction
    *
    * This function adds either a `scope` or `readScope` to the given pending
    * transaction.
    *
    * @param trx - the `TransactionHandle` of the pending transaction to modify
    * @param scope - the `AccountName` to add  
    * @param readOnly - whether `scope` should be added to `scope[]` or `readScope[]`
    */
   void transactionRequireScope(TransactionHandle trx, AccountName scope, int readOnly = 0);

   /**
    * @brief finalize the pending message and add it to the transaction
    *
    * The message handle should be considered invalid after the call
    *
    * @param trx - the `TransactionHandle` of the pending transaction to modify
    * @param msg - the `MessageHandle` of the pending message to add
    */
   void transactionAddMessage(TransactionHandle trx, MessageHandle msg);


   /**
    * @brief send a pending transaction
    *
    * This function will finalize a pending transaction and enqueue it for 
    * processing either immediately after this transaction and its previously
    * sent inline transactions complete OR in a future block. 
    * 
    * This handle should be considered invalid after the call
    *
    * @param trx - the `TransactionHandle` of the pending transaction to send
    */
   void transactionSend(TransactionHandle trx);

   /**
    * @brief drop a pending transaction
    *
    * This function will discard a pending transaction and its associated
    * resources.  Once a transaction is sent it can no longer be dropped.
    * 
    * This handle should be considered invalid after the call
    *
    * @param trx - the `TransactionHandle` of the pending transaction to send
    */
   void transactionDrop(TransactionHandle trx);
   

   /**
    * @brief create a pending message 
    *
    * This function creates a pending message to be included in a deferred
    * transaction or to be send as an inline message
    * 
    * This message has no default permissions, see @ref messageRequirePermission
    *
    * @param code - the `AccountName` which owns the contract code to execute
    * @param type - the type of this message
    * @param data - the payload data for this message
    * @param size - the size of `data`
    */
   MessageHandle messageCreate(AccountName code, FuncName type, void const* data, int size);

   /**
    * @brief require a permission for the pending message
    *
    * Indicates that a given pending message requires a certain permission
    *
    * @param msg - the `MessageHandle` pending message referred to
    * @param account - the `AccountName` to of the permission
    * @param permission - the `PermissionName` to of the permision
    */
   void messageRequirePermission(MessageHandle msg, AccountName account, PermissionName permission);


   /**
    * @brief send a pending message as an inline message
    *
    * This handle should be considered invalid after the call
    *
    * @param msg - the `MessageHandle` of the pending message to send inline
    */
   void messageSend(MessageHandle msg);


   /**
    * @brief discard a pending message
    *
    * This handle should be considered invalid after the call
    *
    * @param trx - the `MessageHandle` of the pending message to discard
    */
   void messageDrop(MessageHandle msg);


   ///@ } transactioncapi
}
