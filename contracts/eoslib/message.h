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
    *  @param msg - a pointer where up to @ref len bytes of the current message will be coppied
    *  @return the number of bytes copied to msg
    */
   uint32_t readMessage( void* msg, uint32_t len );

   /**
    * This method is useful for dynamicly sized messages
    *
    * @return the length of the current message's data field
    */
   uint32_t messageSize();

   /**
    *  Verifies that @ref name exists in the set of notified accounts on a message. Throws if not found
    */
   void        requireNotice( AccountName );

   /**
    *  Verifies that @ref name exists in the set of provided auths on a message. Throws if not found
    */
   void        requireAuth( AccountName name );

   /**
    *  @return the account which specifes the code that is being run
    */
   AccountName currentCode();


   /**
    *  Aborts processing of this message and unwinds all pending changes
    *
    *  @param test - 0 to abort, 1 to ignore
    *  @param cstr - a null terminated message to explain the reason for failure
    */
   void  assert( uint32_t test, const char* cstr );

   /**
    *  Returns the time in seconds from 1970 of the last accepted block (not the block including this message)
    */
   Time  now();

   ///@ } messagecapi
}
