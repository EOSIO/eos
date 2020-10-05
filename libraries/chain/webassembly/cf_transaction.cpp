#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/apply_context.hpp>

namespace eosio { namespace chain { namespace webassembly {
   int32_t interface::read_transaction( legacy_span<char> data ) const {
      if( data.size() == 0 ) return transaction_size();

      // always pack the transaction here as exact pack format is part of consensus
      // and an alternative packed format could be stored in get_packed_transaction()
      const packed_transaction& packed_trx = context.trx_context.packed_trx;
      bytes trx = fc::raw::pack( static_cast<const transaction&>( packed_trx.get_transaction() ) );
      size_t copy_size = std::min( static_cast<size_t>(data.size()), trx.size() );
      std::memcpy( data.data(), trx.data(), copy_size );

      return copy_size;
   }

   int32_t interface::transaction_size() const {
      const packed_transaction& packed_trx = context.trx_context.packed_trx;
      return fc::raw::pack_size( static_cast<const transaction&>( packed_trx.get_transaction() ) );
   }

   int32_t interface::expiration() const {
     return context.trx_context.packed_trx.get_transaction().expiration.sec_since_epoch();
   }

   int32_t interface::tapos_block_num() const {
     return context.trx_context.packed_trx.get_transaction().ref_block_num;
   }

   int32_t interface::tapos_block_prefix() const {
     return context.trx_context.packed_trx.get_transaction().ref_block_prefix;
   }

   int32_t interface::get_action( uint32_t type, uint32_t index, legacy_span<char> buffer ) const {
      return context.get_action( type, index, buffer.data(), buffer.size() );
   }
}}} // ns eosio::chain::webassembly
