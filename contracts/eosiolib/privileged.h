#pragma once

#ifdef __cplusplus
extern "C" {
#endif

   /**
    * @defgroup privilegedapi Privileged API
    * @ingroup systemapi
    * @brief Defines an API for accessing configuration of the chain that can only be done by privileged accounts
    */

   void set_resource_limits( account_name account, uint64_t ram_bytes, uint64_t net_weight, uint64_t cpu_weight, int64_t ignored);

   void set_active_producers( char *producer_data, uint32_t producer_data_size );

   bool is_privileged( account_name account );

   void     set_blockchain_parameters_packed(char* data, uint32_t datalen);

   uint32_t get_blockchain_parameters_packed(char* data, uint32_t datalen);

   ///@ } privilegedcapi
#ifdef __cplusplus
}
#endif
