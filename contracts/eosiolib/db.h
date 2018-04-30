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


 /**
  *  @defgroup databaseC64BitIndex Single 64 bit Index Table
  *  @brief C APIs for interfacing with a Single 64 bit Index Table.
  *  @ingroup database
  */

/**
  *
  *  Store 64 bit index
  *  @brief Store a 64 bit index- scope	-
  *  @param scope - The account scope that will be read, must exist in the transaction scopes list
  *  @param table - The ID/name of the table within the current scope/code context to modify
  *  @param payer - The account that is paying for this storage
  *  @param id - Id of entry
  *  @param data - Record to store
  *  @param len - Size of data
  *  @pre datalen >= sizeof(data)
  *  @pre data is a valid pointer to a range of memory at least datalen bytes long
  *  @pre ((uint64_t*)data) stores the primary key
  *  @pre scope is declared by the current transaction
  *  @pre this method is being called from an apply context (not validate or precondition)
  *  @return 1 if a new record was created, 0 if an existing record was updated
  */
int32_t db_store_i64(account_name scope, table_name table, account_name payer, uint64_t id,  const void* data, uint32_t len);

/**
  *
  *  Update 64 bit index
  *
  *  @brief Store a 64 bit index- scope	-
  *  @param iterator - The iterator to update
  *  @param table - The ID/name of the table within the current scope/code context to modify
  *  @param payer - The account that is paying for this storage
  *  @param id - Id of entry
  *  @param data - Data to store
  *  @param len - Size of data
  *  @pre datalen >= sizeof(data)
  *  @pre data is a valid pointer to a range of memory at least datalen bytes long
  *  @pre ((uint64_t*)data) stores the primary key
  *  @pre scope is declared by the current transaction
  *  @pre this method is being called from an apply context (not validate or precondition)
  *  @return 1 if a new record was created, 0 if an existing record was updated
  */
void db_update_i64(int32_t iterator, account_name payer, const void* data, uint32_t len);

/**
  *
  *  Remove 64 bit index
  *
  *  @brief Remove 64 bit index
  *  @param iterator - The iterator to remove
  *
  * Example:
  *  @code
  *    int itr = db_find_i64(receiver, receiver, table1, N(alice));
  *  eosio_assert(itr >= 0, "primary_i64_general - db_find_i64");
  *  db_remove_i64(itr);
  *  itr = db_find_i64(receiver, receiver, table1, N(alice));
  * @endcode
  */
void db_remove_i64(int32_t iterator);

/**
  *
  *  Get 64 bit index
  *
  *  @brief Remove Get 64 bit index
  *  @param Iterator - Iterator
  *  @param data - Data to store
  *  @param len - Size of data
  *
  * Example:
  * @code
  * char value[50];
  * auto len = db_get_i64(itr, value, buffer_len);
  * value[len] = '\0';
  * std::string s(value);
  * @endcode
  */
int32_t db_get_i64(int32_t iterator, const void* data, uint32_t len);

/**
  *
  *  Get next record from 64 bit index iterator
  *
  *  @brief Remove 64 bit index
  *  @param iterator - The iterator to remove
  *
  * Example:
  * @code
  * int charlie_itr = db_find_i64(receiver, receiver, table1, N(charlie));
  * // nothing after charlie
  * uint64_t prim = 0
  * int end_itr = db_next_i64(charlie_itr, &prim);
  * @endcode
  */
