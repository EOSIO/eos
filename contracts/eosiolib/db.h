/**
 *  @file db.h
 *  @copyright defined in eos/LICENSE
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
 *  EOSIO organizes data according to the following broad structure:
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
 *    - long double key
 *  @{
 */

/**
  *
  *  Store a record in a primary 64-bit integer index table
  *
  *  @brief Store a record in a primary 64-bit integer index table
  *  @param scope - The scope where the table resides (implied to be within the code of the current receiver)
  *  @param table - The table name
  *  @param payer - The account that pays for the storage costs
  *  @param id - ID of the entry
  *  @param data - Record to store
  *  @param len - Size of data
  *  @pre `data` is a valid pointer to a range of memory at least `len` bytes long
  *  @pre `*((uint64_t*)data)` stores the primary key
  *  @return iterator to the newly created table row
  *  @post a new entry is created in the table
  */
int32_t db_store_i64(account_name scope, table_name table, account_name payer, uint64_t id,  const void* data, uint32_t len);

/**
  *
  *  Update a record in a primary 64-bit integer index table
  *
  *  @brief Update a record in a primary 64-bit integer index table
  *  @param iterator - Iterator to the table row containing the record to update
  *  @param payer - The account that pays for the storage costs (use 0 to continue using current payer)
  *  @param data - New updated record
  *  @param len - Size of data
  *  @pre `data` is a valid pointer to a range of memory at least `len` bytes long
  *  @pre `*((uint64_t*)data)` stores the primary key
  *  @pre `iterator` points to an existing table row in the table
  *  @post the record contained in the table row pointed to by `iterator` is replaced with the new updated record
  */
void db_update_i64(int32_t iterator, account_name payer, const void* data, uint32_t len);

/**
  *
  *  Remove a record from a primary 64-bit integer index table
  *
  *  @brief Remove a record from a primary 64-bit integer index table
  *  @param iterator - Iterator to the table row to remove
  *  @pre `iterator` points to an existing table row in the table
  *  @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer
  *
  *  Example:
  *
  *  @code
  *  int32_t itr = db_find_i64(receiver, receiver, table1, N(alice));
  *  eosio_assert(itr >= 0, "Alice cannot be removed since she was already not found in the table");
  *  db_remove_i64(itr);
  *  @endcode
  */
void db_remove_i64(int32_t iterator);

/**
  *
  *  Get a record in a primary 64-bit integer index table
  *
  *  @brief Get a record in a primary 64-bit integer index table
  *  @param iterator - The iterator to the table row containing the record to retrieve
  *  @param data - Pointer to the buffer which will be filled with the retrieved record
  *  @param len - Size of the buffer
  *  @return size of the data copied into the buffer if `len > 0`, or size of the retrieved record if `len == 0`.
  *  @pre `iterator` points to an existing table row in the table
  *  @pre `data` is a valid pointer to a range of memory at least `len` bytes long
  *  @post `data` will be filled with the retrieved record (truncated to the first `len` bytes if necessary)
  *
  *  Example:
  *
  *  @code
  *  char value[50];
  *  auto len = db_get_i64(itr, value, 0);
  *  eosio_assert(len <= 50, "buffer to small to store retrieved record");
  *  db_get_i64(itr, value, len);
  *  @endcode
  */
int32_t db_get_i64(int32_t iterator, const void* data, uint32_t len);

/**
  *
  *  Find the table row following the referenced table row in a primary 64-bit integer index table
  *
  *  @brief Find the table row following the referenced table row in a primary 64-bit integer index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row
  *  @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table)
  *  @pre `iterator` points to an existing table row in the table
  *  @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched
  *
  *  Example:
  *
  *  @code
  *  int32_t charlie_itr = db_find_i64(receiver, receiver, table1, N(charlie));
  *  // expect nothing after charlie
  *  uint64_t prim = 0
  *  int32_t  end_itr = db_next_i64(charlie_itr, &prim);
  *  eosio_assert(end_itr < -1, "Charlie was not the last entry in the table");
  *  @endcode
  */
int32_t db_next_i64(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find the table row preceding the referenced table row in a primary 64-bit integer index table
  *
  *  @brief Find the table row preceding the referenced table row in a primary 64-bit integer index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row
  *  @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table)
  *  @pre `iterator` points to an existing table row in the table or it is the end iterator of the table
  *  @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched
  *
  *  Example:
  *
  *  @code
  *  uint64_t prim = 0;
  *  int32_t  itr_prev = db_previous_i64(itr, &prim);
  *  @endcode
  */
