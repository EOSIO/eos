/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/system.h>

extern "C" {
   /**
    * @defgroup messageapi message API
    * @ingroup contractdev
    * @brief Define API for querying message properties
    *
    */

   /**
    * @defgroup messagecapi message C API
    * @ingroup messageapi
    * @brief Define API for querying message properties
    *
    *
    * A EOS.IO message has the following abstract structure:
    *
    * ```
    *   struct message {
    *     account_name code; // the contract defining the primary code to execute for code/type
    *     func_name type; // the action to be taken
    *     AccountPermission[] authorization; // the accounts and permission levels provided
    *     bytes data; // opaque data processed by code
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
    * uint32_t total = read_action(buffer, 5); // buffer contains the content of the message up to 5 bytes
    * print(total); // Output: 5
    *
    * uint32_t msgsize = action_size();
    * print(msgsize); // Output: size of the above message's data field
    *
    * require_notice(N(initc)); // initc account will be notified for this message
    *
    * require_auth(N(inita)); // Do nothing since inita exists in the auth list
    * require_auth(N(initb)); // Throws an exception
    *
    * account_name code = current_receiver();
    * print(Name(code)); // Output: eos
    *
    * assert(Name(current_receiver()) === "eos", "This message expects to be received by eos"); // Do nothing
    * assert(Name(current_receiver()) === "inita", "This message expects to be received by inita"); // Throws exception and roll back transfer transaction
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
   uint32_t read_action( void* msg, uint32_t len );

   /**
    * Get the length of the current message's data field
    * This method is useful for dynamically sized messages
    * @brief Get the length of current message's data field
    * @return the length of the current message's data field
    */
   uint32_t action_size();

   /**
    *  Add the specified account to set of accounts to be notified
    *  @brief Add the specified account to set of accounts to be notified
    *  @param name - name of the account to be verified
    */
   void require_notice( account_name name );

   /**
    *  Verifies that @ref name exists in the set of provided auths on a message. Throws if not found
    *  @brief Verify specified account exists in the set of provided auths
    *  @param name - name of the account to be verified
    */
   void require_auth( account_name name );

   /**
    *  Get the account which specifies the code that is being run
    *  @brief Get the account which specifies the code that is being run
    *  @return the account which specifies the code that is being run
    */
   account_name current_receiver();

   ///@ } messagecapi
}
