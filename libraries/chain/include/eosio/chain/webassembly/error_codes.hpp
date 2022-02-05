#pragma once

namespace eosio { namespace chain { namespace webassembly { namespace error_codes {

   enum recover_key_safe : int32_t {
      undefined = -1, ///< undefined error
      none = 0, ///< succeed
      invalid_message_digest, ///< failed to deserialize a message digest
      invalid_signature_format, ///< failed to deserialize a signature
      unactivated_key_type, ///< failed to recover a key using unactivated key algorithm
      invalid_signature_data, ///< failed to recover a key from the given signature data
      insufficient_output_buffer, ///< failed to store a recovered key due to insufficient output buffer
   };

}}}} // ns eosio::chain::webassembly::error_codes
