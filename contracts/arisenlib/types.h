/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @defgroup types Builtin Types
 *  @ingroup contractdev
 *  @brief Specifies builtin types, typedefs and aliases
 *
 *  @{
 */

/**
 * @brief Name of an account
 * @details Name of an account
 */
typedef uint64_t account_name;

/**
 * @brief Name of a permission
 * @details Name of a permission
 */
typedef uint64_t permission_name;

/**
 * @brief Name of a table
 * @details Name of a table
 */
typedef uint64_t table_name;

/**
 * @brief Time
 * @details Time
 */
typedef uint32_t time;

/**
 * @brief Name of a scope
 * @details Name of a scope
 */
typedef uint64_t scope_name;

/**
 * @brief Name of an action
 * @details Name of an action
 */
typedef uint64_t action_name;

/**
 * @brief Macro to align/overalign a type to ensure calls to intrinsics with pointers/references are properly aligned
 * @details Macro to align/overalign a type to ensure calls to intrinsics with pointers/references are properly aligned
 */

typedef uint16_t weight_type;

/* macro to align/overalign a type to ensure calls to intrinsics with pointers/references are properly aligned */
#define ALIGNED(X) __attribute__ ((aligned (16))) X

/**
 * @brief EOSIO Public Key
 * @details EOSIO Public Key. It is 34 bytes.
 */
struct public_key {
   char data[34];
};

/**
 * @brief EOSIO Signature
 * @details EOSIO Signature. It is 66 bytes.
 */
struct signature {
   uint8_t data[66];
};

/**
 * @brief 256-bit hash
 * @details 256-bit hash
 */
struct ALIGNED(checksum256) {
   uint8_t hash[32];
};

/**
 * @brief 160-bit hash
 * @details 160-bit hash
 */
struct ALIGNED(checksum160) {
   uint8_t hash[20];
};

/**
 * @brief 512-bit hash
 * @details 512-bit hash
 */
struct ALIGNED(checksum512) {
   uint8_t hash[64];
};

/**
 * @brief Type of EOSIO Transaction Id
 * @details Type of EOSIO Transaction Id. It is 256-bit hash
 */
typedef struct checksum256 transaction_id_type;
typedef struct checksum256 block_id_type;

struct account_permission {
   account_name account;
   permission_name permission;
};

#ifdef __cplusplus
} /// extern "C"
#endif
/// @}
