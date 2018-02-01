/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/types.h>
extern "C" {
/**
 *  This method is implemented as:
 *
 *  checksum calc_hash;
 *  sha256( data, length, &calc_hash );
 *  assert( calc_hash == hash, "invalid hash" );
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void assert_sha256( char* data, uint32_t length, const checksum* hash );

/**
 *  Calculates sha256( data,length) and stores result in memory pointed to by hash 
 */
void sha256( char* data, uint32_t length, checksum* hash );
}