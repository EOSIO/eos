/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosiolib/types.h>
extern "C" {

/**
 *  @defgroup cryptoapi Chain API
 *  @brief Defines API for calculating and checking hash
 *  @ingroup contractdev
 */

/**
 *  @defgroup cryptocapi Chain C API
 *  @brief Defines %C API for calculating and checking hash
 *  @ingroup chainapi
 *  @{
 */

/**
 *  Tests if the sha256 hash generated from data matches the provided checksum.
 *  This method is optimized to a NO-OP when in fast evaluation mode.
 *  @brief Tests if the sha256 hash generated from data matches the provided checksum.
 *
 *  @param data - Data you want to hash
 *  @param length - Data length
 *  @param hash - `checksum256*` hash to compare to
 *
 *  @pre **assert256 hash** of `data` equals provided `hash` parameter.
 *  @post Executes next statement. If was not `true`, hard return.
 *
 *  Example:
*
 *  @code
 *  checksum hash;
 *  char data;
 *  uint32_t length;
 *  assert_sha256( data, length, hash )
 *  //If the sha256 hash generated from data does not equal provided hash, anything below will never fire.
 *  eosio::print("sha256 hash generated from data equals provided hash");
 *  @endcode
 */
void assert_sha256( const char* data, uint32_t length, const checksum256* hash );

/**
 *  Tests if the sha1 hash generated from data matches the provided checksum.
 *  This method is optimized to a NO-OP when in fast evaluation mode.
 *  @brief Tests if the sha1 hash generated from data matches the provided checksum.
 *
 *  @param data - Data you want to hash
 *  @param length - Data length
 *  @param hash - `checksum160*` hash to compare to
 *
 *  @pre **sha1 hash** of `data` equals provided `hash` parameter.
 *  @post Executes next statement. If was not `true`, hard return.
 *
 *  Example:
*
 *  @code
 *  checksum hash;
 *  char data;
 *  uint32_t length;
 *  assert_sha1( data, length, hash )
 *  //If the sha1 hash generated from data does not equal provided hash, anything below will never fire.
 *  eosio::print("sha1 hash generated from data equals provided hash");
 *  @endcode
 */
void assert_sha1( const char* data, uint32_t length, const checksum160* hash );

/**
 *  Tests if the sha512 hash generated from data matches the provided checksum.
 *  This method is optimized to a NO-OP when in fast evaluation mode.
 *  @brief Tests if the sha512 hash generated from data matches the provided checksum.
 *
 *  @param data - Data you want to hash
 *  @param length - Data length
 *  @param hash - `checksum512*` hash to compare to
 *
 *  @pre **assert512 hash** of `data` equals provided `hash` parameter.
 *  @post Executes next statement. If was not `true`, hard return.
 *
 *  Example:
*
 *  @code
 *  checksum hash;
 *  char data;
 *  uint32_t length;
 *  assert_sha512( data, length, hash )
 *  //If the sha512 hash generated from data does not equal provided hash, anything below will never fire.
 *  eosio::print("sha512 hash generated from data equals provided hash");
 *  @endcode
 */
void assert_sha512( const char* data, uint32_t length, const checksum512* hash );

/**
 *  Tests if the ripemod160 hash generated from data matches the provided checksum.
 *  @brief Tests if the ripemod160 hash generated from data matches the provided checksum.
 *
 *  @param data - Data you want to hash
 *  @param length - Data length
 *  @param hash - `checksum160*` hash to compare to
 *
 *  @pre **assert160 hash** of `data` equals provided `hash` parameter.
 *  @post Executes next statement. If was not `true`, hard return.
 *
 *  Example:
*
 *  @code
 *  checksum hash;
 *  char data;
 *  uint32_t length;
 *  assert_ripemod160( data, length, hash )
 *  //If the ripemod160 hash generated from data does not equal provided hash, anything below will never fire.
 *  eosio::print("ripemod160 hash generated from data equals provided hash");
 *  @endcode
 */
void assert_ripemd160( const char* data, uint32_t length, const checksum160* hash );

/**
 *  Hashes `data` using `sha256` and stores result in memory pointed to by hash.
 *  @brief Hashes `data` using `sha256` and stores result in memory pointed to by hash.
 *
 *  @param data - Data you want to hash
 *  @param length - Data length
 *  @param hash - Hash pointer
 *
 *  Example:
*
 *  @code
 *  checksum calc_hash;
 *  sha256( data, length, &calc_hash );
 *  eos_assert( calc_hash == hash, "invalid hash" );
 *  @endcode
 */
void sha256( const char* data, uint32_t length, checksum256* hash );

/**
 *  Hashes `data` using `sha1` and stores result in memory pointed to by hash.
 *  @brief Hashes `data` using `sha1` and stores result in memory pointed to by hash.
 *
 *  @param data - Data you want to hash
 *  @param length - Data length
 *  @param hash - Hash pointer
 *
 *  Example:
*
 *  @code
 *  checksum calc_hash;
 *  sha1( data, length, &calc_hash );
 *  eos_assert( calc_hash == hash, "invalid hash" );
 *  @endcode
 */
void sha1( const char* data, uint32_t length, checksum160* hash );

/**
 *  Hashes `data` using `sha512` and stores result in memory pointed to by hash.
 *  @brief Hashes `data` using `sha512` and stores result in memory pointed to by hash.
 *
 *  @param data - Data you want to hash
 *  @param length - Data length
 *  @param hash - Hash pointer
 *
 *  Example:
*
 *  @code
 *  checksum calc_hash;
 *  sha512( data, length, &calc_hash );
 *  eos_assert( calc_hash == hash, "invalid hash" );
 *  @endcode
 */
void sha512( const char* data, uint32_t length, checksum512* hash );

/**
 *  Hashes `data` using `ripemod160` and stores result in memory pointed to by hash.
 *  @brief Hashes `data` using `ripemod160` and stores result in memory pointed to by hash.
 *
 *  @param data - Data you want to hash
 *  @param length - Data length
 *  @param hash - Hash pointer
 *
 *  Example:
*
 *  @code
 *  checksum calc_hash;
 *  ripemod160( data, length, &calc_hash );
 *  eos_assert( calc_hash == hash, "invalid hash" );
 *  @endcode
 */
void ripemd160( const char* data, uint32_t length, checksum160* hash );

/**
 *  Calculates the public key used for a given signature and hash used to create a message.
 *  @brief Calculates the public key used for a given signature and hash used to create a message.
 *
 *  @param digest - Hash used to create a message
 *  @param sig - Signature
 *  @param siglen - Signature length
 *  @param pub - Public key
 *  @param publen - Public key length
 *
 *  Example:
*
 *  @code
 *  @endcode
 */
int recover_key( const checksum256* digest, const char* sig, size_t siglen, char* pub, size_t publen );

/**
 *  Tests a given public key with the generated key from digest and the signature.
 *  @brief Tests a given public key with the generated key from digest and the signature.
 *
 *  @param digest - What the key will be generated from
 *  @param sig - Signature
 *  @param siglen - Signature length
 *  @param pub - Public key
 *  @param publen - Public key length
 *
 *  @pre **assert recovery key** of `pub` equals the key generated from the `digest` parameter
 *  @post Executes next statement. If was not `true`, hard return.
 *
 *  Example:
*
 *  @code
 *  checksum digest;
 *  char sig;
 *  size_t siglen;
 *  char pub;
 *  size_t publen;
 *  assert_recover_key( digest, sig, siglen, pub, publen )
 *  // If the given public key does not match with the generated key from digest and the signature, anything below will never fire.
 *  eosio::print("pub key matches the pub key generated from digest");
 *  @endcode
 */
void assert_recover_key( const checksum256* digest, const char* sig, size_t siglen, const char* pub, size_t publen );

/// }@cryptocapi

}
