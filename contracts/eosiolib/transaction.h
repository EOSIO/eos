/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosiolib/types.h>

extern "C" {
   /**
    * @defgroup transactionapi Transaction API
    * @ingroup contractdev
    * @brief Defines API for sending transactions and inline actions
    *
    *
    * Deferred transactions will not be processed until a future block.  They
    * can therefore have no effect on the success or failure of their parent
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
    * processing then the whole tree of transactions and actions rooted in the
    * block will be marked as failing and none of effects on the database will
    * persist.
    *
    * Inline actions and Deferred transactions must adhere to the permissions
    * available to the parent transaction or, in the future, delegated to the
    * contract account for future use.
    */

   /**
    * @defgroup transactioncapi Transaction C API
    * @ingroup transactionapi
    * @brief Defines API for sending transactions
    *
    * @{
    */

    /**
     *  Sends a deferred transaction.
     *
     *  @brief Sends a deferred transaction.
     *  @param sender_id - ID of sender
     *  @param payer - Account paying for RAM
     *  @param serialized_transaction - Pointer of serialized transaction to be deferred
     *  @param size - Size to reserve
     *  @param replace_existing - f this is `0` then if the provided sender_id is already in use by an in-flight transaction from this contract, which will be a failing assert. If `1` then transaction will atomically cancel/replace the inflight transaction
     */
     void send_deferred(const uint128_t& sender_id, account_name payer, const char *serialized_transaction, size_t size, uint32_t replace_existing = 0);

    /**
     *  Cancels a deferred transaction.
     *
     *  @brief Cancels a deferred transaction.
     *  @param sender_id - The id of the sender
     *
     *  @pre The deferred transaction ID exists.
     *  @pre The deferred transaction ID has not yet been published.
     *  @post Deferred transaction canceled.
     *
     *  @return 1 if transaction was canceled, 0 if transaction was not found
     *
     *  Example:
     *
     *  @code
     *  id = 0xffffffffffffffff
     *  cancel_deferred( id );
     *  @endcode
     */
   int cancel_deferred(const uint128_t& sender_id);

   /**
    * Access a copy of the currently executing transaction.
    *
    * @brief Access a copy of the currently executing transaction.
    * @param buffer - a buffer to write the current transaction to
    * @param size - the size of the buffer, 0 to return required size
    * @return the size of the transaction written to the buffer, or number of bytes that can be copied if size==0 passed
    */
   size_t read_transaction(char *buffer, size_t size);

   /**
    * Gets the size of the currently executing transaction.
    *
    * @brief Gets the size of the currently executing transaction.
    * @return size of the currently executing transaction
    */
   size_t transaction_size();

   /**
    * Gets the block number used for TAPOS on the currently executing transaction.
    *
    * @brief Gets the block number used for TAPOS on the currently executing transaction.
    * @return block number used for TAPOS on the currently executing transaction
    * Example:
    * @code
    * int tbn = tapos_block_num();
    * @endcode
    */
   int tapos_block_num();

   /**
    * Gets the block prefix used for TAPOS on the currently executing transaction.
    *
    * @brief Gets the block prefix used for TAPOS on the currently executing transaction.
    * @return block prefix used for TAPOS on the currently executing transaction
    * Example:
    * @code
    * int tbp = tapos_block_prefix();
    * @endcode
    */
   int tapos_block_prefix();

   /**
    * Gets the expiration of the currently executing transaction.
    *
    * @brief Gets the expiration of the currently executing transaction.
    * @return expiration of the currently executing transaction
    * Example:
    * @code
    * time tm = expiration();
    * eosio_print(tm);
    * @endcode
    */
   time expiration();

   /**
    * Retrieves the indicated action from the active transaction.
    *
    * @brief Retrieves the indicated action from the active transaction.
    * @param type - 0 for context free action, 1 for action
    * @param index - the index of the requested action
    * @param buff - output packed buff of the action
    * @param size - amount of buff read, pass 0 to have size returned
    * @return the size of the action, -1 on failure
    */
   int get_action( uint32_t type, uint32_t index, char* buff, size_t size );

   /**
    * Retrieve the signed_transaction.context_free_data[index].
    *
    * @brief Retrieve the signed_transaction.context_free_data[index].
    * @param index - the index of the context_free_data entry to retrieve
    * @param buff - output buff of the context_free_data entry
    * @param size - amount of context_free_data[index] to retrieve into buff, 0 to report required size
    * @return size copied, or context_free_data[index].size() if 0 passed for size, or -1 if index not valid
    */
   int get_context_free_data( uint32_t index, char* buff, size_t size );

   ///@ } transactioncapi
}
