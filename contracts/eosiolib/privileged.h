#pragma once

extern "C" {

   /**
    * @defgroup privilegedapi Privileged API
    * @ingroup systemapi
    * @brief Defines an API for accessing configuration of the chain that can only be done by privileged accounts
    */

   void set_resource_limits( account_name account, uint64_t ram_bytes, uint64_t net_weight, uint64_t cpu_weight, int64_t ignored);

   void set_active_producers( char *producer_data, size_t producer_data_size );

   bool is_privileged( account_name account );

   struct blockchain_parameters {
      uint32_t target_block_size;
      uint32_t max_block_size;

      uint32_t target_block_acts_per_scope;
      uint32_t max_block_acts_per_scope;

      uint32_t target_block_acts; ///< regardless of the amount of parallelism, this defines target compute time per block
      uint32_t max_block_acts; ///< regardless of the amount of parallelism, this maximum compute time per block

      uint64_t max_storage_size;
      uint32_t max_transaction_lifetime;
      uint32_t max_transaction_exec_time;
      uint16_t max_authority_depth;
      uint16_t max_inline_depth;
      uint32_t max_inline_action_size;
      uint32_t max_generated_transaction_size;
   };

   void set_blockchain_parameters(const struct blockchain_parameters* params);

   void get_blockchain_parameters(struct blockchain_parameters* params);

   ///@ } privilegedcapi
}
