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
 *  @brief Defines APIs that store and retrieve data on the blockchain
 *  @ingroup contractdev
 * 
 *  @defgroup databasecpp Database C++ API
 *  @brief Defines an interface to EOSIO database
 *  @ingroup database
 *  
 *  @details
 *  EOS.IO organizes data according to the following broad structure:
 *  - **code** - the account name which has write permission
 *     - **scope** - an area where the data is stored
 *        - **table** - a name for the table that is being stored
 *           - **record** - a row in the table
 */

/**
 *  @defgroup databasec Database C API
 *  @brief Defines %C APIs for interfacing with the database.
 *  @ingroup database
 *  
 *  @details Database C API provides low level interface to EOSIO database.
 * 
 *  @section tabletypes Supported Table Types
 *  Following are the table types supported by the C API:
 *  1. Primary Table
 *    - 64-bit integer key
 *  2. Secondary Index Table
 *    - 64-bit integer key
 *    - 128-bit integer key
 *    - 256-bit integer key
 *    - double key
 *    - long double
 *  @{
 */

/**
  *
  *  Store a record in a primary 64-bit integer index table
  * 
  *  @brief Store a record in a primary 64-bit integer index table
  *  @param scope - The scope where the record will be stored
  *  @param table - The ID/name of the table within the current scope/code context
  *  @param payer - The account that is paying for this storage
  *  @param id - Id of the entry
  *  @param data - Record to store
  *  @param len - Size of data
  *  @pre len >= sizeof(data)
  *  @pre data is a valid pointer to a range of memory at least `len` bytes long
  *  @pre `*((uint64_t*)data)` stores the primary key
  *  @pre this method is being called from an apply context (not validate or precondition)
  *  @return iterator to the newly created object
  *  @post a new entry is created in the table
  */
int32_t db_store_i64(account_name scope, table_name table, account_name payer, uint64_t id,  const void* data, uint32_t len);

/**
  *
  *  Update a record inside a primary 64-bit integer index table
  *
  *  @brief Update a record inside a primary 64-bit integer index table
  *  @param iterator - Iterator of the record to update
  *  @param payer - The account that is paying for this storage
  *  @param data - New updated record
  *  @param len - Size of data
  *  @pre len >= sizeof(data)
  *  @pre `data` is a valid pointer to a range of memory at least `len` bytes long
  *  @pre `*((uint64_t*)data)` stores the primary key
  *  @pre this method is being called from an apply context (not validate or precondition)
  *  @pre `iterator` is pointing to the existing data inside the table
  *  @post the data pointed by the iterator is replaced with the new data
  */
void db_update_i64(int32_t iterator, account_name payer, const void* data, uint32_t len);

/**
  *
  *  Remove a record inside a primary 64-bit integer index table
  *
  *  @brief Remove a record inside a primary 64-bit integer index table
  *  @param iterator - The iterator pointing to the record to remove
  *  @pre `iterator` is pointing to the existing data inside the table
  *  @post the data is removed
  *
  * Example:
  *  @code
  *  int itr = db_find_i64(receiver, receiver, table1, N(alice));
  *  eosio_assert(itr >= 0, "primary_i64_general - db_find_i64");
  *  db_remove_i64(itr);
  *  itr = db_find_i64(receiver, receiver, table1, N(alice)); 
  *  @endcode
  */
void db_remove_i64(int32_t iterator);

/**
  *
  *  Get a record inside a primary 64-bit integer index table
  *
  *  @brief Get a record inside a primary 64-bit integer index table
  *  @param iterator - The iterator to the record
  *  @param data - The buffer which will be replaced with the retrieved record
  *  @param len - Size of the buffer
  *  @return size of the retrieved record
  *  @pre `iterator` is pointing to the existing data inside the table
  *  @pre `data` is a valid pointer to a range of memory at least `len` bytes long
  *  @pre `len` needs to be larger than the size of the data that is going to be retrieved
  *  @post `data` will be filled with the retrieved data
  *
  *  Example:
*
  *  @code
  *  char value[50];
  *  auto len = db_get_i64(itr, value, buffer_len);
  *  value[len] = '\0';
  *  std::string s(value);
  *  @endcode
  */
