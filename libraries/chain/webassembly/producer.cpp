#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   int32_t interface::get_active_producers( legacy_span<account_name> producers ) const {
      auto active_producers = context.get_active_producers();

      size_t len = active_producers.size();
      auto s = len * sizeof(chain::account_name);
      if( producers.size_bytes() == 0 ) return s;

      auto copy_size = std::min( producers.size(), s );
      std::memcpy( producers.data(), active_producers.data(), copy_size );

      return copy_size;
   }
}}} // ns eosio::chain::webassembly
