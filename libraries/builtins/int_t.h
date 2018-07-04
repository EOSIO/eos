#ifndef __compiler_rt_int_t_h__
#define __compiler_rt_int_t_h__
#include <stdint.h>
#include <limits.h>

typedef union
{
    __int128 all;
    struct
    {
        uint64_t low;
        int64_t high;
    }s;
} twords;

typedef union
{
    unsigned __int128 all;
    struct
    {
        uint64_t low;
        uint64_t high;
    }s;
} utwords;

typedef union
{
    uint64_t all;
    struct
    {
        uint32_t low;
        uint32_t high;
    }s;
} udwords;

typedef union
{
    udwords u;
    double  f;
} double_bits;


typedef __int128 ti_int;
typedef unsigned __int128 tu_int;
inline __int128 __clzti2(__int128 a)
{
    twords x;
    x.all = a;
    const int64_t f = -(x.s.high == 0);
    return __builtin_clzll((x.s.high & ~f) | (x.s.low & f)) +
           ((int32_t)f & ((int32_t)(sizeof(int64_t) * CHAR_BIT)));
}

#endif// __compiler_rt_int_t_h__
