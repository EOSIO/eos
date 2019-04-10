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
#include <chainbase/chainbase.hpp>
#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   /**
    * @class global_property_object
    * @brief Maintains global state information (committee_member list, current fees)
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are set by committee_members to tune the blockchain parameters.
    */
   class global_property_object : public chainbase::object<global_property_object_type, global_property_object>
   {
      OBJECT_CTOR(global_property_object, (proposed_schedule))

      id_type                           id;
      optional<block_num_type>          proposed_schedule_block_num;
      shared_producer_schedule_type     proposed_schedule;
      chain_config                      configuration;
   };

   // *bos*
   class global_property2_object : public chainbase::object<global_property2_object_type, global_property2_object>
   {
      OBJECT_CTOR(global_property2_object, (cfg))

      id_type                       id;
      chain_config2                 cfg;
      guaranteed_minimum_resources    gmr;//guaranteed_minimum_resources
   };

   class upgrade_property_object : public chainbase::object<upgrade_property_object_type, upgrade_property_object>
   {
      OBJECT_CTOR(upgrade_property_object)
      //TODO: should use a more complicated struct to include id, digest and status of every single upgrade.

      id_type                       id;
      block_num_type                upgrade_target_block_num = 0;
   };


   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information (committee_member list, current fees)
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
    * current values of global blockchain properties.
    */
   class dynamic_global_property_object : public chainbase::object<dynamic_global_property_object_type, dynamic_global_property_object>
   {
        OBJECT_CTOR(dynamic_global_property_object)

        id_type    id;
        uint64_t   global_action_sequence = 0;
   };

   using global_property_multi_index = chainbase::shared_multi_index_container<
      global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(global_property_object, global_property_object::id_type, id)
         >
      >
   >;

   using dynamic_global_property_multi_index = chainbase::shared_multi_index_container<
      dynamic_global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(dynamic_global_property_object, dynamic_global_property_object::id_type, id)
         >
      >
   >;

   // *bos*
   using global_property2_multi_index = chainbase::shared_multi_index_container<
      global_property2_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(global_property2_object, global_property2_object::id_type, id)
         >
      >
   >;

   using upgrade_property_multi_index = chainbase::shared_multi_index_container<
      upgrade_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(upgrade_property_object, upgrade_property_object::id_type, id)
         >
      >
   >;
}}

CHAINBASE_SET_INDEX_TYPE(eosio::chain::global_property_object, eosio::chain::global_property_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::dynamic_global_property_object,
                         eosio::chain::dynamic_global_property_multi_index)
// *bos*
CHAINBASE_SET_INDEX_TYPE(eosio::chain::global_property2_object, eosio::chain::global_property2_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::upgrade_property_object, eosio::chain::upgrade_property_multi_index)

FC_REFLECT(eosio::chain::dynamic_global_property_object,
           (global_action_sequence)
          )

FC_REFLECT(eosio::chain::global_property_object,
           (proposed_schedule_block_num)(proposed_schedule)(configuration)
          )
// *bos*
FC_REFLECT(eosio::chain::global_property2_object,
           (cfg)(gmr)
          )
FC_REFLECT(eosio::chain::upgrade_property_object,
           (upgrade_target_block_num)
          )