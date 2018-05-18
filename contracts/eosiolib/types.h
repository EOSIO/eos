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
 *  @brief Specifies typedefs and aliases
 *
 *  @{
 */

typedef uint64_t account_name;
typedef uint64_t permission_name;
typedef uint64_t token_name;
typedef uint64_t table_name;
typedef uint32_t time;
typedef uint64_t scope_name;
typedef uint64_t action_name;

typedef uint64_t asset_symbol;
typedef int64_t share_type;
typedef uint16_t weight_type;

/* macro to align/overalign a type to ensure calls to intrinsics with pointers/references are properly aligned */
#define ALIGNED(X) __attribute__ ((aligned (16))) X

struct public_key {
   char data[34];
};

struct signature {
   uint8_t data[66];
};

struct ALIGNED(checksum256) {
   uint8_t hash[32];
};

struct ALIGNED(checksum160) {
   uint8_t hash[20];
};

struct ALIGNED(checksum512) {
   uint8_t hash[64];
};

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
