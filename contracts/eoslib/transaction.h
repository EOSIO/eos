#pragma once
#include <eoslib/types.h>

extern "C" {
   /**
    * @defgroup transactionapi Transaction API
    * @ingroup contractdev
    * @brief Define API for sending transactions to other contracts inline with
    * the current transaction or deferred to a future block.
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
    * A Transaction can be processed immediately after the execution of the 
    * parent transaction (inline).  An inline transaction is more restrictive
    * than a deferred transaction however, it allows the success or failure
    * of the parent transaction to be affected by the new transaction.  In 
    * other words, if an inline transaction fails then the whole tree of 
    * transactions rooted in the block will me marked as failing and none of
    * their affects on the database will persist.  
    *
    * Because of this and the parallel nature of transaction application, 
    * inline transactions may not adopt any `scope` which is not listed in 
    * their parent transaction's scope.  They may adopt any `readScope` listed
    * in either their parent's `scope` or `readScope`.
    *
    * Deferred transactions carry no `scope` or `readScope` restrictions.  They
    * may adopt any valid accounts for either field.
    *
    * Deferred transactions will not be processed until a future block.  They
    * can therefore have no effect on the success of failure of their parent 
    * transaction so long as they appear well formed.  If any other condition
    * causes the parent transaction to be marked as failing, then the deferred
    * transaction will never be processed. 
    *
    * Both Deferred and Inline transactions must adhere to the permissions
    * available to the parent transaction or, in the future, delegated to the
    * contract account for future use.
    * 
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
    * @brief add a scope to a pending transaction
    *
    * This function adds either a `scope` or `readScope` to the given pending
    * transaction.
    *
    * @param trx - the `TransactionHandle` of the pending transaction to modify
    * @param scope - the `AccountName` to add  
    * @param readOnly - whether `scope` should be added to `scope[]` or `readScope[]`
    */
   void transactionAddScope(TransactionHandle trx, AccountName scope, int readOnly = 0);
   
   /**
    * @brief set the destination for the pending message
    *
    * This function sets the `AccountName` of the owner of the contract
    * that is acting as the reciever of the pending message and the type of
    * message;
    *
    * @param trx - the `TransactionHandle` of the pending transaction to modify
    * @param code - the `AccountName` which owns the contract code to execute
    * @param type - the type of this message
    */
   void transactionSetMessageDestination(TransactionHandle trx, AccountName code, FuncName type);

   /**
    * @brief add a permission to the pending message
    *
    * This function adds a @ref PermissionName to the pending message
    *
    * @param trx - the `TransactionHandle` of the pending transaction to modify
    * @param permission - the `PermissionName` to add
    */
   void transactionAddMessagePermission(TransactionHandle trx, PermissionName permission);


   /**
    * @brief finalize the pending message and add it to the transaction
    *
    * This function adds payload data to the pending message and pushes it into
    * the given transaction.
    * 
    * This will reset the destination and permissions for the pending message on
    * the given transaction.
    *
    * @param trx - the `TransactionHandle` of the pending transaction to modify
    * @param data - the payload data for this message
    * @param size - the size of `data`
    */
   void transactionPushMessage(TransactionHandle trx, void* data, int size);
   
   /**
    * @brief reset the destination and persmisions of the pending transaction
    *
    * This will reset the destination and permissions for the pending message on
    * the given transaction without committing the existing settings effectively
    * dropping any changes made so far.
    *
    * @param trx - the `TransactionHandle` of the pending transaction to modify
    */
   void transactionResetMessage(TransactionHandle trx);


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
    * @param inlineMode - whether to send as an inline transaction (!=0) or deferred(=0)
    */
   void transactionSend(TransactionHandle trx, int inlineMode = 0);

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
   

   ///@ } transactioncapi
}
