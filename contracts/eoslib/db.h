/**
 *  @file db.h
 *  @brief Defines C API for interfacing with blockchain database
 */
#pragma once

#include <eoslib/types.h>
extern "C" {
/**
 *  @defgroup database Database API
 *  @brief APIs that store and retreive data on the blockchain
 *  @ingroup contractdev
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
 * @return 1 if the record was updated, 0 if no record with key was found
 */
int32_t update_i64( AccountName scope, TableName table, const void* data, uint32_t datalen );

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
int32_t front_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );
int32_t back_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );
int32_t next_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );
int32_t previous_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );
int32_t lower_bound_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );
int32_t upper_bound_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );

/**
 *  @param data - must point to at lest 8 bytes containing primary key
 *
 *  @return 1 if a record was removed, and 0 if no record with key was found
 */
int32_t remove_i64( AccountName scope, TableName table, void* data );

///@} db_i64



/**
 *  @defgroup dbi128i128  Dual 128 bit Index Table
 *  @brief Interface to a database table with 128 bit primary and secondary keys and arbitary binary data value.
 *  @ingroup databaseC 
 *
 *  @param scope - the account where table data will be found
 *  @param code  - the code which owns the table
 *  @param table - the name of the table where record is stored
 *  @param data  - a pointer to memory that is at least 32 bytes long 
 *  @param len   - the length of data, must be greater than or equal to 32 bytes
 *
 *  @return the total number of bytes read or -1 for "not found" or "end" where bytes 
 *  read includes 32 bytes of the key  
 *
 *  These methods assume a database table with records of the form:
 *
 *  ```
 *     struct Record {
 *        uint128  primary;
 *        uint128  secondary;
 *        ... arbitrary data ...
 *     };
 *
 *  ```
 *
 *  You can iterate over these indicies with primary index sorting records by { primary, secondary } and
 *  the secondary index sorting records by { secondary, primary }.  This means that duplicates of the primary or
 *  secondary values are allowed so long as there are no duplicates of the combination {primary, secondary}.
 *
 *  @see Table class in C++ API
 * 
 *  @{
 */

int32_t load_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t front_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t back_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t next_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t previous_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t upper_bound_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t lower_bound_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

int32_t load_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t front_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t back_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t next_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t previous_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t upper_bound_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t lower_bound_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );


/**
 * @param data - must point to at lest 32 bytes containing {primary,secondary}
 *
 * @return 1 if a record was removed, and 0 if no record with key was found
 */
int32_t remove_i128i128( AccountName scope, TableName table, const void* data );
/**
 * @return 1 if a new record was created, 0 if an existing record was updated
 */
int32_t store_i128i128( AccountName scope, TableName table, const void* data, uint32_t len );

/**
 * @return 1 if the record was updated, 0 if no record with key was found
 */
int32_t update_i128i128( AccountName scope, TableName table, const void* data, uint32_t len );

///@}  dbi128i128

/**
 *  @defgroup dbi64i64i64 Triple 64 bit Index Table
 *  @brief Interface to a database table with 64 bit primary, secondary and tertiary keys and arbitary binary data value.
 *  @ingroup databaseC 
 *
 *  @param scope - the account where table data will be found
 *  @param code  - the code which owns the table
 *  @param table - the name of the table where record is stored
 *  @param data  - a pointer to memory that is at least 32 bytes long 
 *  @param len   - the length of data, must be greater than or equal to 32 bytes
 *
 *  @return the total number of bytes read or -1 for "not found" or "end" where bytes 
 *  read includes 24 bytes of the key  
 *
 *  These methods assume a database table with records of the form:
 *
 *  ```
 *     struct Record {
 *        uint64  primary;
 *        uint64  secondary;
 *        uint64  tertiary;
 *        ... arbitrary data ...
 *     };
 *
 *  ```
 *
 *  You can iterate over these indicies with primary index sorting records by { primary, secondary, tertiary },
 *  the secondary index sorting records by { secondary, tertiary } and the tertiary index sorting records by
 *  { tertiary }.
 *
 *  @see Table class in C++ API
 * 
 *  @{
 */

int32_t load_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t front_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t back_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t next_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t previous_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t upper_bound_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t lower_bound_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

int32_t load_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t front_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t back_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t next_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t previous_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t upper_bound_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t lower_bound_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

int32_t load_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t front_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t back_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t next_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t previous_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t upper_bound_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t lower_bound_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 * @param data - must point to at lest 24 bytes containing {primary,secondary,tertiary}
 *
 * @return 1 if a record was removed, and 0 if no record with key was found
 */
int32_t remove_i64i64i64( AccountName scope, TableName table, const void* data );
/**
 * @return 1 if a new record was created, 0 if an existing record was updated
 */
int32_t store_i64i64i64( AccountName scope, TableName table, const void* data, uint32_t len );

/**
 * @return 1 if the record was updated, 0 if no record with key was found
 */
int32_t update_i64i64i64( AccountName scope, TableName table, const void* data, uint32_t len );

///@}  dbi64i64i64
}
