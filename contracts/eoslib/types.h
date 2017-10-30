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
typedef long long            int64_t;
typedef unsigned long long   uint64_t;
typedef unsigned long        uint32_t;
typedef unsigned short       uint16_t; 
typedef long                 int32_t;
typedef unsigned __int128    uint128_t;
typedef __int128             int128_t;
typedef unsigned char        uint8_t;
typedef char                 int8_t;
typedef short                int16_t;

typedef unsigned int size_t;

typedef uint64_t AccountName;
typedef uint64_t PermissionName;
typedef uint64_t TokenName;
typedef uint64_t TableName;
typedef uint32_t Time;
typedef uint64_t FuncName;

typedef uint64_t AssetSymbol;
typedef int64_t ShareType;

#define PACKED(X) __attribute((packed)) X

struct uint256 {
   uint64_t words[4];
};

struct PublicKey {
   uint8_t data[33];
};

struct Asset {
   ShareType   amount;
   AssetSymbol symbol;
};

struct Price {
   Asset base;
   Asset quote;
};

struct Signature {
   uint8_t data[65];
};

struct Checksum {
   uint64_t hash[4];
};

struct FixedString16 {
   uint8_t len;
   char    str[16];
};

typedef FixedString16 FieldName;

struct FixedString32 {
   uint8_t len;
   char    str[32];
};

typedef FixedString32 TypeName;

struct Bytes {
   uint32_t len;
   uint8_t* data;
};


} /// extern "C"
/// @}
