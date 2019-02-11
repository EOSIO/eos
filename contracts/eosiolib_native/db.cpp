/**
 *  @file db.h
 *  @copyright defined in eos/LICENSE
 *  @brief Defines C API for interfacing with blockchain database
 */

#include <eosiolib/db.h>

#include "vm_api.h"

int32_t db_store_i64(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id,  const void* data, uint32_t len) {
   return get_vm_api()->db_store_i64(scope, table, payer, id, (const char*)data, len);
}

int32_t db_store_i64_ex(uint64_t code, uint64_t scope, uint64_t table, uint64_t payer, uint64_t id,  const void* data, uint32_t len) {
   return get_vm_api()->db_store_i64_ex(code, scope, table, payer, id, (const char*)data, len);
}


void db_update_i64(int32_t iterator, uint64_t payer, const void* data, uint32_t len) {
   get_vm_api()->db_update_i64( iterator, payer, (const char*)data, len);
}

void db_remove_i64(int32_t iterator) {
   get_vm_api()->db_remove_i64(iterator);
}

void db_update_i64_ex( uint64_t scope, uint64_t payer, uint64_t table, uint64_t id, const char* buffer, size_t buffer_size ) {
   get_vm_api()->db_update_i64_ex(scope, payer, table, id, buffer, buffer_size);
}

void db_remove_i64_ex( uint64_t scope, uint64_t payer, uint64_t table, uint64_t id ) {
   get_vm_api()->db_remove_i64_ex(scope, payer, table, id);
}

int32_t db_get_i64(int32_t iterator, void* data, uint32_t len) {
   return get_vm_api()->db_get_i64(iterator, data, len);
}

int32_t db_get_i64_ex( int itr, uint64_t* primary, char* buffer, size_t buffer_size ) {
   return get_vm_api()->db_get_i64_ex( itr, primary, buffer, buffer_size );
}

const char* db_get_i64_exex( int itr, size_t* buffer_size ) {
   return get_vm_api()->db_get_i64_exex( itr,  buffer_size);
}

int32_t db_next_i64(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_next_i64(iterator, primary);
}

int32_t db_previous_i64(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_previous_i64(iterator, primary);
}

int32_t db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
   return get_vm_api()->db_find_i64(code, scope, table, id);
}

int32_t db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
   return get_vm_api()->db_lowerbound_i64(code, scope, table, id);
}

int32_t db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
   return get_vm_api()->db_upperbound_i64(code, scope, table, id);
}

int32_t db_end_i64(uint64_t code, uint64_t scope, uint64_t table) {
   return get_vm_api()->db_end_i64(code, scope, table);
}

int32_t db_idx64_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const uint64_t* secondary) {
   return get_vm_api()->db_idx64_store(scope, table, payer, id, secondary);
}

void db_idx64_update(int32_t iterator, uint64_t payer, const uint64_t* secondary) {
   get_vm_api()->db_idx64_update(iterator, payer, secondary);
}

void db_idx64_remove(int32_t iterator) {
   get_vm_api()->db_idx64_remove(iterator);
}

int32_t db_idx64_next(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx64_next(iterator, primary);
}

int32_t db_idx64_previous(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx64_previous(iterator, primary);
}

int32_t db_idx64_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint64_t* secondary, uint64_t primary) {
   return get_vm_api()->db_idx64_find_primary(code, scope, table, secondary, primary);
}

int32_t db_idx64_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint64_t* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx64_find_secondary(code, scope, table, secondary, primary);
}

int32_t db_idx64_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx64_lowerbound(code, scope, table, secondary, primary);
}

int32_t db_idx64_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx64_upperbound(code, scope, table, secondary, primary);
}

int32_t db_idx64_end(uint64_t code, uint64_t scope, uint64_t table) {
   return get_vm_api()->db_idx64_end(code, scope, table);
}

int32_t db_idx128_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const uint128_t* secondary) {
   return get_vm_api()->db_idx128_store(scope, table, payer, id, secondary);
}

void db_idx128_update(int32_t iterator, uint64_t payer, const uint128_t* secondary) {
   get_vm_api()->db_idx128_update(iterator, payer, secondary);
}

void db_idx128_remove(int32_t iterator) {
   get_vm_api()->db_idx128_remove(iterator);
}

int32_t db_idx128_next(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx128_next(iterator, primary);
}

int32_t db_idx128_previous(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx128_previous(iterator, primary) ;
}

int32_t db_idx128_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t* secondary, uint64_t primary) {
   return get_vm_api()->db_idx128_find_primary(code, scope, table, secondary, primary);
}

int32_t db_idx128_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx128_find_secondary(code, scope, table, secondary, primary);
}

int32_t db_idx128_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx128_lowerbound(code, scope, table, secondary, primary);
}

int32_t db_idx128_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx128_upperbound(code, scope, table, secondary, primary);
}

int32_t db_idx128_end(uint64_t code, uint64_t scope, uint64_t table) {
   return get_vm_api()->db_idx128_end(code, scope, table);
}

int32_t db_idx256_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const uint128_t* data, uint32_t data_len ) {
   return get_vm_api()->db_idx256_store(scope, table, payer, id, data, data_len );
}

void db_idx256_update(int32_t iterator, uint64_t payer, const uint128_t* data, uint32_t data_len) {
   get_vm_api()->db_idx256_update(iterator, payer, data, data_len);
}

