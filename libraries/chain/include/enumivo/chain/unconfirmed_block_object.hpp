
/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <enumivo/chain/types.hpp>
#include <enumivo/chain/authority.hpp>
#include <enumivo/chain/block_timestamp.hpp>
#include <enumivo/chain/contract_types.hpp>

#include "multi_index_includes.hpp"

namespace enumivo { namespace chain {

   class unconfirmed_block_object : public chainbase::object<unconfirmed_block_object_type, unconfirmed_block_object> {
      OBJECT_CTOR(unconfirmed_block_object,(packedblock) )

      id_type        id;
      uint32_t       blocknum = 0;
      shared_string  packedblock;

      void set_block( const signed_block_ptr& b ) {
         packedblock.resize( fc::raw::pack_size( *b ) );
         fc::datastream<char*> ds( packedblock.data(), packedblock.size() );
         fc::raw::pack( ds, *b );
      }

      signed_block_ptr get_block()const {
         fc::datastream<const char*> ds( packedblock.data(), packedblock.size() );
         auto result = std::make_shared<signed_block>();
         fc::raw::unpack( ds, *result );
         return result;
      }
   };

   struct by_num;
   using unconfirmed_block_index = chainbase::shared_multi_index_container<
      unconfirmed_block_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<unconfirmed_block_object, unconfirmed_block_object::id_type, &unconfirmed_block_object::id>>,
         ordered_unique<tag<by_num>, member<unconfirmed_block_object, uint32_t, &unconfirmed_block_object::blocknum>>
      >
   >;

} } // enumivo::chain

CHAINBASE_SET_INDEX_TYPE(enumivo::chain::unconfirmed_block_object, enumivo::chain::unconfirmed_block_index)

