#pragma once

#include <eoslib/types.h>

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the ID/name of the table within the current scope/code context to modify
 * @param key   - an key that can be used to lookup data in the table
 *
 * @return a unique ID assigned to this table record separate from the key, used for iterator access
 */
int32_t store_i64( AccountName scope, TableName table, uint64_t key, const void* data, uint32_t datalen );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param key   - an key that can be used to lookup data in the table
 *  @param data  - location to copy the data stored at key
 *  @param datalen - the maximum length of data to read
 *
 *  @return the number of bytes read or -1 if key was not found
 */
int32_t load_i64( AccountName scope, AccountName code, TableName table, uint64_t key, void* data, uint32_t datalen );
int32_t remove_i64( AccountName scope, TableName table, uint64_t key );



/**
 *  These methods expect data to point to a record that is at least 2*sizeof(uint128_t) where the leading
 *  32 bytes are the primary and secondary keys.  These keys will be interpreted and sorted as unsigned
 *  128 bit integers.  
 *
 *  @return the total number of bytes read or -1 for "not found" or "end".  
 */
///@{
int32_t front_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t back_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t next_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );
int32_t previous_primary_i128i128( AccountName scope, AccountName code, TableName table, void* data, uint32_t len );

int32_t load_primary_i128i128( AccountName scope, AccountName code, TableName table, 
                                const void* primary, void* data, uint32_t len );

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

