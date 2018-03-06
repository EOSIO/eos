/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <stdint.h>
#include <wchar.h>

template <uint32_t N>
struct checksum_n {
   uint8_t hash[N>>3];
};

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
   char data[34];
};

struct signature {
   uint8_t data[66];
};

typedef checksum_n<160> checksum160;
typedef checksum_n<256> checksum256;
typedef checksum_n<512> checksum512;

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

#ifdef __cplusplus
} /// extern "C"
#endif
/// @}
