#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>

#include "multi_index_includes.hpp"


namespace eosio { namespace chain {

   template<typename T>
   struct ratio {
      T numerator;
      T denominator;
   };

   template<typename T>
   ratio<T> make_ratio(T n, T d) {
      return ratio<T>{n, d};
   }

   template<typename T>
   T operator* (T value, const ratio<T>& r) {
      return (value * r.numerator) / r.denominator;
   }

   /**
    *  This class accumulates an average value taking periodic inputs.
    *
    *  The value stored is Precision times the sum of the inputs.
    */
   template<uint64_t Precision = config::rate_limiting_precision>
   struct average_accumulator
   {
      average_accumulator(uint32_t window_blocks)
      : window_blocks(window_blocks)
      , last_head_block_num()
      , value(0)
      {
      }

      uint32_t   window_blocks;
      uint32_t   last_head_block_num;
      uint64_t   value;

      /**
       * return the average value in rate_limiting_precision
       */
      uint64_t average()const { return value / window_blocks; }

      void add_usage( uint64_t units, uint32_t head_block_num )
      {
         if( head_block_num != last_head_block_num ) {
            const auto decay = make_ratio(
               (uint64_t) window_blocks - 1,
               (uint64_t) window_blocks
            );

            value = value * decay;
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

   struct elastic_limit_parameters {
      uint64_t target;           // the desired usage
      uint64_t max;              // the maximum usage
      uint32_t periods;          // the number of aggregation periods that contribute to the average usage

      uint32_t max_multiplier;   // the multiplier by which virtual space can oversell usage when uncongested
      ratio    contract_rate;    // the rate at which a congested resource contracts its limit
      ratio    expand_rate;       // the rate at which an uncongested resource expands its limits
   };

   class resource_limits_config_object : public chainbase::object<resource_limits_config_object_type, resource_limits_config_object> {
      OBJECT_CTOR(resource_limits_config_object);
      id_type id;

      uint32_t  base_per_transaction_net_usage;
      uint32_t  base_per_transaction_cpu_usage;

      uint32_t  per_signature_cpu_usage;

      elastic_limit_parameters cpu_limit_parameters;
      elastic_limit_parameters net_limit_parameters;
   };

   class resource_limits_state_object : public chainbase::object<resource_limits_state_object_type, resource_limits_state_object> {
      OBJECT_CTOR(resource_limits_state_object);

      /**
       * Track the average blocksize over the past 60 seconds and use it to adjust the
       * reserve ratio for bandwidth rate limiting calclations.
       */
      average_accumulator<config::blocksize_average_window_ms> average_block_net_usage;

      /**
       * Track the average actions per block over the past 60 seconds and use it to
       * adjust hte reserve ration for action rate limiting calculations
       */
      average_accumulator<config::blocksize_average_window_ms> average_block_cpu_usage;

      void update_virtual_net_limit( const resource_limits_config_object& cfg );
      void update_virtual_cpu_limit( const resource_limits_config_object& cfg );

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
      uint64_t virtual_net_limit = 0;

      /**
       *  Increases when average_bloc
       */
      uint64_t virtual_cpu_limit = 0;

   };

   using resource_limits_config_index = chainbase::shared_multi_index_container<
      resource_limits_config_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<resource_limits_object, resource_limits_object::id_type, &resource_limits_object::id>>
      >
   >;


} } /// eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::resource_limits_object, eosio::chain::resource_limits_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::resource_usage_object_type, eosio::chain::resource_usage_index)
