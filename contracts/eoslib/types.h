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
typedef long                 int32_t;
typedef unsigned __int128    uint128_t;
typedef __int128             int128_t;
typedef unsigned char        uint8_t;

typedef uint64_t AccountName;
typedef uint64_t PermissionName;
typedef uint64_t TokenName;
typedef uint64_t TableName;
typedef uint32_t Time;
typedef uint64_t FuncName;

#define PACKED(X) __attribute((packed)) X

struct uint256 {
   uint64_t words[4];
};

} /// extern "C"
/// @}
