#pragma once

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

typedef int8_t           int_least8_t;
typedef int16_t         int_least16_t;
typedef int32_t         int_least32_t;
typedef int64_t         int_least64_t;
typedef uint8_t         uint_least8_t;
typedef uint16_t       uint_least16_t;
typedef uint32_t       uint_least32_t;
typedef uint64_t       uint_least64_t;

typedef int8_t            int_fast8_t;
typedef int16_t          int_fast16_t;
typedef int32_t          int_fast32_t;
typedef int64_t          int_fast64_t;
typedef uint8_t          uint_fast8_t;
typedef uint16_t        uint_fast16_t;
typedef uint32_t        uint_fast32_t;
typedef uint64_t        uint_fast64_t;

// /usr/local/wasm/lib/clang/4.0.1/include/stdint.h
#define __stdint_join3(a,b,c) a ## b ## c

#define  __intn_t(n) __stdint_join3( int, n, _t)
typedef  __intn_t(__INTPTR_WIDTH__)  intptr_t;

#define __uintn_t(n) __stdint_join3(uint, n, _t)
typedef __uintn_t(__INTPTR_WIDTH__) uintptr_t;

typedef __INTMAX_TYPE__  intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;

// musl/x86_64/bits/stdint.h

#define INT_FAST16_MIN  INT32_MIN
#define INT_FAST32_MIN  INT32_MIN

#define INT_FAST16_MAX  INT32_MAX
#define INT_FAST32_MAX  INT32_MAX

#define UINT_FAST16_MAX UINT32_MAX
#define UINT_FAST32_MAX UINT32_MAX

#define INTPTR_MIN      INT64_MIN
#define INTPTR_MAX      INT64_MAX
#define UINTPTR_MAX     UINT64_MAX
#define PTRDIFF_MIN     INT64_MIN
#define PTRDIFF_MAX     INT64_MAX
#define SIZE_MAX        UINT64_MAX
