#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/transaction_context.hpp>

namespace eosio { namespace chain { namespace webassembly {
   int32_t interface::read_transaction( legacy_span<char> data ) const {
      bytes trx = context.get_packed_transaction();

      auto s = trx.size();
      if (data.size() == 0) return s;

      auto copy_size = std::min( static_cast<size_t>(data.size()), s );
      std::memcpy( data.data(), trx.data(), copy_size );

      return copy_size;
   }

   int32_t interface::transaction_size() const {
      return context.get_packed_transaction().size();
   }

   int32_t interface::expiration() const {
     return context.trx_context.trx.expiration.sec_since_epoch();
   }

   int32_t interface::tapos_block_num() const {
     return context.trx_context.trx.ref_block_num;
   }
   int32_t interface::tapos_block_prefix() const {
     return context.trx_context.trx.ref_block_prefix;
   }

   int32_t interface::get_action( uint32_t type, uint32_t index, legacy_span<char> buffer ) const {
      return context.get_action( type, index, buffer.data(), buffer.size() );
   }
}}} // ns eosio::chain::webassembly
