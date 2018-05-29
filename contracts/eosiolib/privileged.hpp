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

      uint64_t max_block_net_usage;

      uint32_t target_block_net_usage_pct;

      uint32_t max_transaction_net_usage;

      /**
       * The base amount of net usage billed for a transaction to cover incidentals
       * @brief The base amount of net usage billed for a transaction to cover incidentals
       */
      uint32_t base_per_transaction_net_usage;

      uint32_t net_usage_leeway;

      uint32_t context_free_discount_net_usage_num;

      uint32_t context_free_discount_net_usage_den;

      uint32_t max_block_cpu_usage;

      uint32_t target_block_cpu_usage_pct;

      uint32_t max_transaction_cpu_usage;

      uint32_t min_transaction_cpu_usage;


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
       * Maximum lifetime of a transacton
       *
       * @brief Maximum lifetime of a transacton
       */
      uint32_t max_transaction_lifetime;

      uint32_t deferred_trx_expiration_window;

      uint32_t max_transaction_delay;

      /**
       * Maximum size of inline action
       *
       * @brief Maximum size of inline action
       */
      uint32_t max_inline_action_size;

      /**
       * Maximum depth of inline action
       *
       * @brief Maximum depth of inline action
       */
      uint16_t max_inline_action_depth;

      /**
       * Maximum authority depth
       *
       * @brief Maximum authority depth
       */
      uint16_t max_authority_depth;


      EOSLIB_SERIALIZE( blockchain_parameters,
                        (max_block_net_usage)(target_block_net_usage_pct)
                        (max_transaction_net_usage)(base_per_transaction_net_usage)(net_usage_leeway)
                        (context_free_discount_net_usage_num)(context_free_discount_net_usage_den)

                        (max_block_cpu_usage)(target_block_cpu_usage_pct)
                        (max_transaction_cpu_usage)(min_transaction_cpu_usage)

                        (max_transaction_lifetime)(deferred_trx_expiration_window)(max_transaction_delay)
                        (max_inline_action_size)(max_inline_action_depth)(max_authority_depth)
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
