#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>

#include "multi_index_includes.hpp"


namespace eosio { namespace chain {

   /**
    *  This class accumulates an average value taking periodic inputs.
    *
    *  The average value returns to 0 after WindowMS without any updates and
    *  decays linerally if updates occur in the middle of a window before adding
    *  the new value.
    *
    *  The value stored is Precision times the sum of the inputs.
    */
   template<uint32_t WindowMs, uint64_t Precision = config::rate_limiting_precision>
   struct average_accumulator
   {
      time_point last_update;
      uint64_t   value = 0;

      /**
       * return the average value in rate_limiting_precision
       */
      uint64_t average()const { return value / WindowMs; }

      void add_usage( uint64_t units, time_point now )
      {
         if( now != last_update ) {
            auto elapsed = now - last_update;
            if( elapsed > fc::milliseconds(WindowMs) ) {
               value  = 0;
            } else if( elapsed > fc::days(0) ) {
               value *= (fc::milliseconds(WindowMs) - elapsed).count();
               value /=  fc::milliseconds(WindowMs).count();
            }
            last_update    = now;
         }
         value += units * Precision;
      }
   };

   /**
    * Every account that authorizes a transaction is billed for the full size of that transaction. This object
    * tracks the average usage of that account.
    */
   struct resource_limits_object : public chainbase::object<resource_limits_object_type, resource_limits_object> {

      OBJECT_CTOR(resource_limits_object)

      id_type id;
      account_name owner;
      bool pending = false;

      int64_t net_weight = 0;
      int64_t cpu_weight = 0;
      int64_t ram_bytes = 0;

   };

   struct by_owner;
   using resource_limits_index = chainbase::shared_multi_index_container<
      resource_limits_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<resource_limits_object, resource_limits_object::id_type, &resource_limits_object::id>>,
         ordered_unique<tag<by_owner>,
            composite_key<resource_limits_object,
               BOOST_MULTI_INDEX_MEMBER(resource_limits_object, bool, pending),
               BOOST_MULTI_INDEX_MEMBER(resource_limits_object, account_name, owner)
            >
         >
      >
   >;

   struct resource_usage_object : public chainbase::object<resource_usage_object_type, resource_usage_object> {
      OBJECT_CTOR(resource_usage_object)

      id_type id;
      account_name owner;

      average_accumulator<config::bandwidth_average_window_ms> net_usage;
      average_accumulator<config::compute_average_window_ms>   cpu_usage;
      uint64_t                                                 ram_usage = 0;
   };

   using resource_usage_index = chainbase::shared_multi_index_container<
      resource_usage_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<resource_usage_object, resource_usage_object::id_type, &resource_usage_object::id>>,
         ordered_unique<tag<by_owner>, member<resource_usage_object, account_name, &resource_usage_object::owner> >
      >
   >;

   class resource_limits_state_object : public chainbase::object<resource_limits_state_object_type, resource_limits_state_object> {
      OBJECT_CTOR(resource_limits_state_object);

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

      uint64_t total_net_weight = 0;
      uint64_t total_cpu_weight = 0;
      uint64_t total_ram_bytes = 0;

      /* TODO: we have to guarantee that we don't over commit our ram.  This can be tricky if multiple threads are
       * updating limits in parallel.  For now, we are not running in parallel so this is a temporary measure that
       * allows us to quarantine this guarantee from the rest of the otherwise parallel-compatible code
       */
      uint64_t speculative_ram_bytes = 0;

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

   };

} } /// eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::resource_limits_object, eosio::chain::resource_limits_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::resource_usage_object_type, eosio::chain::resource_usage_index)
