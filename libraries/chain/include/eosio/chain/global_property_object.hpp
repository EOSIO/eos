/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
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

#include <eosio/chain/rate_limiting.hpp>

namespace eosio { namespace chain {

   struct blocknum_producer_schedule {
      blocknum_producer_schedule( allocator<char> a )
      :second(a){}

      block_num_type                first;
      shared_producer_schedule_type second;
   };

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
      OBJECT_CTOR(global_property_object, (active_producers)(new_active_producers)(pending_active_producers) )

      id_type                                                  id;
      chain_config                                             configuration;
      shared_producer_schedule_type                            active_producers;
      shared_producer_schedule_type                            new_active_producers;

      /** every block that has change in producer schedule gets inserted into this list, this includes
       * all blocks that see a change in producer signing keys or vote order.
       *
       * TODO: consider moving this to a more effeicent datatype
       */
      shared_vector< blocknum_producer_schedule > pending_active_producers;
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
        OBJECT_CTOR(dynamic_global_property_object, (block_merkle_root))

        id_type              id;
        uint32_t             head_block_number = 0;
        block_id_type        head_block_id;
        time_point           time;
        account_name         current_producer;

        uint64_t total_net_weight  = 1;
        uint64_t total_cpu_weight  = 1;
        uint64_t total_db_capacity = 1024*1024*1024ull*1024ull;
        uint64_t total_db_reserved = 0;

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
        //uint64_t recent_slots_filled;

        uint32_t last_irreversible_block_num = 0;

        /**
         * Track the average blocksize over the past 60 seconds and use it to adjust the
         * reserve ratio for bandwidth rate limiting calclations.
         */
        average_accumulator<config::blocksize_average_window_ms> average_block_size;

        /**
         * Track the average actions per block over the past 60 seconds and use it to
         * adjust hte reserve ration for action rate limiting calculations
         */
        average_accumulator<config::blocksize_average_window_ms> average_block_acts;

        void update_virtual_net_bandwidth( const chain_config& cfg );
        void update_virtual_act_bandwidth( const chain_config& cfg );

        /**
         * The virtual number of bytes that would be consumed over blocksize_average_window_ms
         * if all blocks were at their maximum virtual size. This is virtual because the
         * real maximum block is less, this virtual number is only used for rate limiting users.
         *
         * It's lowest possible value is max_block_size * blocksize_average_window_ms / block_interval
         * It's highest possible value is 1000 times its lowest possible value
         *
         * This means that the most an account can consume during idle periods is 1000x the bandwidth
         * it is gauranteed under congestion.
         *
         * Increases when average_block_size < target_block_size, decreases when
         * average_block_size > target_block_size, with a cap at 1000x max_block_size
         * and a floor at max_block_size;
         **/
        uint64_t virtual_net_bandwidth = 0;

        /**
         *  Increases when average_bloc
         */
        uint64_t virtual_act_bandwidth = 0;

        /**
         * Used to calculate the merkle root over all blocks
         */
        shared_incremental_merkle  block_merkle_root;
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
           (current_absolute_slot)
           /* (recent_slots_filled) */
           (last_irreversible_block_num)
          )

FC_REFLECT(eosio::chain::global_property_object,
           (configuration)
           (active_producers)
          )
