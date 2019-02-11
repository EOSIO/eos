/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <eosiolib/chain.h>

#include "vm_api.h"

uint32_t get_active_producers( account_name* producers, uint32_t datalen ) {
   return get_vm_api()->get_active_producers(producers, datalen);
}

