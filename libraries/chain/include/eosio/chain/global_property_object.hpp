/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <fc/uint128.hpp>
#include <fc/array.hpp>

#include <eosio/chain/types.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/incremental_merkle.hpp>
#include <eosio/chain/snapshot.hpp>
#include <chainbase/chainbase.hpp>
#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   /**
    * @class global_property_object
    * @brief Maintains global state information about block producer schedules and chain configuration parameters
    * @ingroup object
    * @ingroup implementation
    */
   class global_property_object : public chainbase::object<global_property_object_type, global_property_object>
   {
      OBJECT_CTOR(global_property_object, (proposed_schedule))

   public:
      id_type                             id;
      optional<block_num_type>            proposed_schedule_block_num;
      shared_producer_authority_schedule  proposed_schedule;
      chain_config                        configuration;
   };


   using global_property_multi_index = chainbase::shared_multi_index_container<
      global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(global_property_object, global_property_object::id_type, id)
         >
      >
   >;

   struct snapshot_global_property_object {
      optional<block_num_type>            proposed_schedule_block_num;
      producer_authority_schedule         proposed_schedule;
      chain_config                        configuration;
   };

   namespace detail {
      template<>
      struct snapshot_row_traits<global_property_object> {
         using value_type = global_property_object;
         using snapshot_type = snapshot_global_property_object;

         static snapshot_global_property_object to_snapshot_row( const global_property_object& value, const chainbase::database& ) {
            return {value.proposed_schedule_block_num, producer_authority_schedule::from_shared(value.proposed_schedule), value.configuration};
         }

         static void from_snapshot_row( snapshot_global_property_object&& row, global_property_object& value, chainbase::database& ) {
            value.proposed_schedule_block_num = row.proposed_schedule_block_num;
            value.proposed_schedule = row.proposed_schedule.to_shared(value.proposed_schedule.producers.get_allocator());
            value.configuration = row.configuration;
         }
      };
   }

   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information that frequently change
    * @ingroup object
    * @ingroup implementation
    */
   class dynamic_global_property_object : public chainbase::object<dynamic_global_property_object_type, dynamic_global_property_object>
   {
        OBJECT_CTOR(dynamic_global_property_object)

        id_type    id;
        uint64_t   global_action_sequence = 0;
   };

   using dynamic_global_property_multi_index = chainbase::shared_multi_index_container<
      dynamic_global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(dynamic_global_property_object, dynamic_global_property_object::id_type, id)
         >
      >
   >;

}}

CHAINBASE_SET_INDEX_TYPE(eosio::chain::global_property_object, eosio::chain::global_property_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::dynamic_global_property_object,
                         eosio::chain::dynamic_global_property_multi_index)

FC_REFLECT(eosio::chain::global_property_object,
            (proposed_schedule_block_num)(proposed_schedule)(configuration)
          )

FC_REFLECT(eosio::chain::snapshot_global_property_object,
            (proposed_schedule_block_num)(proposed_schedule)(configuration)
          )

FC_REFLECT(eosio::chain::dynamic_global_property_object,
            (global_action_sequence)
          )
