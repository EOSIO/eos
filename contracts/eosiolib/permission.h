/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/types.h>

extern "C" {
   /**
    *  @brief Checks if a transaction is authorized by a provided set of keys and permissions
    *
    *  @param trx_data - pointer to the start of the serialized transaction
    *  @param trx_size - size (in bytes) of the serialized transaction
    *  @param pubkeys_data - pointer to the start of the serialized vector of provided public keys
    *  @param pubkeys_size  - size (in bytes) of serialized vector of provided public keys (can be 0 if no public keys are to be provided)
    *  @param perms_data - pointer to the start of the serialized vector of provided permissions (empty permission name acts as wildcard)
    *  @param perms_size - size (in bytes) of the serialized vector of provided permissions
    *
    *  @return 1 if the transaction is authorized, 0 otherwise
    */
   int32_t
   check_transaction_authorization( const char* trx_data,     uint32_t trx_size,
                                    const char* pubkeys_data, uint32_t pubkeys_size,
                                    const char* perms_data,   uint32_t perms_size
                                  );

   /**
    *  @brief Checks if a permission is authorized by a provided delay and a provided set of keys and permissions
    *
    *  @param account    - the account owner of the permission
    *  @param permission - the name of the permission to check for authorization
    *  @param pubkeys_data - pointer to the start of the serialized vector of provided public keys
    *  @param pubkeys_size  - size (in bytes) of serialized vector of provided public keys (can be 0 if no public keys are to be provided)
    *  @param perms_data - pointer to the start of the serialized vector of provided permissions (empty permission name acts as wildcard)
    *  @param perms_size - size (in bytes) of the serialized vector of provided permissions
    *  @param delay_us - the provided delay in microseconds (cannot exceed INT64_MAX)
    *
    *  @return 1 if the permission is authorized, 0 otherwise
    */
   int32_t
   check_permission_authorization( account_name account,
                                   permission_name permission,
                                   const char* pubkeys_data, uint32_t pubkeys_size,
                                   const char* perms_data,   uint32_t perms_size,
                                   uint64_t delay_us
                                 );

   /**
    *  @brief Returns the last used time of a permission
    *
    *  @param account    - the account owner of the permission
    *  @param permission - the name of the permission
    *
    *  @return the last used time (in microseconds since Unix epoch) of the permission
    */
   int64_t get_permission_last_used( account_name account, permission_name permission );


   /**
    *  @brief Returns the creation time of an account
    *
    *  @param account    - the account
    *
    *  @return the creation time (in microseconds since Unix epoch) of the account
    */
   int64_t get_account_creation_time( account_name account );

}
