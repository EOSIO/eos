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
 * Example
 * @code
 * #pragma pack(push, 1)
 * struct TestModel {
 *    AccountName   name;
 *    unsigned char age;
 *    uint64_t      phone;
 * };
 *
 * TestModel alice{ N(alice), 20, 4234622};
 * TestModel bob  { N(bob),   15, 11932435};
 * TestModel carol{ N(carol), 30, 545342453};
 * TestModel dave { N(dave),  46, 6535354};
 *
 * int32_t res = store_i64(CurrentCode(),  N(test_table), &dave,  sizeof(TestModel));
 * res = store_i64(CurrentCode(), N(test_table), &carol, sizeof(TestModel));
 * res = store_i64(CurrentCode(), N(test_table), &bob, sizeof(TestModel));
 * res = store_i64(CurrentCOde(), N(test_table), &alice, sizeof(TestModel));
 * TestModel alice;
 * alice.name = N(alice);
 * res = load_i64( currentCode(), currentCode(), N(test_table), &alice, sizeof(TestModel) );
 * ASSERT(res == sizeof(TestModel) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "load_i64");
 *
 * res = front_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
 * ASSERT(res == sizeof(TestModel) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "front_i64 1");
 *
 * res = back_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
 * ASSERT(res == sizeof(TestModel) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "back_i64 2");
 *
 * res = previous_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
 * ASSERT(res == sizeof(TestModel) && tmp.name == N(carol) && tmp.age == 30 && tmp.phone == 545342453, "carol previous");
 *
 * res = next_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
 * ASSERT(res == sizeof(TestModel) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "back_i64 2");
 *
 * uint64_t key = N(alice);
 * res = remove_i64(currentCode(), N(test_table), &key);
 * ASSERT(res == 1, "remove alice");
 *
 * TestModel lb;
 * lb.name = N(bob);
 * res = lower_bound_i64( currentCode(), currentCode(), N(test_table), &lb, sizeof(TestModel) );
 * ASSERT(res == sizeof(TestModel) && lb.name == N(bob), "lower_bound_i64 bob" );
 *
 * TestModel ub;
 * ub.name = N(alice);
 * res = upper_bound_i64( currentCode(), currentCode(), N(test_table), &ub, sizeof(TestModel) );
 * ASSERT(res == sizeof(TestModel) && ub.age == 15 && ub.name == N(bob), "upper_bound_i64 bob" );
 * @endcode
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
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the ID/name of the table within the current scope/code context to modify
 *
 * @return 1 if the record was updated, 0 if no record with key was found
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

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the front record
 *  @param datalen - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t front_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the back record
 *  @param datalen - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t back_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the next record. Should be initialized with the key to get next record.
 *  @param datalen - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if key was not found
 */
int32_t next_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the previous record. Should be initialized with the key to get previous record.
 *  @param datalen - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if key was not found
 */
int32_t previous_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the lower bound. Should be initialized with the key to find lower bound of.
 *  @param datalen - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if key was not found
 */
int32_t lower_bound_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the upper bound. Should be initialized with the key to find upper bound of.
 *  @param datalen - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if key was not found
 */
int32_t upper_bound_i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t datalen );

/**
 *  @param scope - the account socpe that will be read, must exist in the transaction scopes list
 *  @param table - the ID/name of the table withing the scope/code context to query
 *  @param data - must point to at lest 8 bytes containing primary key
 *
 *  @return 1 if a record was removed, and 0 if no record with key was found
 */
int32_t remove_i64( AccountName scope, TableName table, void* data );

///@} db_i64