void db_idx256_remove(int32_t iterator) {
   get_vm_api()->db_idx256_remove(iterator);
}

int32_t db_idx256_next(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx256_next(iterator, primary);
}

int32_t db_idx256_previous(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx256_previous(iterator, primary);
}

int32_t db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data, uint32_t data_len, uint64_t primary) {
   return get_vm_api()->db_idx256_find_primary(code, scope, table, data, data_len, primary);
}

int32_t db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t* data, uint32_t data_len, uint64_t* primary) {
   return get_vm_api()->db_idx256_find_secondary(code, scope, table, data, data_len, primary);
}

int32_t db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data, uint32_t data_len, uint64_t* primary) {
   return get_vm_api()->db_idx256_lowerbound(code, scope, table, data, data_len, primary);
}

int32_t db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data, uint32_t data_len, uint64_t* primary) {
   return get_vm_api()->db_idx256_upperbound(code, scope, table, data, data_len, primary);
}

int32_t db_idx256_end(uint64_t code, uint64_t scope, uint64_t table) {
   return get_vm_api()->db_idx256_end(code, scope, table);
}

int32_t db_idx_double_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const double* secondary) {
   return get_vm_api()->db_idx_double_store(scope, table, payer, id, (const float64_t*)secondary);
}

void db_idx_double_update(int32_t iterator, uint64_t payer, const double* secondary) {
   get_vm_api()->db_idx_double_update(iterator, payer, (const float64_t*)secondary);
}

void db_idx_double_remove(int32_t iterator) {
   get_vm_api()->db_idx_double_remove(iterator);
}

int32_t db_idx_double_next(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx_double_next(iterator, primary);
}

int32_t db_idx_double_previous(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx_double_previous(iterator, primary);
}

int32_t db_idx_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, double* secondary, uint64_t primary) {
   return get_vm_api()->db_idx_double_find_primary(code, scope, table, (float64_t*)secondary, primary);
}

int32_t db_idx_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const double* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx_double_find_secondary(code, scope, table, (const float64_t*)secondary, primary);
}

int32_t db_idx_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, double* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx_double_lowerbound(code, scope, table, (float64_t*)secondary, primary);
}

int32_t db_idx_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, double* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx_double_upperbound(code, scope, table, (float64_t*)secondary, primary);
}

int32_t db_idx_double_end(uint64_t code, uint64_t scope, uint64_t table) {
   return get_vm_api()->db_idx_double_end(code, scope, table);
}

int32_t db_idx_long_double_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const long double* secondary) {
   return get_vm_api()->db_idx_long_double_store(scope, table, payer, id, (const float128_t*)secondary);
}

void db_idx_long_double_update(int32_t iterator, uint64_t payer, const long double* secondary) {
   get_vm_api()->db_idx_long_double_update(iterator, payer, (const float128_t*)secondary);
}

void db_idx_long_double_remove(int32_t iterator) {
   get_vm_api()->db_idx_long_double_remove(iterator);
}

int32_t db_idx_long_double_next(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx_long_double_next(iterator, primary);
}

int32_t db_idx_long_double_previous(int32_t iterator, uint64_t* primary) {
   return get_vm_api()->db_idx_long_double_previous(iterator, primary);
}

int32_t db_idx_long_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, long double* secondary, uint64_t primary) {
   return get_vm_api()->db_idx_long_double_find_primary(code, scope, table, (float128_t*)secondary, primary);
}

int32_t db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const long double* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx_long_double_find_secondary(code, scope, table, (const float128_t*)secondary, primary);
}

int32_t db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, long double* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx_long_double_lowerbound(code, scope, table, (float128_t*)secondary, primary);
}

int32_t db_idx_long_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, long double* secondary, uint64_t* primary) {
   return get_vm_api()->db_idx_long_double_upperbound(code, scope, table, (float128_t*)secondary, primary);
}
int32_t db_idx_long_double_end(uint64_t code, uint64_t scope, uint64_t table) {
   return get_vm_api()->db_idx_long_double_end(code, scope, table);
}


int db_store_i256( uint64_t scope, uint64_t table, uint64_t payer, void* id, int size, const char* buffer, size_t buffer_size ) {
   return get_vm_api()->db_store_i256(scope, table, payer, id, size, buffer, buffer_size);
}

void db_update_i256( int iterator, uint64_t payer, const char* buffer, size_t buffer_size ) {
   return get_vm_api()->db_update_i256(iterator, payer, buffer, buffer_size);
}

void db_remove_i256( int iterator ) {
   return get_vm_api()->db_remove_i256(iterator);
}

int db_get_i256( int iterator, char* buffer, size_t buffer_size ) {
   return get_vm_api()->db_get_i256(iterator, buffer, buffer_size);
}

int db_find_i256( uint64_t code, uint64_t scope, uint64_t table, void* id, int size ) {
   return get_vm_api()->db_find_i256(code, scope, table, id, size);
}

int db_upperbound_i256( uint64_t code, uint64_t scope, uint64_t table, char* id, int size ) {
   return get_vm_api()->db_upperbound_i256(code, scope, table, id, size);
}

int db_lowerbound_i256( uint64_t code, uint64_t scope, uint64_t table, char* id, int size ) {
   return get_vm_api()->db_lowerbound_i256(code, scope, table, id, size);
}
