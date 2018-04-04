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
typedef uint16_t weight_type;

#define PACKED(X) __attribute((packed)) X

struct public_key {
   char data[34];
};

struct signature {
   uint8_t data[66];
};

struct checksum256 {
   uint8_t hash[32];
};

struct checksum160 {
   uint8_t hash[20];
};

struct checksum512 {
   uint8_t hash[64];
};

struct fixed_string16 {
   uint8_t len;
   char str[16];
};

typedef struct checksum256 transaction_id_type;

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
