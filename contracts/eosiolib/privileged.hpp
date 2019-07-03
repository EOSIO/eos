#pragma once
#include "privileged.h"
#include "serialize.hpp"
#include "types.h"

namespace eosio {

   /**
    * @defgroup privilegedcppapi Privileged C++ API
    * @ingroup privilegedapi
    * @brief Defines C++ Privileged API
    *
    * @{
    */

   /**
    * Tunable blockchain configuration that can be changed via consensus
    *
    * @brief Tunable blockchain configuration that can be changed via consensus
    */
   struct blockchain_parameters {

   uint32_t   base_per_transaction_net_usage;      ///< the base amount of net usage billed for a transaction to cover incidentals
   uint32_t   context_free_discount_net_usage_num; ///< the numerator for the discount on net usage of context-free data
   uint32_t   context_free_discount_net_usage_den; ///< the denominator for the discount on net usage of context-free data

   uint32_t   min_transaction_cpu_usage;           ///< the minimum billable cpu usage (in microseconds) that the chain requires

   uint64_t   min_transaction_ram_usage;           ///< the minimum billable ram usage (in bytes) that the chain requires

   uint32_t   max_transaction_lifetime;            ///< the maximum number of seconds that an input transaction's expiration can be ahead of the time of the block in which it is first included
   uint32_t   deferred_trx_expiration_window;      ///< the number of seconds after the time a deferred transaction can first execute until it expires
   uint32_t   max_transaction_delay;               ///< the maximum number of seconds that can be imposed as a delay requirement by authorization checks
   uint32_t   max_inline_action_size;              ///< maximum allowed size (in bytes) of an inline action
   uint16_t   max_inline_action_depth;             ///< recursion depth limit on sending inline actions
   uint16_t   max_authority_depth;                 ///< recursion depth limit for checking if an authority is satisfied

   uint64_t   ram_size;
   uint64_t   reserved_ram_size;

   std::vector<uint64_t> max_block_usage;
   std::vector<uint64_t> max_transaction_usage;
   
   std::vector<uint64_t> target_virtual_limits;
   std::vector<uint64_t> min_virtual_limits;
   std::vector<uint64_t> max_virtual_limits;
   std::vector<uint32_t> usage_windows;
   
   std::vector<uint16_t> virtual_limit_decrease_pct;
   std::vector<uint16_t> virtual_limit_increase_pct;

   std::vector<uint32_t> account_usage_windows;


      EOSLIB_SERIALIZE( blockchain_parameters,
           (base_per_transaction_net_usage)
           (context_free_discount_net_usage_num)(context_free_discount_net_usage_den)

           (min_transaction_cpu_usage)
           (min_transaction_ram_usage)

           (max_transaction_lifetime)(deferred_trx_expiration_window)(max_transaction_delay)
           (max_inline_action_size)(max_inline_action_depth)(max_authority_depth)

           (ram_size)(reserved_ram_size)
           
           (max_block_usage)(max_transaction_usage)
           
           (target_virtual_limits)(min_virtual_limits)(max_virtual_limits)(usage_windows)
           (virtual_limit_decrease_pct)(virtual_limit_increase_pct)(account_usage_windows)
      )
   };

   /**
    * @brief Set the blockchain parameters
    * Set the blockchain parameters
    * @param params - New blockchain parameters to set
    */
   void set_blockchain_parameters(const eosio::blockchain_parameters& params);

   /**
    * @brief Retrieve the blolckchain parameters
    * Retrieve the blolckchain parameters
    * @param params - It will be replaced with the retrieved blockchain params
    */
   void get_blockchain_parameters(eosio::blockchain_parameters& params);

   ///@} priviledgedcppapi

   /**
   *  @defgroup producertype Producer Type
   *  @ingroup types
   *  @brief Defines producer type
   *
   *  @{
   */

   /**
    * Maps producer with its signing key, used for producer schedule
    *
    * @brief Maps producer with its signing key
    */
   struct producer_key {

      /**
       * Name of the producer
       *
       * @brief Name of the producer
       */
      account_name     producer_name;

      /**
       * Block signing key used by this producer
       *
       * @brief Block signing key used by this producer
       */
      public_key       block_signing_key;

      friend bool operator < ( const producer_key& a, const producer_key& b ) {
         return a.producer_name < b.producer_name;
      }

      EOSLIB_SERIALIZE( producer_key, (producer_name)(block_signing_key) )
   };
}
