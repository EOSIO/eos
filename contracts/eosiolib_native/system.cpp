/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <eosiolib/system.h>

#include "vm_api.h"

extern "C" {

void  eosio_assert( uint32_t test, const char* msg ) {
   get_vm_api()->eosio_assert( test, msg );
}

void  eosio_assert_message( uint32_t test, const char* msg, uint32_t msg_len ) {
   get_vm_api()->eosio_assert_message( test, msg, msg_len );
}

void  eosio_assert_code( uint32_t test, uint64_t code ) {
   get_vm_api()->eosio_assert_code( test, code );
}

void  eosio_exit( int32_t code ) {
   get_vm_api()->eosio_exit( code );
}

uint64_t  current_time() {
   return get_vm_api()->current_time();
}

uint32_t  now() {
   return (uint32_t)( current_time() / 1000000 );
}

uint64_t wasm_call(const char* func, uint64_t* args , int argc) {
//   printf("++++++++wasm_call: %s\n", func);
   return get_vm_api()->wasm_call(func, args , argc);
}

uint64_t call(uint64_t account, uint64_t func) {
   return get_vm_api()->call(account, func);
}

int call_get_args(char* args , int len) {
   return get_vm_api()->call_get_args(args, len);
}

int call_set_args(const char* args , int len) {
   return get_vm_api()->call_set_args(args, len);
}

int call_set_results(const char* result , int len) {
   return get_vm_api()->call_set_results(result, len);
}

int call_get_results(char* result , int len) {
   return get_vm_api()->call_get_results(result, len);
}

int has_option(const char* _option) {
   return get_vm_api()->has_option(_option);
}

int get_option(const char* option, char *result, int size) {
   return get_vm_api()->get_option(option, result, size);
}

const char* get_code( uint64_t receiver, size_t* size ) {
   return get_vm_api()->get_code(receiver, size);
}

void set_code(uint64_t user_account, int vm_type, char* code, int code_size) {
   get_vm_api()->set_code(user_account, vm_type, code, code_size);
}

int get_code_id( uint64_t account, char* code_id, size_t size ) {
   return get_vm_api()->get_code_id(account, code_id, size);
}

int get_code_type( uint64_t account) {
   return get_vm_api()->get_code_type(account);
}


}
