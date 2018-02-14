#pragma once

extern "C" {

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

   void set_resource_limits( account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight, int64_t ignored);

   void set_active_producers( char *producer_data, size_t producer_data_size );

   ///@ } privilegedcapi
}
