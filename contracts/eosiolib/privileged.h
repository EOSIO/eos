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
    * @brief Defines %C Privileged API
    *
    * @{
    */

   /**
    * @brief Set the resource limit of an account
    * Set the resource limit of an account
    * @param account - name of the account whose resource limit to be set
    * @param ram_bytes - ram limit
    * @param net_weight - net limit
    * @param cpu_weight - cput limit
    */
   void set_resource_limits( account_name account, uint64_t ram_bytes, uint64_t net_weight, uint64_t cpu_weight );

   /**
    * @brief Set new active producers
    * Set new active producers. Producers will only be activated once the block which starts the next round is irrreversible
    * @param producer_data - pointer to producer schedule packed as bytes
    * @param producer_data_size - size of the packed producer schedule 
    * @pre `producer_data` is a valid pointer to a range of memory at least `producer_data_size` bytes long that contains serialized produced schedule data
    */
   void set_active_producers( char *producer_data, uint32_t producer_data_size );

   /**
    * @brief Check if an account is privileged
    * Check if an account is privileged
    * @param account - name of the account to be checked
    * @return true if the account is privileged
    * @return false if the account is not privileged
    */
   bool is_privileged( account_name account );

   /**
    * @brief Set the privileged status of an account 
    * Set the privileged status of an account 
    * @param account - name of the account whose privileged account to be set
    * @param is_priv - privileged status
    */
   void set_privileged( account_name account, bool is_priv );
   
   /**
    * @brief Set the blockchain parameters 
    * Set the blockchain parameters 
    * @param data - pointer to blockchain parameters packed as bytes
    * @param datalen - size of the packed blockchain parameters
    * @pre `data` is a valid pointer to a range of memory at least `datalen` bytes long that contains packed blockchain params data
    */
   void     set_blockchain_parameters_packed(char* data, uint32_t datalen);

   /**
    * @brief Retrieve the blolckchain parameters
    * Retrieve the blolckchain parameters
    * @param data - output buffer of the blockchain parameters, only retrieved if sufficent size to hold packed data.
    * @param datalen - size of the data buffer, 0 to report required size.
    * @return size of the blockchain parameters
    * @pre `data` is a valid pointer to a range of memory at least `datalen` bytes long
    * @post `data` is filled with packed blockchain parameters
    */
   uint32_t get_blockchain_parameters_packed(char* data, uint32_t datalen);

   /**
    * @brief Activate new feature
    * Activate new feature
    * @param f - name (identifier) of the feature to be activated
    */
   void activate_feature( int64_t f );

   ///@ } privilegedcapi
#ifdef __cplusplus
}
#endif
