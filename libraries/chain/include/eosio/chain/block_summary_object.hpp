/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/multi_index_includes.hpp>

namespace eosio { namespace chain {
   /**
    *  @brief tracks minimal information about past blocks to implement TaPOS
    *  @ingroup object
    *
    *  When attempting to calculate the validity of a transaction we need to
    *  lookup a past block and check its block hash and the time it occurred
    *  so we can calculate whether the current transaction is valid and at
    *  what time it should expire.
    */
   class block_summary_object : public cyberway::chaindb::object<block_summary_object_type, block_summary_object>
   {
         CHAINDB_OBJECT_ID_CTOR(block_summary_object)

         id_type        id;
         block_id_type  block_id;
   };

   struct by_block_id;
   using block_summary_table = cyberway::chaindb::table_container<
      block_summary_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(block_summary_object, block_summary_object::id_type, id)>
   //      ordered_unique<tag<by_block_id>, BOOST_MULTI_INDEX_MEMBER(block_summary_object, block_id_type, block_id)>
      >
   >;

} }

CHAINDB_SET_TABLE_TYPE(eosio::chain::block_summary_object, eosio::chain::block_summary_table)
CHAINDB_TAG(eosio::chain::block_summary_object, blocksum)

FC_REFLECT( eosio::chain::block_summary_object, (id)(block_id) )
