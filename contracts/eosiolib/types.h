/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

extern "C" {

/**
 *  @defgroup types Builtin Types
 *  @ingroup contractdev
 *  @brief Specifies typedefs and aliases
 *
 *  @{
 */

typedef unsigned __int128    uint128_t;
typedef unsigned long long   uint64_t;
typedef unsigned long        uint32_t;
typedef unsigned short       uint16_t; 
typedef unsigned char        uint8_t;

typedef __int128             int128_t;
typedef long long            int64_t;
typedef long                 int32_t;
typedef short                int16_t;
typedef char                 int8_t;

struct uint256 {
   uint64_t words[4];
};

typedef unsigned int size_t;

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
   char    str[16];
};

typedef fixed_string16 field_name;

struct fixed_string32 {
   uint8_t len;
   char    str[32];
};

typedef fixed_string32 type_name;

struct account_permission {
   account_name account;
   permission_name permission;
};

} /// extern "C"
/// @}
