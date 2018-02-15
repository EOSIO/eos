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
typedef uint16_t region_id;

typedef uint64_t asset_symbol;
typedef int64_t share_type;

#define PACKED(X) __attribute((packed)) X

struct public_key {
   uint8_t data[33];
};

struct signature {
   uint8_t data[65];
};

struct checksum {
   uint64_t hash[4];
};

struct fixed_string16 {
   uint8_t len;
   char str[16];
};

typedef struct fixed_string16 field_name;

struct fixed_string32 {
   uint8_t len;
   char str[32];
};

typedef struct fixed_string32 type_name;

struct account_permission {
   account_name account;
   permission_name permission;
};

union {
   uint8_t bytes[8];
   uint16_t uint16[8];
   uint32_t uint32[8];
   uint64_t uint64[4];
   uint128_t uint128[2];
} typedef uint256;

bool operator==( const uint256& a, const uint256& b ) {
   return a.uint128[0] == b.uint128[0] && a.uint128[1] == b.uint128[1];
}

bool operator!=( const uint256& a, const uint256& b ) {
   return !(a == b);
}

#ifdef __cplusplus
} /// extern "C"
#endif
/// @}