/**
 * @defgroup dbstr  Single String Index Table
 * @brief These methods interface with a simple table with String unique primary key and arbitrary binary data value.
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
 * @pre *((Name*)data) stores the primary key
 * @pre scope is declared by the current transaction
 * @pre this method is being called from an apply context (not validate or precondition)
 *
 * @post a record is either created or updated with the given scope and table.
 *
 * @throw if called with an invalid precondition execution will be aborted
 *
 */
 int32_t store_str( AccountName scope, TableName table, char* key, uint32_t keylen, char* value, uint32_t valuelen );
 
 /**
  * @return 1 if the record was updated, 0 if no record with key was found
  */
 int32_t update_str( AccountName scope, TableName table, char* key, uint32_t keylen, char* value, uint32_t valuelen );
 
 /**
  *  @param scope - the account scope that will be read, must exist in the transaction scopes list
  *  @param code  - identifies the code that controls write-access to the data
  *  @param table - the ID/name of the table within the scope/code context to query
  *  @param data  - location to copy the stored record, should be initialized with the key to get
  *  @param datalen - the maximum length of data to read, must be greater than sizeof(uint64_t)
  *
  *  @return the number of bytes read or -1 if key was not found
  */
 int32_t load_str( AccountName scope, AccountName code, TableName table, char* key, uint32_t keylen, char* value, uint32_t valuelen );
 int32_t front_str( AccountName scope, AccountName code, TableName table, char* value, uint32_t valuelen );
 int32_t back_str( AccountName scope, AccountName code, TableName table, char* value, uint32_t valuelen );
 int32_t next_str( AccountName scope, AccountName code, TableName table, char* key, uint32_t keylen, char* value, uint32_t valuelen );
 int32_t previous_str( AccountName scope, AccountName code, TableName table, char* key, uint32_t keylen, char* value, uint32_t valuelen );
 int32_t lower_bound_str( AccountName scope, AccountName code, TableName table, char* key, uint32_t keylen, char* value, uint32_t valuelen );
 int32_t upper_bound_str( AccountName scope, AccountName code, TableName table, char* key, uint32_t keylen, char* value, uint32_t valuelen );
 
 /**
  *  @param data - must point to at lest 8 bytes containing primary key
  *
  *  @return 1 if a record was removed, and 0 if no record with key was found
  */
 int32_t remove_str( AccountName scope, TableName table, char* key, uint32_t keylen );
 
 ///@} dbstr

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
 *  Example
 *  @code
 *  struct TestModel128x2 {
 *     uint128_t number;
 *     uint128_t price;
 *     uint64_t  extra;
 *     uint64_t  table_name;
 *  };
 *
 *  TestModel128x2 alice{0, 500, N(alice), N(table_name)};
 *  TestModel128x2 bob{1, 1000, N(bob), N(table_name)};
 *  TestModel128x2 carol{2, 1500, N(carol), N(table_name)};
 *  TestModel128x2 dave{3, 2000, N(dave), N(table_name)};
 *  int32_t res = store_i128i128(CurrentCode(), N(table_name), &alice, sizeof(TestModel128x2));
 *  res = store_i128i128(CurrentCode(), N(table_name), &bob, sizeof(TestModel128x2));
 *  ASSERT(res == 1, "db store failed");
 *  res = store_i128i128(CurrentCode(), N(table_name), &carol, sizeof(TestModel128x2));
 *  ASSERT(res == 1, "db store failed");
 *  res = store_i128i128(CurrentCode(), N(table_name), &dave, sizeof(TestModel128x2));
 *  ASSERT(res == 1, "db store failed");
 *
 *  TestModel128x2 query;
 *  query.number = 0;
 *  res = load_primary_i128i128(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel128x2));
 *  ASSERT(res == sizeof(TestModel128x2) && query.number == 0 && query.price == 500 && query.extra == N(alice), "load");
 *
 *  res = front_primary_i128i128(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel128x2));
 *  ASSERT(res == sizeof(TestModel128x2) && query.number == 3 && query.price = 2000 && query.extra == N(dave), "front");
 *
 *  res = next_primary_i128i128(CurrentCode(), CurrentCode(), N(table_name), & query, sizeof(TestModel128x2));
 *  ASSERT(res == sizeof(TestModel128x2) && query.number == 2 && query.price == 1500 && query.extra == N(carol), "next");
 *
 *  res = back_primary_i128i128(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel128x2));
 *  ASSERT(res == sizeof(TestModel128x2) && query.number == 0 && query.price == 500 && query.extra == N(alice), "back");
 *
 *  res = previous_primary_i128i128(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel128x2));
 *  ASSERT(res == sizeof(TestModel128x2) && query.number == 1 && query.price == 1000 && query.extra == N(bob), "previous");
 *
 *  query.number = 0;
 *  res = lower_bound_primary_i128i128(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel128x2));
 *  ASSERT(res == sizeof(TestModel128x2) && query.number == 0 && query.price == 500 && query.extra == N(alice), "lower");
 *
 *  res = upper_bound_primary_i128i128(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel128x2));
 *  ASSERT(res == sizeof(TestModel128x2) && query.number == 1 && query.price == 1000 && query.extra == N(bob), "upper");
 *
 * query.extra = N(bobby);
 * res = update_i128128(CurrentCode(), N(table_name), &query, sizeof(TestModel128x2));
 * ASSERT(res == sizeof(TestModel128x2) && query.number == 1 & query.price == 1000 && query.extra == N(bobby), "update");
 *
 * res = remove_i128128(CurrentCode(), N(table_name), &query, sizeof(TestModel128x2));
 * ASSERT(res == 1, "remove")
 *  @endcode
 *
 *  @{
 */

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param code - the code which owns the table
 * @param table - the ID/name of the table within the current scope/code context to modify
 * @param data - location to copy the record, must be initialized with the primary key to load
 * @param len - length of record to copy
 * @return the number of bytes read, -1 if key was not found
 *
 * @pre len >= sizeof(uint128_t)
 * @pre data is a valid pointer to a range of memory at least datalen bytes long
 * @pre *((uint128_t*)data) stores the primary key
 * @pre scope is declared by the current transaction
 * @pre this method is being called from an apply context (not validate or precondition)
 *
 * @post data will be initialized with the len bytes of record matching the key.
 *
 * @throw if called with an invalid precondition execution will be aborted
 *
 */
int32_t load_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the front record of primary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t front_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the back record of primary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t back_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the next record of primary key; must be initialized with a key.
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t next_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the previous record of primary key; must be initialized with a key.
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t previous_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the upper bound of a primary key; must be initialized with a key.
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t upper_bound_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the lower bound of a primary key; must be initialized with a key.
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t lower_bound_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param code - the code which owns the table
 * @param table - the ID/name of the table within the current scope/code context to modify
 * @param data - location to copy the record, must be initialized with the secondary key to load
 * @param len - length of record to copy
 * @return the number of bytes read, -1 if key was not found
 *
 * @pre len >= sizeof(uint128_t)
 * @pre data is a valid pointer to a range of memory at least datalen bytes long
 * @pre *((uint128_t*)data) stores the secondary key
 * @pre scope is declared by the current transaction
 * @pre this method is being called from an apply context (not validate or precondition)
 *
 * @post data will be initialized with the len bytes of record matching the key.
 *
 * @throw if called with an invalid precondition execution will be aborted
 *
 */
int32_t load_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the front record of secondary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t front_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the back record of secondary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t back_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the next record of secondary key; must be initialized with a key.
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t next_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the previous record of secondary key; must be initialized with a key.
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t previous_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the upper bound of given secondary key; must be initialized with a key.
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t upper_bound_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the lower bound of given secondary key; must be initialized with a key.
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t lower_bound_secondary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );


