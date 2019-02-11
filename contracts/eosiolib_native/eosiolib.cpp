#include <eosiolib/types.h>
#include <eosiolib/privileged.hpp>

#include <eosiolib/datastream.hpp>

namespace eosio {

   void set_blockchain_parameters(const eosio::blockchain_parameters& params) {
      char buf[sizeof(eosio::blockchain_parameters)];
      eosio::datastream<char *> ds( buf, sizeof(buf) );
      ds << params;
      set_blockchain_parameters_packed( buf, ds.tellp() );
   }

   void get_blockchain_parameters(eosio::blockchain_parameters& params) {
      char buf[sizeof(eosio::blockchain_parameters)];
      size_t size = get_blockchain_parameters_packed( buf, sizeof(buf) );
      eosio_assert( size <= sizeof(buf), "buffer is too small" );
      eosio::datastream<const char*> ds( buf, size_t(size) );
      ds >> params;
   }

} /// namespace eosio

#include "vm_api.h"
static struct vm_api* s_api = nullptr;
void vm_register_api(struct vm_api* api) {
   if (!api) {
      throw std::runtime_error("vm_api pointer can not be NULL!");
   }
   s_api = api;
}

struct vm_api* get_vm_api() {
   if (!s_api) {
      throw std::runtime_error("vm api not specified!!!");
   }
   return s_api;
}

extern "C" uint64_t string_to_uint64(const char* str) {
   return get_vm_api()->string_to_uint64(str);
}

extern "C" int32_t uint64_to_string(uint64_t n, char* out, int size) {
   return get_vm_api()->uint64_to_string(n, out, size);
}
void vm_api_throw_exception(int type, const char* fmt, ...) {
   get_vm_api()->throw_exception(type, fmt);
}
