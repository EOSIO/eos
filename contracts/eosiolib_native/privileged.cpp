#include <eosiolib/types.h>
#include <eosiolib/privileged.h>

#include "vm_api.h"

extern "C" int is_feature_active( int64_t feature_name ) {
   return false;
}

void set_resource_limits( uint64_t account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight ) {
   get_vm_api()->set_resource_limits( account, ram_bytes, net_weight, cpu_weight );
}

int64_t set_proposed_producers( char *producer_data, uint32_t producer_data_size ) {
   return get_vm_api()->set_proposed_producers( producer_data, producer_data_size );
}

bool is_privileged( uint64_t account )  {
   return get_vm_api()->is_privileged( account );
}

void set_privileged( uint64_t account, bool is_priv ) {
   get_vm_api()->set_privileged( account, is_priv );
}

void set_blockchain_parameters_packed(char* data, uint32_t datalen) {
   get_vm_api()->set_blockchain_parameters_packed(data, datalen);
}

uint32_t get_blockchain_parameters_packed(char* data, uint32_t datalen) {
   return get_vm_api()->get_blockchain_parameters_packed(data, datalen);
}

void activate_feature( int64_t f ) {
   get_vm_api()->activate_feature( f );
}


