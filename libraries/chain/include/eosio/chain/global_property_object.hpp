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
#include <eosio/chain/whitelisted_intrinsics.hpp>
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
      id_type                        id;
      optional<block_num_type>       proposed_schedule_block_num;
      shared_producer_schedule_type  proposed_schedule;
      chain_config                   configuration;
   };


   using global_property_multi_index = chainbase::shared_multi_index_container<
      global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(global_property_object, global_property_object::id_type, id)
         >
      >
   >;

   /**
    * @class protocol_state_object
    * @brief Maintains global state information about consensus protocol rules
    * @ingroup object
    * @ingroup implementation
    */
   class protocol_state_object : public chainbase::object<protocol_state_object_type, protocol_state_object>
   {
      OBJECT_CTOR(protocol_state_object, (activated_protocol_features)(preactivated_protocol_features)(whitelisted_intrinsics))

   public:
      struct activated_protocol_feature {
         digest_type feature_digest;
         uint32_t    activation_block_num = 0;

         activated_protocol_feature() = default;

         activated_protocol_feature( const digest_type& feature_digest, uint32_t activation_block_num )
         :feature_digest( feature_digest )
         ,activation_block_num( activation_block_num )
         {}
      };

   public:
      id_type                                    id;
      shared_vector<activated_protocol_feature>  activated_protocol_features;
      shared_vector<digest_type>                 preactivated_protocol_features;
      whitelisted_intrinsics_type                whitelisted_intrinsics;
   };

   using protocol_state_multi_index = chainbase::shared_multi_index_container<
      protocol_state_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(protocol_state_object, protocol_state_object::id_type, id)
         >
      >
   >;

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
CHAINBASE_SET_INDEX_TYPE(eosio::chain::protocol_state_object, eosio::chain::protocol_state_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::dynamic_global_property_object,
                         eosio::chain::dynamic_global_property_multi_index)

FC_REFLECT(eosio::chain::global_property_object,
            (proposed_schedule_block_num)(proposed_schedule)(configuration)
          )

FC_REFLECT(eosio::chain::protocol_state_object::activated_protocol_feature,
            (feature_digest)(activation_block_num)
          )

FC_REFLECT(eosio::chain::protocol_state_object,
            (activated_protocol_features)(preactivated_protocol_features)(whitelisted_intrinsics)
          )

FC_REFLECT(eosio::chain::dynamic_global_property_object,
            (global_action_sequence)
          )
