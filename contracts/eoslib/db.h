/**
 *  @file db.h
 *  @brief Defines C API for interfacing with blockchain database
 */
#pragma once

#include <eoslib/types.h>
/**
 *  @defgroup database EOS.IO Database API
 *  @brief APIs that store and retreive data on the blockchain
 *
 *  EOS.IO organizes data according to the following broad structure:
 *
 *  - **scope** - an account where the data is stored
 *     - **code** - the account name which has write permission 
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
 *  @section tabletypes Table Types
 *  There are several supported table types identified by the number and
 *  size of the index.  
 *
 *  1. @ref dbi64 
 *  2. @ref dbi128i128
 *
 *  The database APIs assume that the first bytes of each record represent
 *  the primary and/or secondary keys followed by an arbitrary amount of data.
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
 *  @defgroup databaseCpp Database C++ API
 *  @brief C++ APIs for interfacing with the database.
 *  @ingroup database
 */

/**
 * @defgroup dbi64  Single 64 bit Index Table
 * @brief These methods interface with a simple table with 64 bit unique primary key and arbitrary binary data value.
 * @ingroup databaseC
 *
 * @see Table class in C++ API
 *
 * @{
 */

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the ID/name of the table within the current scope/code context to modify
 *
 * @return 1 if a new record was created, 0 if an existing record was updated
 *
 * @pre datalen >= sizeof(uint64_t)
 * @pre data is a valid pointer to a range of memory at least datalen bytes long
 * @pre *((uint64_t*)data) stores the primary key
 * @pre scope is declared by the current transaction
 * @pre this method is being called from an apply context (not validate or precondition)
 *
 * @post a record is either created or updated with the given scope and table.
 *
 * @throw if called with an invalid precondition execution will be aborted
 *
 */
int32_t store_i64( AccountName scope, TableName table, const void* data, uint32_t datalen );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the stored record, should be initialized with the key to get
 *  @param datalen - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if key was not found
 */
int32_t load_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );

/**
 *  @return 1 if a record was removed, and 0 if no record with key was found
 */
int32_t remove_i64( AccountName scope, TableName table, uint64_t key );

///@} db_i64



/**
 *  @defgroup dbi128i128  Dual 128 bit Index Table
 *  @brief Interface to a database table with 128 bit primary and secondary keys and arbitary binary data value.
 *  @ingroup databaseC 
 *
 *  These methods expect data to point to a record that is at least 2*sizeof(uint128_t) where the leading
 *  32 bytes are the primary and secondary keys.  These keys will be interpreted and sorted as unsigned
 *  128 bit integers.  
 *
 *  @return the total number of bytes read or -1 for "not found" or "end" where bytes 
 *  read includes 32 bytes of the key  
 *
 *  @see Table class in C++ API
 * 
 *  @{
 */

int32_t front_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t back_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t next_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t previous_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

int32_t load_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

int32_t upper_bound_primary_i128i128( AccountName scope, AccountName code, TableName table, 
                                      const void* key, void* data, uint32_t len );
int32_t lower_bound_primary_i128i128( AccountName scope, AccountName code, TableName table, 
                                     const void* key, void* data, uint32_t len );

int32_t front_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t back_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t next_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t previous_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );


int32_t upper_bound_secondary_i128i128( AccountName scope, AccountName code, TableName table, 
                                      const void* key, void* data, uint32_t len );
int32_t lower_bound_secondary_i128i128( AccountName scope, AccountName code, TableName table, 
                                     const void* key, void* data, uint32_t len );

int32_t load_secondary_i128i128( AccountName scope, AccountName code, TableName table, const void* secondary, void* data, uint32_t len );
///@}

/// data must point to 2*sizeof(uint128) containing primary and secondary key
bool remove_i128i128( AccountName scope, TableName table, const void* data );
/// data must point to at least 2*sizeof(uint128) containing primary and secondary key
bool store_i128i128( AccountName scope, TableName table, const void* data, uint32_t len );

///@ }