/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the ID/name of the table within the scope/code context to query
 * @param data - must point to at lest 32 bytes containing {primary,secondary}
 *
 * @return 1 if a record was removed, and 0 if no record with key was found
 */
int32_t remove_i128i128( AccountName scope, TableName table, const void* data );
/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the ID/name of the table within the scope/code context to query
 * @param data - must point to a at least 32 bytes containing (primary, secondary)
 * @param len - the length of the data
 * @return 1 if a new record was created, 0 if an existing record was updated
 */
int32_t store_i128i128( AccountName scope, TableName table, const void* data, uint32_t len );

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the ID/name of the table within the scope/code context to query
 * @param data - must to a at least 32 bytes containing (primary, secondary)
 * @param len - the length of the data
 * @return 1 if the record was updated, 0 if no record with key was found
 */
int32_t update_i128i128( AccountName scope, TableName table, const void* data, uint32_t len );

///@}  dbi128i128

/**
 *  @defgroup dbi64i64i64 Triple 64 bit Index Table
 *  @brief Interface to a database table with 64 bit primary, secondary and tertiary keys and arbitrary binary data value.
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
 *  You can iterate over these indices with primary index sorting records by { primary, secondary, tertiary },
 *  the secondary index sorting records by { secondary, tertiary } and the tertiary index sorting records by
 *  { tertiary }.
 *
 *  @see Table class in C++ API
 *
 *  Example
 *  @code
 *  struct TestModel3xi64 {
 *         uint64_t a;
 *         uint64_t b;
 *         uint64_t c;
 *         uint64_t name;
 *  };
 *
 *  TestModel3xi64 alice{ 0, 0, 0, N(alice) };
 *  TestModel3xi64 bob{ 1, 1, 1, N(bob) };
 *  TestModel3xi64 carol{ 2, 2, 2, N(carol) };
 *  TestModel3xi64 dave{ 3, 3, 3, N(dave) };
 *
 *  int32_t res = store_i64i64i64(CurrentCode(), N(table_name), &alice, sizeof(TestModel3xi64));
 *  res = store_i64i64i64(CurrentCode(), N(table_name), &bob, sizeof(TestModel3xi64));
 *  res = store_i64i64i64(CurrentCode(), N(table_name), &carol, sizeof(TestModel3xi64));
 *  res = store_i64i64i64(CurrentCode(), N(table_name), &dave, sizeof(TestModel3xi64));
 *
 *  TestModel3xi64 query;
 *  query.a = 0;
 *  res = load_primary_i64i64i64(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel3xi64));
 *  ASSERT(res == sizeof(TestModel3xi64) && query.name == N(alice), "load");
 *
 *  res = front_primary_i64i64i64(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel3xi64));
 *  ASSERT(res == sizeof(TestModel3xi64) && query.name == N(dave), "front");
 *
 *  res = back_primary_i64i64i64(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel3xi64));
 *  ASSERT(res == sizeof(TestModel3xi64) && query.name == N(alice), "back");
 *
 *  res = previous_primary_i64i64i64(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel3xi64));
 *  ASSERT(res == sizeof(TestModel3xi64) && query.name == N(bob), "previous");
 *
 *  res = next_primary_i64i64i64(CurrentCode(), CurrentCode(), N(table_name), &query, sizeof(TestModel3xi64));
 *  ASSERT(res == sizeof(TestModel3xi64) && query.name == N(alice), "next");*
 *
 *  @endcode
 *  @{
 */

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param code - the code which owns the table
 * @param table - the ID/name of the table within the current scope/code context to modify
 * @param data - location to copy the record, must be initialized with the (primary,secondary,tertiary) to load
 * @param len - length of record to copy
 * @return the number of bytes read, -1 if key was not found
 *
  * @pre data is a valid pointer to a range of memory at least len bytes long
 * @pre *((uint64_t*)data) stores the primary key
 * @pre scope is declared by the current transaction
 * @pre this method is being called from an apply context (not validate or precondition)
 *
 * @post data will be initialized with the len bytes of record matching the key.
 *
 * @throw if called with an invalid precondition execution will be aborted
 *
 */
