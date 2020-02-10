#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   int32_t interface::get_context_free_data( uint32_t index, legacy_array_ptr<char> buffer, uint32_t buffer_size) const {
      return context.get_context_free_data( index, buffer, buffer_size );
   }
}}} // ns eosio::chain::webassembly
