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
#include <eosio/chain/multi_index_includes.hpp>

namespace eosio { namespace chain {

   /**
    * @class global_property_object
    * @brief Maintains global state information (committee_member list, current fees)
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are set by committee_members to tune the blockchain parameters.
    */
   class global_property_object : public cyberway::chaindb::object<global_property_object_type, global_property_object>
   {
      CHAINDB_OBJECT_ID_CTOR(global_property_object)

      id_type                           id;
      optional<block_num_type>          proposed_schedule_block_num;
      shared_producer_schedule_type     proposed_schedule;
      chain_config                      configuration;
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
   class dynamic_global_property_object : public cyberway::chaindb::object<dynamic_global_property_object_type, dynamic_global_property_object>
   {
        CHAINDB_OBJECT_ID_CTOR(dynamic_global_property_object)

        id_type    id;
        uint64_t   global_action_sequence = 0;
   };

   using global_property_table = cyberway::chaindb::table_container<
      global_property_object,
      cyberway::chaindb::indexed_by<
          cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(global_property_object, global_property_object::id_type, id)
         >
      >
   >;

   using dynamic_global_property_table = cyberway::chaindb::table_container<
      dynamic_global_property_object,
      cyberway::chaindb::indexed_by<
          cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(dynamic_global_property_object, dynamic_global_property_object::id_type, id)
         >
      >
   >;

}}

CHAINDB_SET_TABLE_TYPE(eosio::chain::global_property_object, eosio::chain::global_property_table)
CHAINDB_TAG(eosio::chain::global_property_object, gproperty)
CHAINDB_SET_TABLE_TYPE(eosio::chain::dynamic_global_property_object, eosio::chain::dynamic_global_property_table)
CHAINDB_TAG(eosio::chain::dynamic_global_property_object, gdynproperty)

FC_REFLECT(eosio::chain::dynamic_global_property_object,
           (id)(global_action_sequence)
          )

FC_REFLECT(eosio::chain::global_property_object,
           (id)(proposed_schedule_block_num)(proposed_schedule)(configuration)
          )
