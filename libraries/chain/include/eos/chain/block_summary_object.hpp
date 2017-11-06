/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/types.hpp>

#include "multi_index_includes.hpp"

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
   class block_summary_object : public chainbase::object<block_summary_object_type, block_summary_object>
   {
         OBJECT_CTOR(block_summary_object)

         id_type        id;
         block_id_type  block_id;
   };

   struct by_block_id;
   using block_summary_multi_index = chainbase::shared_multi_index_container<
      block_summary_object,
      indexed_by<
         ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(block_summary_object, block_summary_object::id_type, id)>
   //      ordered_unique<tag<by_block_id>, BOOST_MULTI_INDEX_MEMBER(block_summary_object, block_id_type, block_id)>
      >
   >;

} }

CHAINBASE_SET_INDEX_TYPE(eosio::chain::block_summary_object, eosio::chain::block_summary_multi_index)

FC_REFLECT( eosio::chain::block_summary_object, (block_id) )
