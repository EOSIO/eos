#pragma once
#include <eoslib/types.h>

extern "C" {
   /**
    * @defgroup messageapi Message API
    * @ingroup contractdev
    * @brief Define API for querying message properties
    *
    * A EOS.IO message has the following abstract structure:
    *
    * ```
    *   struct Message {
    *     Name code; ///< primary account whose code defines the action
    *     Name action; ///< the name of the action.
    *     Name recipients[]; ///< accounts whose code will be notified (in addition to code)
    *     Name authorization[]; ///< accounts that have approved this message
    *     char data[];
    *   };
    * ```
    *
    * This API enables your contract to inspect the fields on the current message and act accordingly.
    *
    */

   /**
    * @defgroup messagecapi Message C API
    * @ingroup messageapi
    * @brief Define API for querying message properties
    *
    * @{
    */

   /**
    *  Copy up to @ref len bytes of current message to the specified location
    *  @brief Copy current message to the specified location
    *  @param msg - a pointer where up to @ref len bytes of the current message will be copied
    *  @param len - len of the current message to be copied
    *  @return the number of bytes copied to msg
    */
   uint32_t readMessage( void* msg, uint32_t len );

   /**
    * Get the length of the current message's data field
    * This method is useful for dynamicly sized messages
    * @brief Get the lenght of current message's data field
    * @return the length of the current message's data field
    */
   uint32_t messageSize();

   /**
    *  Verifies that @ref name exists in the set of notified accounts on a message. Throws if not found
    *  @brief Verify specified account exists in the set of notified accounts
    *  @param name - name of the account to be verified
    */
   void        requireNotice( AccountName name );

   /**
    *  Verifies that @ref name exists in the set of provided auths on a message. Throws if not found
    *  @brief Verify specified account exists in the set of provided auths
    *  @param name - name of the account to be verified
    */
   void        requireAuth( AccountName name );

   /**
    *  Get the account which specifies the code that is being run
    *  @brief Get the account which specifies the code that is being run
    *  @return the account which specifies the code that is being run
    */
   AccountName currentCode();


   /**
    *  Aborts processing of this message and unwinds all pending changes if the test condition is true
    *  @brief Aborts processing of this message and unwinds all pending changes
    *  @param test - 0 to abort, 1 to ignore
    *  @param cstr - a null terminated message to explain the reason for failure
    */
   void  assert( uint32_t test, const char* cstr );

   /**
    *  Returns the time in seconds from 1970 of the last accepted block (not the block including this message)
    *  @brief Get time of the last accepted block
    *  @return time in seconds from 1970 of the last accepted block
    */
   Time  now();

   ///@ } messagecapi
}
