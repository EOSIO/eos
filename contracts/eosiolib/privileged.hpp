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
      /**
       * The base amount of net usage billed for a transaction to cover incidentals
       * @brief The base amount of net usage billed for a transaction to cover incidentals
       */
      uint32_t base_per_transaction_net_usage;

      /**
       * The base amount of cpu usage billed for a transaction to cover incidentals
       * 
       * @brief The base amount of cpu usage billed for a transaction to cover incidentals
       */
      uint32_t base_per_transaction_cpu_usage;

      /**
       * The base amount of cpu usage billed for an action to cover incidentals
       * 
       * @brief The base amount of cpu usage billed for an action to cover incidentals
       */
      uint32_t base_per_action_cpu_usage;
      /**
       * The base amount of cpu usage billed for a setcode action to cover compilation/etc
       * 
       * @brief The base amount of cpu usage billed for a setcode action to cover compilation/etc
       */
      uint32_t base_setcode_cpu_usage;
      /**
       * The cpu usage billed for every signature on a transaction
       * 
       * @brief The cpu usage billed for every signature on a transaction
       */
      uint32_t per_signature_cpu_usage;
      /**
       * The net usage billed for every lock on a transaction to cover overhead in the block shards
       * 
       * @brief The net usage billed for every lock on a transaction to cover overhead in the block shards
       */
      uint32_t per_lock_net_usage;

      /**
       * The numerator for the discount on cpu usage for CFA's
       * 
       * @brief The numerator for the discount on cpu usage for CFA's
       */
      uint64_t context_free_discount_cpu_usage_num;

      /**
       * The denominator for the discount on cpu usage for CFA's
       * 
       * @brief The denominator for the discount on cpu usage for CFA's

       */
      uint64_t context_free_discount_cpu_usage_den;

      /**
       * The maximum objectively measured cpu usage that the chain will allow regardless of account limits
       * 
       * @brief The maximum objectively measured cpu usage that the chain will allow regardless of account limits
       */
      uint32_t max_transaction_cpu_usage;
      
      /**
       * The maximum objectively measured net usage that the chain will allow regardless of account limits
       * 
       * @brief The maximum objectively measured net usage that the chain will allow regardless of account limits
       */
      uint32_t max_transaction_net_usage;

      /**
       * The maxiumum cpu usage in instructions for a block
       * 
       * @brief The maxiumum cpu usage in instructions for a block
       */
      uint64_t max_block_cpu_usage;

      /**
       * The target percent (1% == 100, 100%= 10,000) of maximum cpu usage; exceeding this triggers congestion handling
       * 
       * @brief The target percent (1% == 100, 100%= 10,000) of maximum cpu usage; exceeding this triggers congestion handling
       */
      uint32_t target_block_cpu_usage_pct;

      /**
       * The maxiumum net usage in instructions for a block
       * 
       * @brief The maxiumum net usage in instructions for a block
       */
      uint64_t max_block_net_usage;

      /**
       * The target percent (1% == 100, 100%= 10,000) of maximum net usage; exceeding this triggers congestion handling
       * 
       * @brief The target percent (1% == 100, 100%= 10,000) of maximum net usage; exceeding this triggers congestion handling
       */
      uint32_t target_block_net_usage_pct;

      /**
       * Maximum lifetime of a transacton
       * 
       * @brief Maximum lifetime of a transacton
       */
      uint32_t max_transaction_lifetime;

      /**
       * Maximum execution time of a transaction
       * 
       * @brief Maximum execution time of a transaction
       */
      uint32_t max_transaction_exec_time;

      /**
       * Maximum authority depth 
       * 
       * @brief Maximum authority depth 
       */
      uint16_t max_authority_depth;

      /**
       * Maximum depth of inline action
       * 
       * @brief Maximum depth of inline action
       */
      uint16_t max_inline_depth;

      /**
       * Maximum size of inline action
       * 
       * @brief Maximum size of inline action
       */
      uint32_t max_inline_action_size;

      /**
       * Maximum number of generated transaction
       * 
       * @brief Maximum number of generated transaction
       */
      uint32_t max_generated_transaction_count;

      /**
       * Maximum delay of a transaction
       * 
       * @brief Maximum delay of a transaction
       */
      uint32_t max_transaction_delay;

      EOSLIB_SERIALIZE( blockchain_parameters,
                        (base_per_transaction_net_usage)(base_per_transaction_cpu_usage)(base_per_action_cpu_usage)
                        (base_setcode_cpu_usage)(per_signature_cpu_usage)(per_lock_net_usage)
                        (context_free_discount_cpu_usage_num)(context_free_discount_cpu_usage_den)
                        (max_transaction_cpu_usage)(max_transaction_net_usage)
                        (max_block_cpu_usage)(target_block_cpu_usage_pct)
                        (max_block_net_usage)(target_block_net_usage_pct)
                        (max_transaction_lifetime)(max_transaction_exec_time)(max_authority_depth)
                        (max_inline_depth)(max_inline_action_size)(max_generated_transaction_count)
                        (max_transaction_delay)
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
   
   struct producer_key {
      account_name producer_name;
      public_key   block_signing_key;

      EOSLIB_SERIALIZE( producer_key, (producer_name)(block_signing_key) )
   };

   struct producer_schedule {
      uint32_t             version = 0; ///< sequentially incrementing version number
      std::vector<producer_key> producers;

      EOSLIB_SERIALIZE( producer_schedule, (version)(producers) )
   };

}
