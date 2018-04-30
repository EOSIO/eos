#pragma once

#ifdef __cplusplus
extern "C" {
#endif

   /**
    * @defgroup privilegedapi Privileged API
    * @ingroup systemapi
    * @brief Defines an API for accessing configuration of the chain that can only be done by privileged accounts
    */

   /**
    * @defgroup privilegedcapi Privileged C API
    * @ingroup privilegedapi
    * @brief Define C Privileged API
    *
    * @{
    */

   void set_resource_limits( account_name account, uint64_t ram_bytes, uint64_t net_weight, uint64_t cpu_weight );

   bool set_active_producers( char *producer_data, uint32_t producer_data_size );

   bool is_privileged( account_name account );

   void set_privileged( account_name account, bool is_priv );

   void     set_blockchain_parameters_packed(char* data, uint32_t datalen);

   /**
    * Retrieve the blolckchain parameters
    * @param data - output buffer of the blockchain parameters, only retrieved if sufficent size to hold packed data.
    * @param datalen - size of the data buffer, 0 to report required size.
    */
   uint32_t get_blockchain_parameters_packed(char* data, uint32_t datalen);

   void activate_feature( int64_t f );

   ///@ } privilegedcapi
#ifdef __cplusplus
}
#endif
