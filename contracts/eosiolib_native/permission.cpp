/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosiolib/permission.h>

#include "vm_api.h"

int32_t check_transaction_authorization( const char* trx_data,     uint32_t trx_size,
                                 const char* pubkeys_data, uint32_t pubkeys_size,
                                 const char* perms_data,   uint32_t perms_size
                               ) {
   return get_vm_api()->check_transaction_authorization(trx_data, trx_size,
         pubkeys_data, pubkeys_size,
         perms_data,   perms_size
       );
}

int32_t check_permission_authorization( uint64_t account,
                                uint64_t permission,
                                const char* pubkeys_data, uint32_t pubkeys_size,
                                const char* perms_data,   uint32_t perms_size,
                                uint64_t delay_us
                              ) {
   return get_vm_api()->check_permission_authorization( account,
         permission,
         pubkeys_data, pubkeys_size,
         perms_data,   perms_size,
         delay_us
       );
}

int64_t get_permission_last_used( uint64_t account, uint64_t permission ) {
   return get_vm_api()->get_permission_last_used( account, permission );
}

int64_t get_account_creation_time( uint64_t account ) {
   return get_vm_api()->get_account_creation_time( account );
}

