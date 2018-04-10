#include <eosio/chain/block_state.hpp>

namespace eosio { namespace chain {

   block_state::block_state( block_header_state h, signed_block_ptr b )
   :block_header_state( move(h) ), block(move(b))
   {
      if( block ) {
         for( const auto& packed : b->input_transactions ) {
            auto signed_trx = packed.get_signed_transaction();
            auto id = signed_trx.id();
            input_transactions[id] = move(signed_trx);
         }
      }
   }


} } /// eosio::chain
