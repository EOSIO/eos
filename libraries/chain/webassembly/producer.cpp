#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   int32_t interface::get_active_producers( legacy_array_ptr<account_name> producers, uint32_t buffer_size ) {
      auto active_producers = ctx.get_active_producers();

      size_t len = active_producers.size();
      auto s = len * sizeof(chain::account_name);
      if( buffer_size == 0 ) return s;

      auto copy_size = std::min( static_cast<size_t>(buffer_size), s );
      memcpy( producers, active_producers.data(), copy_size );

      return copy_size;
   }
}}} // ns eosio::chain::webassembly