int32_t db_next_i64(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get next record from 64 bit index iterator
  *
  *  @brief Return previous 64 bit index
  *  @param iterator - The iterator to remove
  *
  * Example:
  * @code
  * uint64_t prim = 123;
  * int itr_prev = db_previous_i64(itr, &prim);
  * @endcode
  */
int32_t db_previous_i64(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find 64 bit index
  *
  *  @brief Find record
  *  @param iterator - The iterator to remove
  *
  * Example:
  * @code
  * int itr = db_find_i64(receiver, receiver, table1, N(charlie));
  * uint64_t prim = 0;
  * int itr_prev = db_previous_i64(itr, &prim);
  * @endcode
  */
int32_t db_find_i64(account_name code, account_name scope, table_name table, uint64_t id);

/**
  *
  *  Find lowerbound of 64 bit index
  *
  *  @brief Find record
  *  @param iterator - The iterator to remove
  *
  * Example:
  * @code
  * int itr = db_find_i64(receiver, receiver, table1, N(charlie));
  * uint64_t prim = 0;
  * int itr_prev = db_previous_i64(itr, &prim);
  * @endcode
  */
int32_t db_lowerbound_i64(account_name code, account_name scope, table_name table, uint64_t id);

/**
  *
  *  Find upperbound of 64 bit index
  *
  *  @brief Find record
  *  @param iterator - The iterator to remove
  *
  * Example:
  * @code
  * int itr = db_find_i64(receiver, receiver, table1, N(charlie));
  * uint64_t prim = 0;
  * int itr_prev = db_previous_i64(itr, &prim);
  * @endcode
  */
int32_t db_upperbound_i64(account_name code, account_name scope, table_name table, uint64_t id);

int32_t db_end_i64(account_name code, account_name scope, table_name table);




int32_t db_idx64_store(account_name scope, table_name table, account_name payer, uint64_t id, const uint64_t* secondary);
void db_idx64_update(int32_t iterator, account_name payer, const uint64_t* secondary);
void db_idx64_remove(int32_t iterator);
int32_t db_idx64_next(int32_t iterator, uint64_t* primary);
int32_t db_idx64_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx64_find_primary(account_name code, account_name scope, table_name table, uint64_t* secondary, uint64_t primary);
int32_t db_idx64_find_secondary(account_name code, account_name scope, table_name table, const uint64_t* secondary, uint64_t* primary);
int32_t db_idx64_lowerbound(account_name code, account_name scope, table_name table, uint64_t* secondary, uint64_t* primary);
int32_t db_idx64_upperbound(account_name code, account_name scope, table_name table, uint64_t* secondary, uint64_t* primary);
int32_t db_idx64_end(account_name code, account_name scope, table_name table);




int32_t db_idx128_store(account_name scope, table_name table, account_name payer, uint64_t id, const uint128_t* secondary);
void db_idx128_update(int32_t iterator, account_name payer, const uint128_t* secondary);
void db_idx128_remove(int32_t iterator);
int32_t db_idx128_next(int32_t iterator, uint64_t* primary);
int32_t db_idx128_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx128_find_primary(account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t primary);
int32_t db_idx128_find_secondary(account_name code, account_name scope, table_name table, const uint128_t* secondary, uint64_t* primary);
int32_t db_idx128_lowerbound(account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t* primary);
int32_t db_idx128_upperbound(account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t* primary);
int32_t db_idx128_end(account_name code, account_name scope, table_name table);



int32_t db_idx256_store(account_name scope, table_name table, account_name payer, uint64_t id, const void* data, uint32_t data_len );
void db_idx256_update(int32_t iterator, account_name payer, const void* data, uint32_t data_len);
void db_idx256_remove(int32_t iterator);
int32_t db_idx256_next(int32_t iterator, uint64_t* primary);
int32_t db_idx256_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx256_find_primary(account_name code, account_name scope, table_name table, void* data, uint32_t data_len, uint64_t primary);
int32_t db_idx256_find_secondary(account_name code, account_name scope, table_name table, const void* data, uint32_t data_len, uint64_t* primary);
int32_t db_idx256_lowerbound(account_name code, account_name scope, table_name table, void* data, uint32_t data_len, uint64_t* primary);
int32_t db_idx256_upperbound(account_name code, account_name scope, table_name table, void* data, uint32_t data_len, uint64_t* primary);
int32_t db_idx256_end(account_name code, account_name scope, table_name table);

int32_t db_idx_double_store(account_name scope, table_name table, account_name payer, uint64_t id, const double* secondary);
void db_idx_double_update(int32_t iterator, account_name payer, const double* secondary);
void db_idx_double_remove(int32_t iterator);
int32_t db_idx_double_next(int32_t iterator, uint64_t* primary);
int32_t db_idx_double_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx_double_find_primary(account_name code, account_name scope, table_name table, double* secondary, uint64_t primary);
int32_t db_idx_double_find_secondary(account_name code, account_name scope, table_name table, const double* secondary, uint64_t* primary);
int32_t db_idx_double_lowerbound(account_name code, account_name scope, table_name table, double* secondary, uint64_t* primary);
int32_t db_idx_double_upperbound(account_name code, account_name scope, table_name table, double* secondary, uint64_t* primary);
int32_t db_idx_double_end(account_name code, account_name scope, table_name table);

int32_t db_idx_long_double_store(account_name scope, table_name table, account_name payer, uint64_t id, const long double* secondary);
void db_idx_long_double_update(int32_t iterator, account_name payer, const long double* secondary);
void db_idx_long_double_remove(int32_t iterator);
int32_t db_idx_long_double_next(int32_t iterator, uint64_t* primary);
int32_t db_idx_long_double_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx_long_double_find_primary(account_name code, account_name scope, table_name table, long double* secondary, uint64_t primary);
int32_t db_idx_long_double_find_secondary(account_name code, account_name scope, table_name table, const long double* secondary, uint64_t* primary);
int32_t db_idx_long_double_lowerbound(account_name code, account_name scope, table_name table, long double* secondary, uint64_t* primary);
int32_t db_idx_long_double_upperbound(account_name code, account_name scope, table_name table, long double* secondary, uint64_t* primary);
int32_t db_idx_long_double_end(account_name code, account_name scope, table_name table);

}
