#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   int32_t interface::read_action_data(legacy_array_ptr<char> memory, uint32_t buffer_size) {
      auto s = ctx.get_action().data.size();
      if( buffer_size == 0 ) return s;

      auto copy_size = std::min( static_cast<size_t>(buffer_size), s );
      memcpy( (char*)memory.value, ctx.get_action().data.data(), copy_size );

      return copy_size;
   }

   int32_t interface::action_data_size() {
      return ctx.get_action().data.size();
   }

   name interface::current_receiver() {
      return ctx.get_receiver();
   }

   void interface::set_action_return_value( legacy_array_ptr<char> packed_blob, uint32_t datalen ) {
      ctx.action_return_value.assign( packed_blob.value, packed_blob.value + datalen );
   }
}}} // ns eosio::chain::webassembly