int32_t db_get_i64(int32_t iterator, const void* data, uint32_t len);

/**
  *
  *  Get the next record after the given iterator from a primary 64-bit integer index table
  *
  *  @brief Get the next record after the given iterator from a primary 64-bit integer index table
  *  @param iterator - The iterator to the record
  *  @param primary - It will be replaced with the primary key of the next record 
  *  @return iterator to the next record
  *  @pre `iterator` is pointing to the existing data inside the table
  *  @post `primary` will be replaced with the primary key of the data proceeding the data pointed by the iterator
  *
  *  Example:
*
  *  @code
  *  int charlie_itr = db_find_i64(receiver, receiver, table1, N(charlie));
  *  // nothing after charlie
  *  uint64_t prim = 0
  *  int end_itr = db_next_i64(charlie_itr, &prim);
  *  @endcode
  */
int32_t db_next_i64(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the previous record before the given iterator from a primary 64-bit integer index table
  *
  *  @brief Get the previous record before the given iterator from a primary 64-bit integer index table
  *  @param iterator - The iterator to the record
  *  @param primary - It will be replaced with the primary key of the previous record 
  *  @return iterator to the previous record
  *  @pre `iterator` is pointing to the existing data inside the table
  *  @post `primary` will be replaced with the primary key of the data preceeding the data pointed by the iterator
  *
  *  Example:
*
  *  @code
  *  uint64_t prim = 123;
  *  int itr_prev = db_previous_i64(itr, &prim);
  *  @endcode
  */
int32_t db_previous_i64(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find a record inside a primary 64-bit integer index table
  *
  *  @brief Find a record inside a primary 64-bit integer index table
  *  @param scope - The scope where the record is stored
  *  @param table - The table name where the record is stored
  *  @param id - The primary key of the record to look up
  *  @return iterator to the found record
  *
  *  Example:
*
  *  @code
  *  int itr = db_find_i64(receiver, receiver, table1, N(charlie));
  *  @endcode
  */
int32_t db_find_i64(account_name code, account_name scope, table_name table, uint64_t id);

/**
  *
  *  Find the lowerbound record given a key inside a primary 64-bit integer index table
  *  Lowerbound record is the first nearest record which primary key is <= the given key
  *
  *  @brief Find the lowerbound record given a key inside a primary 64-bit integer index table
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param id - The primary key used as a pivot to determine the lowerbound record
  *  @return iterator to the lowerbound record
  */
int32_t db_lowerbound_i64(account_name code, account_name scope, table_name table, uint64_t id);

/**
  *
  *  Find the upperbound record given a key inside a primary 64-bit integer index table
  *  Upperbound record is the first nearest record which primary key is < the given key
  *
  *  @brief Find the upperbound record given a key inside a primary 64-bit integer index table
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param id - The primary key used as a pivot to determine the upperbound record
  *  @return iterator to the upperbound record
  */
int32_t db_upperbound_i64(account_name code, account_name scope, table_name table, uint64_t id);

/**
  *
  *  Find the latest record inside a primary 64-bit integer index table
  *
  *  @brief Find the latest record inside a primary 64-bit integer index table
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @return iterator to the last record
  */
int32_t db_end_i64(account_name code, account_name scope, table_name table);

/**
  *
  *  Store a record's secondary index in a secondary 64-bit integer index table
  * 
  *  @brief Store a record's secondary index in a secondary 64-bit integer index table
  *  @param scope - The scope where the secondary index will be stored
  *  @param table - The table name where the secondary index will be stored
  *  @param payer - The account that is paying for this storage
  *  @param id - The primary key of the record which secondary index to be stored
  *  @param secondary - The pointer to the key of the secondary index to store
  *  @return iterator to the newly created secondary index
  *  @post new secondary index is created
  */
int32_t db_idx64_store(account_name scope, table_name table, account_name payer, uint64_t id, const uint64_t* secondary);

/**
  *
  *  Update a record's secondary index inside a secondary 64-bit integer index table
  * 
  *  @brief Update a record's secondary index inside a secondary 64-bit integer index table
  *  @param iterator - The iterator to the secondary index
  *  @param payer - The account that is paying for this storage
  *  @param secondary - The pointer to the **new** key of the secondary index
  *  @pre `iterator` is pointing to existing secondary index
  *  @post the seconday index pointed by the iterator is updated by the new value
  */
void db_idx64_update(int32_t iterator, account_name payer, const uint64_t* secondary);

/**
  *
  *  Remove a record's secondary index from a secondary 64-bit integer index table
  * 
  *  @brief Remove a record's secondary index from a secondary 64-bit integer index table
  *  @param iterator - The iterator to the secondary index to be removed
  *  @pre `iterator` is pointing to existing secondary index
  *  @post the secondary index pointed by the iterator is removed from the table
  */
void db_idx64_remove(int32_t iterator);

/**
  *
  *  Get the next secondary index inside a secondary 64-bit integer index table
  * 
  *  @brief Get the next secondary index inside a secondary 64-bit integer index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **next** secondary index
  *  @return iterator to the next secondary index
  *  @pre `iterator` is pointing to the existing secondary index inside the table
  *  @post `primary` will be replaced with the primary key of the secondary index proceeding the secondary index pointed by the iterator
  */
int32_t db_idx64_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the previous secondary index inside a secondary 64-bit integer index table
  * 
  *  @brief Get the previous secondary index inside a secondary 64-bit integer index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **previous** secondary index
  *  @return iterator to the previous secondary index
  *  @pre `iterator` is pointing to the existing secondary index inside the table 
  *  @post `primary` will be replaced with the primary key of the secondary index preceeding the secondary index pointed by the iterator
  */
int32_t db_idx64_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the secondary index of a record  from a secondary 64-bit integer index table given the record's primary key
  * 
  *  @brief Get  the secondary index of a record from a secondary 64-bit integer index table given the record's primary key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - It will be replaced with the secondary index key
  *  @param primary - The record's primary key
  *  @pre A correponding primary 64-bit integer index table with the given code, scope table must exist
  *  @post The secondary param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given record's primary key
  */
int32_t db_idx64_find_primary(account_name code, account_name scope, table_name table, uint64_t* secondary, uint64_t primary);

/**
  *
  *  Get the secondary index of a record from a secondary 64-bit integer index table given the secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary 64-bit integer index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the secondary index contains
  *  @pre A correponding primary 64-bit integer index table with the given code, scope table must exist
  *  @post The secondary param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given secondary index key
  */
int32_t db_idx64_find_secondary(account_name code, account_name scope, table_name table, const uint64_t* secondary, uint64_t* primary);

/**
  *
  *  Get the lowerbound secondary index from a secondary 64-bit integer index table given the secondary index key
  *  Lowerbound secondary index is the first secondary index which key is <= the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary 64-bit integer index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key which acts as lowerbound pivot point, later on it will be replaced with the lowerbound secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the lowerbound secondary index contains
  *  @pre A correponding primary 64-bit integer index table with the given code, scope table must exist
  *  @post The secondary param will contains the lowerbound secondary index key
  *  @post The primary param will contains the record that corresponds to the lowerbound secondary index
  *  @return iterator to the lowerbound secondary index
  */
int32_t db_idx64_lowerbound(account_name code, account_name scope, table_name table, uint64_t* secondary, uint64_t* primary);

/**
  *
  *  Get the upperbound secondary index from a secondary 64-bit integer index table given the secondary index key
  *  Upperbound secondary index is the first secondary index which key is < the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary 64-bit integer index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key which acts as upperbound pivot point, later on it will be replaced with the upperbound secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the upperbound secondary index contains
  *  @pre A correponding primary 64-bit integer index table with the given code, scope table must exist
  *  @post The secondary param will contains the upperbound secondary index key
  *  @post The primary param will contains the record that corresponds to the upperbound secondary index
  *  @return iterator to the upperbound secondary index
  */
int32_t db_idx64_upperbound(account_name code, account_name scope, table_name table, uint64_t* secondary, uint64_t* primary);

/**
  *
  *  Get the last secondary index from a secondary 64-bit integer index table 
  * 
  *  @brief Get the last secondary index from a secondary 64-bit integer index table 
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @return iterator to the last secondary index
  */
int32_t db_idx64_end(account_name code, account_name scope, table_name table);



/**
  *
  *  Store a record's secondary index in a secondary 128-bit integer index table
  * 
  *  @brief Store a record's secondary index in a secondary 128-bit integer index table
  *  @param scope - The scope where the secondary index will be stored
  *  @param table - The table name where the secondary index will be stored
  *  @param payer - The account that is paying for this storage
  *  @param id - The primary key of the record which secondary index to be stored
  *  @param secondary - The pointer to the key of the secondary index to store
  *  @return iterator to the newly created secondary index
  *  @post new secondary index is created
  */
int32_t db_idx128_store(account_name scope, table_name table, account_name payer, uint64_t id, const uint128_t* secondary);

/**
  *
  *  Update a record's secondary index inside a secondary 128-bit integer index table
  * 
  *  @brief Update a record's secondary index inside a secondary 128-bit integer index table
  *  @param iterator - The iterator to the secondary index
  *  @param payer - The account that is paying for this storage
  *  @param secondary - The pointer to the **new** key of the secondary index
  *  @pre `iterator` is pointing to existing secondary index
  *  @post the seconday index pointed by the iterator is updated by the new value
  */
void db_idx128_update(int32_t iterator, account_name payer, const uint128_t* secondary);

/**
  *
  *  Remove a record's secondary index from a secondary 128-bit integer index table
  * 
  *  @brief Remove a record's secondary index from a secondary 128-bit integer index table
  *  @param iterator - The iterator to the secondary index to be removed
  *  @pre `iterator` is pointing to existing secondary index
  *  @post the secondary index pointed by the iterator is removed from the table
  */
void db_idx128_remove(int32_t iterator);

/**
  *
  *  Get the next secondary index inside a secondary 128-bit integer index table
  * 
  *  @brief Get the next secondary index inside a secondary 128-bit integer index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **next** secondary index
  *  @return iterator to the next secondary index
  *  @pre `iterator` is pointing to the existing secondary index inside the table
  *  @post `primary` will be replaced with the primary key of the secondary index proceeding the secondary index pointed by the iterator
  */
int32_t db_idx128_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the previous secondary index inside a secondary 128-bit integer index table
  * 
  *  @brief Get the previous secondary index inside a secondary 128-bit integer index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **previous** secondary index
  *  @return iterator to the previous secondary index
  *  @pre `iterator` is pointing to the existing secondary index inside the table 
  *  @post `primary` will be replaced with the primary key of the secondary index preceeding the secondary index pointed by the iterator
  */
int32_t db_idx128_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the secondary index of a record  from a secondary 128-bit integer index table given the record's primary key
  * 
  *  @brief Get  the secondary index of a record from a secondary 128-bit integer index table given the record's primary key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - It will be replaced with the secondary index key
  *  @param primary - The record's primary key
  *  @pre A correponding primary 128-bit integer index table with the given code, scope table must exist
  *  @post The secondary param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given record's primary key
  */
int32_t db_idx128_find_primary(account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t primary);

/**
  *
  *  Get the secondary index of a record from a secondary 128-bit integer index table given the secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary 128-bit integer index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the secondary index contains
  *  @pre A correponding primary 128-bit integer index table with the given code, scope table must exist
  *  @post The secondary param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given secondary index key
  */
int32_t db_idx128_find_secondary(account_name code, account_name scope, table_name table, const uint128_t* secondary, uint64_t* primary);

/**
  *
  *  Get the lowerbound secondary index from a secondary 128-bit integer index table given the secondary index key
  *  Lowerbound secondary index is the first secondary index which key is <= the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary 128-bit integer index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key which acts as lowerbound pivot point, later on it will be replaced with the lowerbound secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the lowerbound secondary index contains
  *  @pre A correponding primary 128-bit integer index table with the given code, scope table must exist
  *  @post The secondary param will contains the lowerbound secondary index key
  *  @post The primary param will contains the record that corresponds to the lowerbound secondary index
  *  @return iterator to the lowerbound secondary index
  */
int32_t db_idx128_lowerbound(account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t* primary);

/**
  *
  *  Get the upperbound secondary index from a secondary 128-bit integer index table given the secondary index key
  *  Upperbound secondary index is the first secondary index which key is < the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary 128-bit integer index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key which acts as upperbound pivot point, later on it will be replaced with the upperbound secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the upperbound secondary index contains
  *  @pre A correponding primary 128-bit integer index table with the given code, scope table must exist
  *  @post The secondary param will contains the upperbound secondary index key
  *  @post The primary param will contains the record that corresponds to the upperbound secondary index
  *  @return iterator to the upperbound secondary index
  */
int32_t db_idx128_upperbound(account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t* primary);

/**
  *
  *  Get the last secondary index from a secondary 128-bit integer index table 
  * 
  *  @brief Get the last secondary index from a secondary 128-bit integer index table 
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @return iterator to the last secondary index
  */
int32_t db_idx128_end(account_name code, account_name scope, table_name table);

/**
  *
  *  Store a record's secondary index in a secondary 256-bit integer index table
  * 
  *  @brief Store a record's secondary index in a secondary 256-bit integer index table
  *  @param scope - The scope where the secondary index will be stored
  *  @param table - The table name where the secondary index will be stored
  *  @param payer - The account that is paying for this storage
  *  @param id - The primary key of the record which secondary index to be stored
  *  @param data - The pointer to the key of the secondary index to store
  *  @param data_len - Size of the key of the secondary index to store
  *  @return iterator to the newly created secondary index
  *  @pre `data` is pointing to range of memory at least `data_len` bytes long
  *  @post new secondary index is created
  */
int32_t db_idx256_store(account_name scope, table_name table, account_name payer, uint64_t id, const void* data, uint32_t data_len );

/**
  *
  *  Update a record's secondary index inside a secondary 256-bit integer index table
  * 
  *  @brief Update a record's secondary index inside a secondary 256-bit integer index table
  *  @param iterator - The iterator to the secondary index
  *  @param payer - The account that is paying for this storage
  *  @param data - The pointer to the **new** key of the secondary index
  *  @param data_len - Size of the **new** key of the secondary index to store
  *  @pre `iterator` is pointing to existing secondary index
  *  @post the seconday index pointed by the iterator is updated by the new value
  */
void db_idx256_update(int32_t iterator, account_name payer, const void* data, uint32_t data_len);

/**
  *
  *  Remove a record's secondary index from a secondary 256-bit integer index table
  * 
  *  @brief Remove a record's secondary index from a secondary 256-bit integer index table
  *  @param iterator - The iterator to the secondary index to be removed
  *  @pre `iterator` is pointing to existing secondary index
  *  @post the secondary index pointed by the iterator is removed from the table
  */
void db_idx256_remove(int32_t iterator);

/**
  *
  *  Get the next secondary index inside a secondary 256-bit integer index table
  * 
  *  @brief Get the next secondary index inside a secondary 256-bit integer index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **next** secondary index
  *  @return iterator to the next secondary index
 *  @pre `iterator` is pointing to the existing secondary index inside the table
  *  @post `primary` will be replaced with the primary key of the secondary index proceeding the secondary index pointed by the iterator
  */
int32_t db_idx256_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the previous secondary index inside a secondary 256-bit integer index table
  * 
  *  @brief Get the previous secondary index inside a secondary 256-bit integer index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **previous** secondary index
  *  @return iterator to the previous secondary index
 *  @pre `iterator` is pointing to the existing secondary index inside the table 
  *  @post `primary` will be replaced with the primary key of the secondary index preceeding the secondary index pointed by the iterator
  */
int32_t db_idx256_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the secondary index of a record  from a secondary 256-bit integer index table given the record's primary key
  * 
  *  @brief Get  the secondary index of a record from a secondary 256-bit integer index table given the record's primary key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param data - The buffer which will be replaced with the secondary index key
  *  @param data_len - The buffer size
  *  @param primary - The record's primary key
  *  @pre A correponding primary 256-bit integer index table with the given code, scope table must exist
  *  @post The data param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given record's primary key
  */
int32_t db_idx256_find_primary(account_name code, account_name scope, table_name table, void* data, uint32_t data_len, uint64_t primary);

/**
  *
  *  Get the secondary index of a record from a secondary 256-bit integer index table given the secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary 256-bit integer index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param data - The pointer to the secondary index key
  *  @param data_len - Size of the secondary index key 
  *  @param primary - It will be replaced with the primary key of the record which the secondary index contains
  *  @pre A correponding primary 256-bit integer index table with the given code, scope table must exist
  *  @post The data param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given secondary index key
  */
int32_t db_idx256_find_secondary(account_name code, account_name scope, table_name table, const void* data, uint32_t data_len, uint64_t* primary);

/**
  *
  *  Get the lowerbound secondary index from a secondary 256-bit integer index table given the secondary index key
  *  Lowerbound secondary index is the first secondary index which key is <= the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary 256-bit integer index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param data - The pointer to the secondary index key which acts as lowerbound pivot point, later on it will be replaced with the lowerbound secondary index key
  *  @param data_len - Size of the secondary index key 
  *  @param primary - It will be replaced with the primary key of the record which the lowerbound secondary index contains
  *  @pre A correponding primary 256-bit integer index table with the given code, scope table must exist
  *  @post The data param will contains the lowerbound secondary index key
  *  @post The primary param will contains the record that corresponds to the lowerbound secondary index
  *  @return iterator to the lowerbound secondary index
  */
int32_t db_idx256_lowerbound(account_name code, account_name scope, table_name table, void* data, uint32_t data_len, uint64_t* primary);

/**
  *
  *  Get the upperbound secondary index from a secondary 256-bit integer index table given the secondary index key
  *  Upperbound secondary index is the first secondary index which key is < the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary 256-bit integer index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param data - The pointer to the secondary index key which acts as lowerbound pivot point, later on it will be replaced with the lowerbound secondary index key
  *  @param data_len - Size of the secondary index key 
  *  @param primary - It will be replaced with the primary key of the record which the upperbound secondary index contains
  *  @pre A correponding primary 256-bit integer index table with the given code, scope table must exist
  *  @post The data param will contains the upperbound secondary index key
  *  @post The primary param will contains the record that corresponds to the upperbound secondary index
  *  @return iterator to the upperbound secondary index
  */
int32_t db_idx256_upperbound(account_name code, account_name scope, table_name table, void* data, uint32_t data_len, uint64_t* primary);

/**
  *
  *  Get the last secondary index from a secondary 256-bit integer index table 
  * 
  *  @brief Get the last secondary index from a secondary 256-bit integer index table 
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @return iterator to the last secondary index
  */
int32_t db_idx256_end(account_name code, account_name scope, table_name table);

/**
  *
  *  Store a record's secondary index in a secondary double index table
  * 
  *  @brief Store a record's secondary index in a secondary double index table
  *  @param scope - The scope where the secondary index will be stored
  *  @param table - The table name where the secondary index will be stored
  *  @param payer - The account that is paying for this storage
  *  @param id - The primary key of the record which secondary index to be stored
  *  @param secondary - The pointer to the key of the secondary index to store
  *  @return iterator to the newly created secondary index
  */
int32_t db_idx_double_store(account_name scope, table_name table, account_name payer, uint64_t id, const double* secondary);

/**
  *
  *  Update a record's secondary index inside a secondary double index table
  * 
  *  @brief Update a record's secondary index inside a secondary double index table
  *  @param iterator - The iterator to the secondary index
  *  @param payer - The account that is paying for this storage
  *  @param secondary - The pointer to the **new** key of the secondary index
  */
void db_idx_double_update(int32_t iterator, account_name payer, const double* secondary);

/**
  *
  *  Remove a record's secondary index from a secondary double index table
  * 
  *  @brief Remove a record's secondary index from a secondary double index table
  *  @param iterator - The iterator to the secondary index to be removed
  */
void db_idx_double_remove(int32_t iterator);

/**
  *
  *  Get the next secondary index inside a secondary double index table
  * 
  *  @brief Get the next secondary index inside a secondary double index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **next** secondary index
  *  @return iterator to the next secondary index
  */
int32_t db_idx_double_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the previous secondary index inside a secondary double index table
  * 
  *  @brief Get the previous secondary index inside a secondary double index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **previous** secondary index
  *  @return iterator to the previous secondary index
  */
int32_t db_idx_double_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the secondary index of a record  from a secondary double index table given the record's primary key
  * 
  *  @brief Get  the secondary index of a record from a secondary double index table given the record's primary key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - It will be replaced with the secondary index key
  *  @param primary - The record's primary key
  *  @pre A correponding primary double index table with the given code, scope table must exist
  *  @post The secondary param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given record's primary key
  */
int32_t db_idx_double_find_primary(account_name code, account_name scope, table_name table, double* secondary, uint64_t primary);

/**
  *
  *  Get the secondary index of a record from a secondary double index table given the secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary double index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the secondary index contains
  *  @pre A correponding primary double index table with the given code, scope table must exist
  *  @post The secondary param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given secondary index key
  */
int32_t db_idx_double_find_secondary(account_name code, account_name scope, table_name table, const double* secondary, uint64_t* primary);

/**
  *
  *  Get the lowerbound secondary index from a secondary double index table given the secondary index key
  *  Lowerbound secondary index is the first secondary index which key is <= the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary double index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key which acts as lowerbound pivot point, later on it will be replaced with the lowerbound secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the lowerbound secondary index contains
  *  @pre A correponding primary double index table with the given code, scope table must exist
  *  @post The secondary param will contains the lowerbound secondary index key
  *  @post The primary param will contains the record that corresponds to the lowerbound secondary index
  *  @return iterator to the lowerbound secondary index
  */
int32_t db_idx_double_lowerbound(account_name code, account_name scope, table_name table, double* secondary, uint64_t* primary);

/**
  *
  *  Get the upperbound secondary index from a secondary double index table given the secondary index key
  *  Upperbound secondary index is the first secondary index which key is < the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary double index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key which acts as upperbound pivot point, later on it will be replaced with the upperbound secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the upperbound secondary index contains
  *  @pre A correponding primary double index table with the given code, scope table must exist
  *  @post The secondary param will contains the upperbound secondary index key
  *  @post The primary param will contains the record that corresponds to the upperbound secondary index
  *  @return iterator to the upperbound secondary index
  */
int32_t db_idx_double_upperbound(account_name code, account_name scope, table_name table, double* secondary, uint64_t* primary);

/**
  *
  *  Get the last secondary index from a secondary double index table 
  * 
  *  @brief Get the last secondary index from a secondary double index table 
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @return iterator to the last secondary index
  */
int32_t db_idx_double_end(account_name code, account_name scope, table_name table);

/**
  *
  *  Store a record's secondary index in a secondary long double index table
  * 
  *  @brief Store a record's secondary index in a secondary long double index table
  *  @param scope - The scope where the secondary index will be stored
  *  @param table - The table name where the secondary index will be stored
  *  @param payer - The account that is paying for this storage
  *  @param id - The primary key of the record which secondary index to be stored
  *  @param secondary - The pointer to the key of the secondary index to store
  *  @return iterator to the newly created secondary index
  */
int32_t db_idx_long_double_store(account_name scope, table_name table, account_name payer, uint64_t id, const long double* secondary);

/**
  *
  *  Update a record's secondary index inside a secondary long double index table
  * 
  *  @brief Update a record's secondary index inside a secondary long double index table
  *  @param iterator - The iterator to the secondary index
  *  @param payer - The account that is paying for this storage
  *  @param secondary - The pointer to the **new** key of the secondary index
  */
void db_idx_long_double_update(int32_t iterator, account_name payer, const long double* secondary);

/**
  *
  *  Remove a record's secondary index from a secondary long double index table
  * 
  *  @brief Remove a record's secondary index from a secondary long double index table
  *  @param iterator - The iterator to the secondary index to be removed
  */
void db_idx_long_double_remove(int32_t iterator);

/**
  *
  *  Get the next secondary index inside a secondary long double index table
  * 
  *  @brief Get the next secondary index inside a secondary long double index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **next** secondary index
  *  @return iterator to the next secondary index
  */
int32_t db_idx_long_double_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the previous secondary index inside a secondary long double index table
  * 
  *  @brief Get the previous secondary index inside a secondary long double index table
  *  @param iterator - The iterator to the secondary index
  *  @param primary - It will be replaced with the primary key of the record which is stored in the **previous** secondary index
  *  @return iterator to the previous secondary index
  */
int32_t db_idx_long_double_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Get the secondary index of a record  from a secondary long double index table given the record's primary key
  * 
  *  @brief Get  the secondary index of a record from a secondary long double index table given the record's primary key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - It will be replaced with the secondary index key
  *  @param primary - The record's primary key
  *  @pre A correponding primary long double index table with the given code, scope table must exist
  *  @post The secondary param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given record's primary key
  */
int32_t db_idx_long_double_find_primary(account_name code, account_name scope, table_name table, long double* secondary, uint64_t primary);

/**
  *
  *  Get the secondary index of a record from a secondary long double index table given the secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary long double index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the secondary index contains
  *  @pre A correponding primary long double index table with the given code, scope table must exist
  *  @post The secondary param will contains the appropriate secondary index key
  *  @post The primary param will contains the record that corresponds to the appropriate secondary index
  *  @return iterator to the secondary index which contains the given secondary index key
  */
int32_t db_idx_long_double_find_secondary(account_name code, account_name scope, table_name table, const long double* secondary, uint64_t* primary);

/**
  *
  *  Get the lowerbound secondary index from a secondary long double index table given the secondary index key
  *  Lowerbound secondary index is the first secondary index which key is <= the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary long double index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key which acts as lowerbound pivot point, later on it will be replaced with the lowerbound secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the lowerbound secondary index contains
  *  @pre A correponding primary long double index table with the given code, scope table must exist
  *  @post The secondary param will contains the lowerbound secondary index key
  *  @post The primary param will contains the record that corresponds to the lowerbound secondary index
  *  @return iterator to the lowerbound secondary index
  */
int32_t db_idx_long_double_lowerbound(account_name code, account_name scope, table_name table, long double* secondary, uint64_t* primary);

/**
  *
  *  Get the upperbound secondary index from a secondary long double index table given the secondary index key
  *  Upperbound secondary index is the first secondary index which key is < the given secondary index key
  * 
  *  @brief Get the secondary index of a record from a secondary long double index table given the secondary index key
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @param secondary - The pointer to the secondary index key which acts as upperbound pivot point, later on it will be replaced with the upperbound secondary index key
  *  @param primary - It will be replaced with the primary key of the record which the upperbound secondary index contains
  *  @pre A correponding primary long double index table with the given code, scope table must exist
  *  @post The secondary param will contains the upperbound secondary index key
  *  @post The primary param will contains the record that corresponds to the upperbound secondary index
  *  @return iterator to the upperbound secondary index
  */
int32_t db_idx_long_double_upperbound(account_name code, account_name scope, table_name table, long double* secondary, uint64_t* primary);

/**
  *
  *  Get the last secondary index from a secondary long double index table 
  * 
  *  @brief Get the last secondary index from a secondary long double index table 
  *  @param code - The owner of the secondary index table
  *  @param scope - The scope where the secondary index resides
  *  @param table - The table where the secondary index resides
  *  @return iterator to the last secondary index
  */
int32_t db_idx_long_double_end(account_name code, account_name scope, table_name table);

///@} databasec
}