int32_t db_previous_i64(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find a table row in a primary 64-bit integer index table by primary key
  *
  *  @brief Find a table row in a primary 64-bit integer index table by primary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param id - The primary key of the table row to look up
  *  @return iterator to the table row with a primary key equal to `id` or the end iterator of the table if the table row could not be found
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
  *  Find the table row in a primary 64-bit integer index table that matches the lowerbound condition for a given primary key
  *  The table row that matches the lowerbound condition is the first table row in the table with the lowest primary key that is >= the given key
  *
  *  @brief Find the table row in a primary 64-bit integer index table that matches the lowerbound condition for a given primary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param id - The primary key used to determine the lowerbound
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_lowerbound_i64(account_name code, account_name scope, table_name table, uint64_t id);

/**
  *
  *  Find the table row in a primary 64-bit integer index table that matches the upperbound condition for a given primary key
  *  The table row that matches the upperbound condition is the first table row in the table with the lowest primary key that is > the given key
  *
  *  @brief Find the table row in a primary 64-bit integer index table that matches the upperbound condition for a given primary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param id - The primary key used to determine the upperbound
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_upperbound_i64(account_name code, account_name scope, table_name table, uint64_t id);

/**
  *
  *  Get an iterator representing just-past-the-end of the last table row of a primary 64-bit integer index table
  *
  *  @brief Get an iterator representing just-past-the-end of the last table row of a primary 64-bit integer index table
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @return end iterator of the table
  */
int32_t db_end_i64(account_name code, account_name scope, table_name table);

/**
  *
  *  Store an association of a 64-bit integer secondary key to a primary key in a secondary 64-bit integer index table
  *
  *  @brief Store an association of a 64-bit integer secondary key to a primary key in a secondary 64-bit integer index table
  *  @param scope - The scope where the table resides (implied to be within the code of the current receiver)
  *  @param table - The table name
  *  @param payer - The account that pays for the storage costs
  *  @param id - The primary key to which to associate the secondary key
  *  @param secondary - Pointer to the secondary key
  *  @return iterator to the newly created table row
  *  @post new secondary key association between primary key `id` and secondary key `*secondary` is created in the secondary 64-bit integer index table
  */
int32_t db_idx64_store(account_name scope, table_name table, account_name payer, uint64_t id, const uint64_t* secondary);

/**
  *
  *  Update an association for a 64-bit integer secondary key to a primary key in a secondary 64-bit integer index table
  *
  *  @brief Update an association for a 64-bit integer secondary key to a primary key in a secondary 64-bit integer index table
  *  @param iterator - The iterator to the table row containing the secondary key association to update
  *  @param payer - The account that pays for the storage costs (use 0 to continue using current payer)
  *  @param secondary - Pointer to the **new** secondary key that will replace the existing one of the association
  *  @pre `iterator` points to an existing table row in the table
  *  @post the secondary key of the table row pointed to by `iterator` is replaced by `*secondary`
  */
void db_idx64_update(int32_t iterator, account_name payer, const uint64_t* secondary);

/**
  *
  *  Remove a table row from a secondary 64-bit integer index table
  *
  *  @brief Remove a table row from a secondary 64-bit integer index table
  *  @param iterator - Iterator to the table row to remove
  *  @pre `iterator` points to an existing table row in the table
  *  @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer
  */
void db_idx64_remove(int32_t iterator);

/**
  *
  *  Find the table row following the referenced table row in a secondary 64-bit integer index table
  *
  *  @brief Find the table row following the referenced table row in a secondary 64-bit integer index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row
  *  @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table)
  *  @pre `iterator` points to an existing table row in the table
  *  @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx64_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find the table row preceding the referenced table row in a secondary 64-bit integer index table
  *
  *  @brief Find the table row preceding the referenced table row in a secondary 64-bit integer index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row
  *  @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table)
  *  @pre `iterator` points to an existing table row in the table or it is the end iterator of the table
  *  @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx64_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find a table row in a secondary 64-bit integer index table by primary key
  *
  *  @brief Find a table row in a secondary 64-bit integer index table by primary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to a `uint64_t` variable which will have its value set to the secondary key of the found table row
  *  @param primary - The primary key of the table row to look up
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @return iterator to the table row with a primary key equal to `id` or the end iterator of the table if the table row could not be found
  */
int32_t db_idx64_find_primary(account_name code, account_name scope, table_name table, uint64_t* secondary, uint64_t primary);

/**
  *
  *  Find a table row in a secondary 64-bit integer index table by secondary key
  *
  *  @brief Find a table row in a secondary 64-bit integer index table by secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key used to lookup the table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the first table row with a secondary key equal to `*secondary` or the end iterator of the table if the table row could not be found
  */
int32_t db_idx64_find_secondary(account_name code, account_name scope, table_name table, const uint64_t* secondary, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary 64-bit integer index table that matches the lowerbound condition for a given secondary key
  *  The table row that matches the lowerbound condition is the first table row in the table with the lowest secondary key that is >= the given key
  *
  *  @brief Find the table row in a secondary 64-bit integer index table that matches the lowerbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key first used to determine the lowerbound and which is then replaced with the secondary key of the found table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx64_lowerbound(account_name code, account_name scope, table_name table, uint64_t* secondary, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary 64-bit integer index table that matches the upperbound condition for a given secondary key
  *  The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key
  *
  *  @brief Find the table row in a secondary 64-bit integer index table that matches the upperbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key first used to determine the upperbound and which is then replaced with the secondary key of the found table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx64_upperbound(account_name code, account_name scope, table_name table, uint64_t* secondary, uint64_t* primary);

/**
  *
  *  Get an end iterator representing just-past-the-end of the last table row of a secondary 64-bit integer index table
  *
  *  @brief Get an end iterator representing just-past-the-end of the last table row of a secondary 64-bit integer index table
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @return end iterator of the table
  */
int32_t db_idx64_end(account_name code, account_name scope, table_name table);



/**
  *
  *  Store an association of a 128-bit integer secondary key to a primary key in a secondary 128-bit integer index table
  *
  *  @brief Store an association of a 128-bit integer secondary key to a primary key in a secondary 128-bit integer index table
  *  @param scope - The scope where the table resides (implied to be within the code of the current receiver)
  *  @param table - The table name
  *  @param payer - The account that pays for the storage costs
  *  @param id - The primary key to which to associate the secondary key
  *  @param secondary - Pointer to the secondary key
  *  @return iterator to the newly created table row
  *  @post new secondary key association between primary key `id` and secondary key `*secondary` is created in the secondary 128-bit integer index table
  */
int32_t db_idx128_store(account_name scope, table_name table, account_name payer, uint64_t id, const uint128_t* secondary);

/**
  *
  *  Update an association for a 128-bit integer secondary key to a primary key in a secondary 128-bit integer index table
  *
  *  @brief Update an association for a 128-bit integer secondary key to a primary key in a secondary 128-bit integer index table
  *  @param iterator - The iterator to the table row containing the secondary key association to update
  *  @param payer - The account that pays for the storage costs (use 0 to continue using current payer)
  *  @param secondary - Pointer to the **new** secondary key that will replace the existing one of the association
  *  @pre `iterator` points to an existing table row in the table
  *  @post the secondary key of the table row pointed to by `iterator` is replaced by `*secondary`
  */
void db_idx128_update(int32_t iterator, account_name payer, const uint128_t* secondary);

/**
  *
  *  Remove a table row from a secondary 128-bit integer index table
  *
  *  @brief Remove a table row from a secondary 128-bit integer index table
  *  @param iterator - Iterator to the table row to remove
  *  @pre `iterator` points to an existing table row in the table
  *  @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer
  */
void db_idx128_remove(int32_t iterator);

/**
  *
  *  Find the table row following the referenced table row in a secondary 128-bit integer index table
  *
  *  @brief Find the table row following the referenced table row in a secondary 128-bit integer index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row
  *  @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table)
  *  @pre `iterator` points to an existing table row in the table
  *  @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx128_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find the table row preceding the referenced table row in a secondary 128-bit integer index table
  *
  *  @brief Find the table row preceding the referenced table row in a secondary 128-bit integer index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row
  *  @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table)
  *  @pre `iterator` points to an existing table row in the table or it is the end iterator of the table
  *  @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx128_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find a table row in a secondary 128-bit integer index table by primary key
  *
  *  @brief Find a table row in a secondary 128-bit integer index table by primary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to a `uint128_t` variable which will have its value set to the secondary key of the found table row
  *  @param primary - The primary key of the table row to look up
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @return iterator to the table row with a primary key equal to `id` or the end iterator of the table if the table row could not be found
  */
int32_t db_idx128_find_primary(account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t primary);

/**
  *
  *  Find a table row in a secondary 128-bit integer index table by secondary key
  *
  *  @brief Find a table row in a secondary 128-bit integer index table by secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key used to lookup the table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the first table row with a secondary key equal to `*secondary` or the end iterator of the table if the table row could not be found
  */
int32_t db_idx128_find_secondary(account_name code, account_name scope, table_name table, const uint128_t* secondary, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary 128-bit integer index table that matches the lowerbound condition for a given secondary key
  *  The table row that matches the lowerbound condition is the first table row in the table with the lowest secondary key that is >= the given key
  *
  *  @brief Find the table row in a secondary 128-bit integer index table that matches the lowerbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key first used to determine the lowerbound and which is then replaced with the secondary key of the found table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx128_lowerbound(account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary 128-bit integer index table that matches the upperbound condition for a given secondary key
  *  The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key
  *
  *  @brief Find the table row in a secondary 128-bit integer index table that matches the upperbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key first used to determine the upperbound and which is then replaced with the secondary key of the found table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx128_upperbound(account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t* primary);

/**
  *
  *  Get an end iterator representing just-past-the-end of the last table row of a secondary 128-bit integer index table
  *
  *  @brief Get an end iterator representing just-past-the-end of the last table row of a secondary 128-bit integer index table
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @return end iterator of the table
  */
int32_t db_idx128_end(account_name code, account_name scope, table_name table);

/**
  *
  *  Store an association of a 256-bit secondary key to a primary key in a secondary 256-bit index table
  *
  *  @brief Store an association of a 256-bit secondary key to a primary key in a secondary 256-bit index table
  *  @param scope - The scope where the table resides (implied to be within the code of the current receiver)
  *  @param table - The table name
  *  @param payer - The account that pays for the storage costs
  *  @param id - The primary key to which to associate the secondary key
  *  @param data - Pointer to the secondary key data stored as an array of 2 `uint128_t` integers
  *  @param data_len - Must be set to 2
  *  @return iterator to the newly created table row
  *  @post new secondary key association between primary key `id` and the specified secondary key is created in the secondary 256-bit index table
  */
int32_t db_idx256_store(account_name scope, table_name table, account_name payer, uint64_t id, const uint128_t* data, uint32_t data_len );

/**
  *
  *  Update an association for a 256-bit secondary key to a primary key in a secondary 256-bit index table
  *
  *  @brief Update an association for a 256-bit secondary key to a primary key in a secondary 256-bit index table
  *  @param iterator - The iterator to the table row containing the secondary key association to update
  *  @param payer - The account that pays for the storage costs (use 0 to continue using current payer)
  *  @param data - Pointer to the **new** secondary key data (which is stored as an array of 2 `uint128_t` integers) that will replace the existing one of the association
  *  @param data_len - Must be set to 2
  *  @pre `iterator` points to an existing table row in the table
  *  @post the secondary key of the table row pointed to by `iterator` is replaced by the specified secondary key
  */
void db_idx256_update(int32_t iterator, account_name payer, const uint128_t* data, uint32_t data_len);

/**
  *
  *  Remove a table row from a secondary 256-bit index table
  *
  *  @brief Remove a table row from a secondary 256-bit index table
  *  @param iterator - Iterator to the table row to remove
  *  @pre `iterator` points to an existing table row in the table
  *  @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer
  */
void db_idx256_remove(int32_t iterator);

/**
  *
  *  Find the table row following the referenced table row in a secondary 256-bit index table
  *
  *  @brief Find the table row following the referenced table row in a secondary 256-bit index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row
  *  @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table)
  *  @pre `iterator` points to an existing table row in the table
  *  @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx256_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find the table row preceding the referenced table row in a secondary 256-bit index table
  *
  *  @brief Find the table row preceding the referenced table row in a secondary 256-bit index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row
  *  @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table)
  *  @pre `iterator` points to an existing table row in the table or it is the end iterator of the table
  *  @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx256_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find a table row in a secondary 256-bit index table by primary key
  *
  *  @brief Find a table row in a secondary 128-bit integer index table by primary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param data - Pointer to the an array of 2 `uint128_t` integers which will act as the buffer to hold the retrieved secondary key of the found table row
  *  @param data_len - Must be set to 2
  *  @param primary - The primary key of the table row to look up
  *  @post If and only if the table row is found, the buffer pointed to by `data` will be filled with the secondary key of the found table row
  *  @return iterator to the table row with a primary key equal to `id` or the end iterator of the table if the table row could not be found
  */
int32_t db_idx256_find_primary(account_name code, account_name scope, table_name table, uint128_t* data, uint32_t data_len, uint64_t primary);

/**
  *
  *  Find a table row in a secondary 256-bit index table by secondary key
  *
  *  @brief Find a table row in a secondary 256-bit index table by secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param data - Pointer to the secondary key data (which is stored as an array of 2 `uint128_t` integers) used to lookup the table row
  *  @param data_len - Must be set to 2
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the first table row with a secondary key equal to the specified secondary key or the end iterator of the table if the table row could not be found
  */
int32_t db_idx256_find_secondary(account_name code, account_name scope, table_name table, const uint128_t* data, uint32_t data_len, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary 256-bit index table that matches the lowerbound condition for a given secondary key
  *  The table row that matches the lowerbound condition is the first table row in the table with the lowest secondary key that is >= the given key (uses lexicographical ordering on the 256-bit keys)
  *
  *  @brief Find the table row in a secondary 256-bit index table that matches the lowerbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param data - Pointer to the secondary key data (which is stored as an array of 2 `uint128_t` integers) first used to determine the lowerbound and which is then replaced with the secondary key of the found table row
  *  @param data_len - Must be set to 2
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, the buffer pointed to by `data` will be filled with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx256_lowerbound(account_name code, account_name scope, table_name table, uint128_t* data, uint32_t data_len, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary 256-bit index table that matches the upperbound condition for a given secondary key
  *  The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key (uses lexicographical ordering on the 256-bit keys)
  *
  *  @brief Find the table row in a secondary 256-bit index table that matches the upperbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param data - Pointer to the secondary key data (which is stored as an array of 2 `uint128_t` integers) first used to determine the upperbound and which is then replaced with the secondary key of the found table row
  *  @param data_len - Must be set to 2
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, the buffer pointed to by `data` will be filled with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx256_upperbound(account_name code, account_name scope, table_name table, uint128_t* data, uint32_t data_len, uint64_t* primary);

/**
  *
  *  Get an end iterator representing just-past-the-end of the last table row of a secondary 256-bit index table
  *
  *  @brief Get an end iterator representing just-past-the-end of the last table row of a secondary 256-bit index table
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @return end iterator of the table
  */
int32_t db_idx256_end(account_name code, account_name scope, table_name table);

/**
  *
  *  Store an association of a double-precision floating-point secondary key to a primary key in a secondary double-precision floating-point index table
  *
  *  @brief Store an association of a double-precision floating-point secondary key to a primary key in a secondary double-precision floating-point index table
  *  @param scope - The scope where the table resides (implied to be within the code of the current receiver)
  *  @param table - The table name
  *  @param payer - The account that pays for the storage costs
  *  @param id - The primary key to which to associate the secondary key
  *  @param secondary - Pointer to the secondary key
  *  @return iterator to the newly created table row
  *  @post new secondary key association between primary key `id` and secondary key `*secondary` is created in the secondary double-precision floating-point index table
  */
int32_t db_idx_double_store(account_name scope, table_name table, account_name payer, uint64_t id, const double* secondary);

/**
  *
  *  Update an association for a double-precision floating-point secondary key to a primary key in a secondary double-precision floating-point index table
  *
  *  @brief Update an association for a double-precision floating-point secondary key to a primary key in a secondary double-precision floating-point index table
  *  @param iterator - The iterator to the table row containing the secondary key association to update
  *  @param payer - The account that pays for the storage costs (use 0 to continue using current payer)
  *  @param secondary - Pointer to the **new** secondary key that will replace the existing one of the association
  *  @pre `iterator` points to an existing table row in the table
  *  @post the secondary key of the table row pointed to by `iterator` is replaced by `*secondary`
  */
void db_idx_double_update(int32_t iterator, account_name payer, const double* secondary);

/**
  *
  *  Remove a table row from a secondary double-precision floating-point index table
  *
  *  @brief Remove a table row from a secondary double-precision floating-point index table
  *  @param iterator - Iterator to the table row to remove
  *  @pre `iterator` points to an existing table row in the table
  *  @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer
  */
void db_idx_double_remove(int32_t iterator);

/**
  *
  *  Find the table row following the referenced table row in a secondary double-precision floating-point index table
  *
  *  @brief Find the table row following the referenced table row in a secondary double-precision floating-point index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row
  *  @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table)
  *  @pre `iterator` points to an existing table row in the table
  *  @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx_double_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find the table row preceding the referenced table row in a secondary double-precision floating-point index table
  *
  *  @brief Find the table row preceding the referenced table row in a secondary double-precision floating-point index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row
  *  @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table)
  *  @pre `iterator` points to an existing table row in the table or it is the end iterator of the table
  *  @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx_double_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find a table row in a secondary double-precision floating-point index table by primary key
  *
  *  @brief Find a table row in a secondary double-precision floating-point index table by primary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to a `double` variable which will have its value set to the secondary key of the found table row
  *  @param primary - The primary key of the table row to look up
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @return iterator to the table row with a primary key equal to `id` or the end iterator of the table if the table row could not be found
  */
int32_t db_idx_double_find_primary(account_name code, account_name scope, table_name table, double* secondary, uint64_t primary);

/**
  *
  *  Find a table row in a secondary double-precision floating-point index table by secondary key
  *
  *  @brief Find a table row in a secondary double-precision floating-point index table by secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key used to lookup the table row
  *  @param primary - Pointer to a `double` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the first table row with a secondary key equal to `*secondary` or the end iterator of the table if the table row could not be found
  */
int32_t db_idx_double_find_secondary(account_name code, account_name scope, table_name table, const double* secondary, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary double-precision floating-point index table that matches the lowerbound condition for a given secondary key
  *  The table row that matches the lowerbound condition is the first table row in the table with the lowest secondary key that is >= the given key
  *
  *  @brief Find the table row in a secondary double-precision floating-point index table that matches the lowerbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key first used to determine the lowerbound and which is then replaced with the secondary key of the found table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx_double_lowerbound(account_name code, account_name scope, table_name table, double* secondary, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary double-precision floating-point index table that matches the upperbound condition for a given secondary key
  *  The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key
  *
  *  @brief Find the table row in a secondary double-precision floating-point index table that matches the upperbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key first used to determine the upperbound and which is then replaced with the secondary key of the found table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx_double_upperbound(account_name code, account_name scope, table_name table, double* secondary, uint64_t* primary);

/**
  *
  *  Get an end iterator representing just-past-the-end of the last table row of a secondary double-precision floating-point index table
  *
  *  @brief Get an end iterator representing just-past-the-end of the last table row of a secondary double-precision floating-point index table
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @return end iterator of the table
  */
int32_t db_idx_double_end(account_name code, account_name scope, table_name table);

/**
  *
  *  Store an association of a quadruple-precision floating-point secondary key to a primary key in a secondary quadruple-precision floating-point index table
  *
  *  @brief Store an association of a quadruple-precision floating-point secondary key to a primary key in a secondary quadruple-precision floating-point index table
  *  @param scope - The scope where the table resides (implied to be within the code of the current receiver)
  *  @param table - The table name
  *  @param payer - The account that pays for the storage costs
  *  @param id - The primary key to which to associate the secondary key
  *  @param secondary - Pointer to the secondary key
  *  @return iterator to the newly created table row
  *  @post new secondary key association between primary key `id` and secondary key `*secondary` is created in the secondary quadruple-precision floating-point index table
  */
int32_t db_idx_long_double_store(account_name scope, table_name table, account_name payer, uint64_t id, const long double* secondary);

/**
  *
  *  Update an association for a quadruple-precision floating-point secondary key to a primary key in a secondary quadruple-precision floating-point index table
  *
  *  @brief Update an association for a quadruple-precision floating-point secondary key to a primary key in a secondary quadruple-precision floating-point index table
  *  @param iterator - The iterator to the table row containing the secondary key association to update
  *  @param payer - The account that pays for the storage costs (use 0 to continue using current payer)
  *  @param secondary - Pointer to the **new** secondary key that will replace the existing one of the association
  *  @pre `iterator` points to an existing table row in the table
  *  @post the secondary key of the table row pointed to by `iterator` is replaced by `*secondary`
  */
void db_idx_long_double_update(int32_t iterator, account_name payer, const long double* secondary);

/**
  *
  *  Remove a table row from a secondary quadruple-precision floating-point index table
  *
  *  @brief Remove a table row from a secondary quadruple-precision floating-point index table
  *  @param iterator - Iterator to the table row to remove
  *  @pre `iterator` points to an existing table row in the table
  *  @post the table row pointed to by `iterator` is removed and the associated storage costs are refunded to the payer
  */
void db_idx_long_double_remove(int32_t iterator);

/**
  *
  *  Find the table row following the referenced table row in a secondary quadruple-precision floating-point index table
  *
  *  @brief Find the table row following the referenced table row in a secondary quadruple-precision floating-point index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the next table row
  *  @return iterator to the table row following the referenced table row (or the end iterator of the table if the referenced table row is the last one in the table)
  *  @pre `iterator` points to an existing table row in the table
  *  @post `*primary` will be replaced with the primary key of the table row following the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx_long_double_next(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find the table row preceding the referenced table row in a secondary quadruple-precision floating-point index table
  *
  *  @brief Find the table row preceding the referenced table row in a secondary quadruple-precision floating-point index table
  *  @param iterator - The iterator to the referenced table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the previous table row
  *  @return iterator to the table row preceding the referenced table row assuming one exists (it will return -1 if the referenced table row is the first one in the table)
  *  @pre `iterator` points to an existing table row in the table or it is the end iterator of the table
  *  @post `*primary` will be replaced with the primary key of the table row preceding the referenced table row if it exists, otherwise `*primary` will be left untouched
  */
int32_t db_idx_long_double_previous(int32_t iterator, uint64_t* primary);

/**
  *
  *  Find a table row in a secondary quadruple-precision floating-point index table by primary key
  *
  *  @brief Find a table row in a secondary quadruple-precision floating-point index table by primary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to a `long double` variable which will have its value set to the secondary key of the found table row
  *  @param primary - The primary key of the table row to look up
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @return iterator to the table row with a primary key equal to `id` or the end iterator of the table if the table row could not be found
  */
int32_t db_idx_long_double_find_primary(account_name code, account_name scope, table_name table, long double* secondary, uint64_t primary);

/**
  *
  *  Find a table row in a secondary quadruple-precision floating-point index table by secondary key
  *
  *  @brief Find a table row in a secondary quadruple-precision floating-point index table by secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key used to lookup the table row
  *  @param primary - Pointer to a `long double` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the first table row with a secondary key equal to `*secondary` or the end iterator of the table if the table row could not be found
  */
int32_t db_idx_long_double_find_secondary(account_name code, account_name scope, table_name table, const long double* secondary, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary quadruple-precision floating-point index table that matches the lowerbound condition for a given secondary key
  *  The table row that matches the lowerbound condition is the first table row in the table with the lowest secondary key that is >= the given key
  *
  *  @brief Find the table row in a secondary quadruple-precision floating-point index table that matches the lowerbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key first used to determine the lowerbound and which is then replaced with the secondary key of the found table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx_long_double_lowerbound(account_name code, account_name scope, table_name table, long double* secondary, uint64_t* primary);

/**
  *
  *  Find the table row in a secondary quadruple-precision floating-point index table that matches the upperbound condition for a given secondary key
  *  The table row that matches the upperbound condition is the first table row in the table with the lowest secondary key that is > the given key
  *
  *  @brief Find the table row in a secondary quadruple-precision floating-point index table that matches the upperbound condition for a given secondary key
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @param secondary - Pointer to secondary key first used to determine the upperbound and which is then replaced with the secondary key of the found table row
  *  @param primary - Pointer to a `uint64_t` variable which will have its value set to the primary key of the found table row
  *  @post If and only if the table row is found, `*secondary` will be replaced with the secondary key of the found table row
  *  @post If and only if the table row is found, `*primary` will be replaced with the primary key of the found table row
  *  @return iterator to the found table row or the end iterator of the table if the table row could not be found
  */
int32_t db_idx_long_double_upperbound(account_name code, account_name scope, table_name table, long double* secondary, uint64_t* primary);

/**
  *
  *  Get an end iterator representing just-past-the-end of the last table row of a secondary quadruple-precision floating-point index table
  *
  *  @brief Get an end iterator representing just-past-the-end of the last table row of a secondary quadruple-precision floating-point index table
  *  @param code - The name of the owner of the table
  *  @param scope - The scope where the table resides
  *  @param table - The table name
  *  @return end iterator of the table
  */
int32_t db_idx_long_double_end(account_name code, account_name scope, table_name table);

///@} databasec
}
