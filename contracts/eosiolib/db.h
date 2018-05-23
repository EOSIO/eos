/**
 *  @file db.h
 *  @copyright defined in eos/LICENSE.txt
 *  @brief Defines C API for interfacing with blockchain database
 */
#pragma once

#include <eosiolib/types.h>
extern "C" {
/**
 *  @defgroup database Database API
 *  @brief APIs that store and retreive data on the blockchain
 *  @ingroup contractdev
 *
 *  EOS.IO organizes data according to the following broad structure:
 *
 *  - **code** - the account name which has write permission
 *     - **scope** - an account where the data is stored
 *        - **table** - a name for the table that is being stored
 *           - **record** - a row in the table
 *
 *  Every transaction specifies the set of valid scopes that may be read and/or written
 *  to. The code that is running determines what can be written to; therefore, write operations
 *  do not allow you to specify/configure the code.
 *
 *  @note Attempts to read and/or write outside the valid scope and/or code sections will
 *  cause your transaction to fail.
 *
 *
 *  @section tabletypes table Types
 *  These are the supported table types identified by the number and
 *  size of the index:
 *
 *  1. @ref dbi64
 *
 *  The @ref databaseCpp provides a simple interface for storing any fixed-size struct as
 *  a database row.
 *
 */

/**
 *  @defgroup databaseC Database C API
 *  @brief C APIs for interfacing with the database.
 *  @ingroup database
 */

int32_t db_store_i64(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id,  const void* data, uint32_t len);
void db_update_i64(int32_t iterator, uint64_t payer, const void* data, uint32_t len);
void db_remove_i64(int32_t iterator);
int32_t db_get_i64(int32_t iterator, const void* data, uint32_t len);
int32_t db_next_i64(int32_t iterator, uint64_t* primary);
int32_t db_previous_i64(int32_t iterator, uint64_t* primary);
int32_t db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id);
int32_t db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id);
int32_t db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id);
int32_t db_end_i64(uint64_t code, uint64_t scope, uint64_t table);

int32_t db_idx64_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const uint64_t* secondary);
void db_idx64_update(int32_t iterator, uint64_t payer, const uint64_t* secondary);
void db_idx64_remove(int32_t iterator);
int32_t db_idx64_next(int32_t iterator, uint64_t* primary);
int32_t db_idx64_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx64_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint64_t* secondary, uint64_t primary);
int32_t db_idx64_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint64_t* secondary, uint64_t* primary);
int32_t db_idx64_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t* secondary, uint64_t* primary);
int32_t db_idx64_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t* secondary, uint64_t* primary);
int32_t db_idx64_end(uint64_t code, uint64_t scope, uint64_t table);

int32_t db_idx128_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const uint128_t* secondary);
void db_idx128_update(int32_t iterator, uint64_t payer, const uint128_t* secondary);
void db_idx128_remove(int32_t iterator);
int32_t db_idx128_next(int32_t iterator, uint64_t* primary);
int32_t db_idx128_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx128_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t* secondary, uint64_t primary);
int32_t db_idx128_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t* secondary, uint64_t* primary);
int32_t db_idx128_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* secondary, uint64_t* primary);
int32_t db_idx128_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* secondary, uint64_t* primary);
int32_t db_idx128_end(uint64_t code, uint64_t scope, uint64_t table);

int32_t db_idx256_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const void* data, uint32_t data_len );
void db_idx256_update(int32_t iterator, uint64_t payer, const void* data, uint32_t data_len);
void db_idx256_remove(int32_t iterator);
int32_t db_idx256_next(int32_t iterator, uint64_t* primary);
int32_t db_idx256_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, void* data, uint32_t data_len, uint64_t primary);
int32_t db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const void* data, uint32_t data_len, uint64_t* primary);
int32_t db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, void* data, uint32_t data_len, uint64_t* primary);
int32_t db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, void* data, uint32_t data_len, uint64_t* primary);
int32_t db_idx256_end(uint64_t code, uint64_t scope, uint64_t table);

int32_t db_idx_double_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const double* secondary);
void db_idx_double_update(int32_t iterator, uint64_t payer, const double* secondary);
void db_idx_double_remove(int32_t iterator);
int32_t db_idx_double_next(int32_t iterator, uint64_t* primary);
int32_t db_idx_double_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, double* secondary, uint64_t primary);
int32_t db_idx_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const double* secondary, uint64_t* primary);
int32_t db_idx_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, double* secondary, uint64_t* primary);
int32_t db_idx_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, double* secondary, uint64_t* primary);
int32_t db_idx_double_end(uint64_t code, uint64_t scope, uint64_t table);

int32_t db_idx_long_double_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const long double* secondary);
void db_idx_long_double_update(int32_t iterator, uint64_t payer, const long double* secondary);
void db_idx_long_double_remove(int32_t iterator);
int32_t db_idx_long_double_next(int32_t iterator, uint64_t* primary);
int32_t db_idx_long_double_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx_long_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, long double* secondary, uint64_t primary);
int32_t db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const long double* secondary, uint64_t* primary);
int32_t db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, long double* secondary, uint64_t* primary);
int32_t db_idx_long_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, long double* secondary, uint64_t* primary);
int32_t db_idx_long_double_end(uint64_t code, uint64_t scope, uint64_t table);

}
