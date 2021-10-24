#pragma once

namespace eosio { namespace chain { namespace webassembly { namespace error_codes {

   enum recover_key_safe : int32_t {
      undefined = -1,
      none = 0,
      invalid_signature_format,
      unactivated_key_type,
      invalid_signature_data,
      insufficient_output_buffer,
   };

}}}} // ns eosio::chain::webassembly::error_codes