int32_t load_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the front record of primary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t front_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the back record of primary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t back_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the next record of primary key; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t next_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the previous record of primary key; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t previous_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the upper bound of a primary key; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t upper_bound_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the lower bound of primary key; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t lower_bound_primary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param code - the code which owns the table
 * @param table - the ID/name of the table within the current scope/code context to modify
 * @param data - location to copy the record, must be initialized with the (secondary,tertiary) to load
 * @param len - length of record to copy
 * @return the number of bytes read, -1 if key was not found
 *
  * @pre data is a valid pointer to a range of memory at least len bytes long
 * @pre *((uint64_t*)data) stores the secondary key
 * @pre scope is declared by the current transaction
 * @pre this method is being called from an apply context (not validate or precondition)
 *
 * @post data will be initialized with the len bytes of record matching the key.
 *
 * @throw if called with an invalid precondition execution will be aborted
 *
 */
int32_t load_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the front record of a secondary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t front_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the back record of secondary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t back_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the next record; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t next_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the previous record; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t previous_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the upper bound of tertiary key; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t upper_bound_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the lower bound of secondary key; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t lower_bound_secondary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param code - the code which owns the table
 * @param table - the ID/name of the table within the current scope/code context to modify
 * @param data - location to copy the record, must be initialized with the (tertiary) to load
 * @param len - length of record to copy
 * @return the number of bytes read, -1 if key was not found
 *
  * @pre data is a valid pointer to a range of memory at least len bytes long
 * @pre *((uint64_t*)data) stores the tertiary key
 * @pre scope is declared by the current transaction
 * @pre this method is being called from an apply context (not validate or precondition)
 *
 * @post data will be initialized with the len bytes of record matching the key.
 *
 * @throw if called with an invalid precondition execution will be aborted
 *
 */
int32_t load_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the front record of a tertiary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t front_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the back record of a tertiary key
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t back_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the next record; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t next_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the previous record; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t previous_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the upper bound of tertiary key; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t upper_bound_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param data  - location to copy the lower bound of tertiary key; must be initialized with a key value
 *  @param len - the maximum length of data to read, must be greater than sizeof(uint64_t)
 *
 *  @return the number of bytes read or -1 if no record found
 */
int32_t lower_bound_tertiary_i64i64i64( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the name of table where record is stored
 * @param data - must point to at least 24 bytes containing {primary,secondary,tertiary}
 *
 * @return 1 if a record was removed, and 0 if no record with key was found
 */
int32_t remove_i64i64i64( AccountName scope, TableName table, const void* data );
/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the name of table where record is stored
 * @param data - must point to at least 24 bytes containing (primary,secondary,tertiary)
 * @param len - length of the data
 * @return 1 if a new record was created, 0 if an existing record was updated
 */
int32_t store_i64i64i64( AccountName scope, TableName table, const void* data, uint32_t len );

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the name of table where record is stored
 * @param data - must point to at least 24 bytes containing (primary,secondary,tertiary)
 * @param len - length of the data
 * @return 1 if the record was updated, 0 if no record with key was found
 */
int32_t update_i64i64i64( AccountName scope, TableName table, const void* data, uint32_t len );

///@}  dbi64i64i64
}
