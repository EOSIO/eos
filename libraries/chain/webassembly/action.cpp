#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   int32_t interface::read_action_data(legacy_array_ptr<char> memory, uint32_t buffer_size) const {
      auto s = context.get_action().data.size();
      if( buffer_size == 0 ) return s;

      auto copy_size = std::min( static_cast<size_t>(buffer_size), s );
      std::memcpy( (char*)memory.value, context.get_action().data.data(), copy_size );

      return copy_size;
   }

   int32_t interface::action_data_size() const {
      return context.get_action().data.size();
   }

   name interface::current_receiver() const {
      return context.get_receiver();
   }

   void interface::set_action_return_value( legacy_array_ptr<char> packed_blob, uint32_t datalen ) {
      context.action_return_value.assign( packed_blob.value, packed_blob.value + datalen );
   }
}}} // ns eosio::chain::webassembly
