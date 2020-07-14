#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/contract_types.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   class reversible_block_object : public chainbase::object<reversible_block_object_type, reversible_block_object> {
      OBJECT_CTOR(reversible_block_object,(packedblock) )

      id_type        id;
      uint32_t       blocknum = 0; //< blocknum should not be changed within a chainbase modifier lambda
      shared_string  packedblock;

      void set_block( const signed_block_ptr& b ) {
         packedblock.resize_and_fill( fc::raw::pack_size( *b ), [&b](char* data, std::size_t size) {
            fc::datastream<char*> ds( data, size );
            fc::raw::pack( ds, *b );
         });
      }

      signed_block_ptr get_block()const {
         fc::datastream<const char*> ds( packedblock.data(), packedblock.size() );
         auto result = std::make_shared<signed_block>();
         fc::raw::unpack( ds, *result );
         return result;
      }

      block_id_type get_block_id()const {
         fc::datastream<const char*> ds( packedblock.data(), packedblock.size() );
         block_header h;
         fc::raw::unpack( ds, h );
         // Only need the block id to then look up the block state in fork database, so just unpack the block_header from the stored packed data.
         // Avoid calling get_block() since that constructs a new signed_block in heap memory and unpacks the full signed_block from the stored packed data.
         return h.calculate_id();
      }
   };

   struct by_num;
   using reversible_block_index = chainbase::shared_multi_index_container<
      reversible_block_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<reversible_block_object, reversible_block_object::id_type, &reversible_block_object::id>>,
         ordered_unique<tag<by_num>, member<reversible_block_object, uint32_t, &reversible_block_object::blocknum>>
      >
   >;

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::reversible_block_object, eosio::chain::reversible_block_index)
