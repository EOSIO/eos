/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/types.h>

extern "C" {
   /**
    * @defgroup transactionapi transaction API
    * @ingroup contractdev
    * @brief Define API for sending transactions and inline messages
    *
    * A EOS.IO transaction has the following abstract structure:
    *
    * ```
    *   struct transaction {
    *     Name scope[]; ///< accounts whose data may be read or written
    *     Name readScope[]; ///< accounts whose data may be read
    *     message messages[]; ///< accounts that have approved this message
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
   void send_deferred(const uint128_t& sender_id, account_name payer, const char *serialized_transaction, size_t size, uint32_t replace_existing = 0);

   void cancel_deferred(const uint128_t& sender_id);

   /**
    * access a copy of the currently executing transaction
    *
    * @param buffer - a buffer to write the current transaction to
    * @param size - the size of the buffer, 0 to return required size
    * @return the size of the transaction written to the buffer, or number of bytes that can be copied if size==0 passed
    */
   size_t read_transaction(char *buffer, size_t size);

   /**
    * get the size of the currently executing transaction
    */
   size_t transaction_size();

   /**
    * get the block number used for TAPOS on the currently executing transaction
    *
    */
   int tapos_block_num();

   /**
    * get the block prefix used for TAPOS on the currently executing transaction
    */
   int tapos_block_prefix();

   /**
    * get the expiration of the currently executing transaction
    */
   time expiration();

   /**
    * Retrieve the indicated action from the active transaction.
    * @param type - 0 for context free action, 1 for action
    * @param index - the index of the requested action
    * @param buff - output packed buff of the action
    * @param size - amount of buff read, pass 0 to have size returned
    * @return the size of the action, -1 on failure
    */
   int get_action( uint32_t type, uint32_t index, char* buff, size_t size );

   /**
    * Retrieve the signed_transaction.context_free_data[index].
    * @param index - the index of the context_free_data entry to retrieve
    * @param buff - output buff of the context_free_data entry
    * @param size - amount of context_free_data[index] to retrieve into buff, 0 to report required size
    * @return size copied, or context_free_data[index].size() if 0 passed for size, or -1 if index not valid
    */
   int get_context_free_data( uint32_t index, char* buff, size_t size );

   ///@ } transactioncapi
}
