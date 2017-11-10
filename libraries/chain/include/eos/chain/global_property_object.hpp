/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/uint128.hpp>
#include <fc/array.hpp>

#include <eos/chain/types.hpp>
#include <eos/chain/blockchain_configuration.hpp>

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
      OBJECT_CTOR(global_property_object)

      id_type id;
      blockchain_configuration configuration;
      std::array<account_name, config::blocks_per_round> active_producers;
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

        id_type           id;
        uint32_t          head_block_number = 0;
        block_id_type     head_block_id;
        time_point_sec    time;
        account_name      current_producer;
        uint32_t          accounts_registered_this_interval = 0;
        
        /**
         * The current absolute slot number.  Equal to the total
         * number of slots since genesis.  Also equal to the total
         * number of missed slots plus head_block_number.
         */
        uint64_t          current_absolute_slot = 0;
        
        /**
         * Bitmap used to compute producer participation. Stores
         * a high bit for each generated block, a low bit for
         * each missed block. Least significant bit is most
         * recent block.
         *
         * NOTE: This bitmap always excludes the head block,
         * which, by definition, exists. The least significant
         * bit corresponds to the block with number
         * head_block_num()-1
         *
         * e.g. if the least significant 5 bits were 10011, it
         * would indicate that the last two blocks prior to the
         * head block were produced, the two before them were
         * missed, and the one before that was produced.
         */
        uint64_t recent_slots_filled;
        
        uint32_t last_irreversible_block_num = 0;
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

}}

CHAINBASE_SET_INDEX_TYPE(eosio::chain::global_property_object, eosio::chain::global_property_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::dynamic_global_property_object,
                         eosio::chain::dynamic_global_property_multi_index)

FC_REFLECT(eosio::chain::dynamic_global_property_object,
           (head_block_number)
           (head_block_id)
           (time)
           (current_producer)
           (accounts_registered_this_interval)
           (current_absolute_slot)
           (recent_slots_filled)
           (last_irreversible_block_num)
          )

FC_REFLECT(eosio::chain::global_property_object,
           (configuration)
           (active_producers)
          )
