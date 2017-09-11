#pragma once
#include <eoslib/system.h>

extern "C" {
   /**
    * @defgroup messageapi Message API
    * @ingroup contractdev
    * @brief Define API for querying message properties
    *
    */

   /**
    * @defgroup messagecapi Message C API
    * @ingroup messageapi
    * @brief Define API for querying message properties
    *
    *
    * A EOS.IO message has the following abstract structure:
    *
    * ```
    *   struct Message {
    *     AccountName code; // the contract defining the primary code to execute for code/type
    *     FuncName type; // the action to be taken
    *     AccountPermission[] authorization; // the accounts and permission levels provided
    *     Bytes data; // opaque data processed by code
    *   };
    * ```
    *
    * This API enables your contract to inspect the fields on the current message and act accordingly.
    *
    * Example:
    * @code
    * // Assume this message is used for the following examples:
    * // {
    * //  "code": "eos",
    * //  "type": "transfer",
    * //  "authorization": [{ "account": "inita", "permission": "active" }],
    * //  "data": {
    * //    "from": "inita",
    * //    "to": "initb",
    * //    "amount": 1000
    * //  }
    * // }
    *
    * char buffer[128];
    * uint32_t total = readMessage(buffer, 5); // buffer contains the content of the message up to 5 bytes
    * print(total); // Output: 5
    *
    * uint32_t msgsize = messageSize();
    * print(msgsize); // Output: size of the above message's data field
    *
    * requireNotice(N(initc)); // initc account will be notified for this message
    *
    * requireAuth(N(inita)); // Do nothing since inita exists in the auth list
    * requireAuth(N(initb)); // Throws an exception
    *
    * AccountName code = currentCode();
    * print(Name(code)); // Output: eos
    *
    * assert(Name(currentCode()) === "eos", "This message expects to be received by eos"); // Do nothing
    * assert(Name(currentCode()) === "inita", "This message expects to be received by inita"); // Throws exception and roll back transfer transaction
    *
    * print(now()); // Output: timestamp of last accepted block
    *
    * @endcode
    *
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
    * This method is useful for dynamically sized messages
    * @brief Get the length of current message's data field
    * @return the length of the current message's data field
    */
   uint32_t messageSize();

   /**
    *  Add the specified account to set of accounts to be notified
    *  @brief Add the specified account to set of accounts to be notified
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

   ///@ } messagecapi
}
