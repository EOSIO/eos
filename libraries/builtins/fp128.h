#ifndef __compiler_rt_fp_128_h__
#define __compiler_rt_fp_128_h__

#include <limits.h>
#include <stdint.h>
#include <softfloat.h>

#define REP_C (__uint128_t)
#define significandBits 112
#define typeWidth       (sizeof(__int128)*CHAR_BIT)
#define exponentBits    (typeWidth - significandBits - 1)
#define maxExponent     ((1 << exponentBits) - 1)
#define exponentBias    (maxExponent >> 1)

#define implicitBit     (REP_C(1) << significandBits)
#define significandMask (implicitBit - 1U)
#define signBit         (REP_C(1) << (significandBits + exponentBits))
#define absMask         (signBit - 1U)
#define exponentMask    (absMask ^ significandMask)
#define oneRep          ((rep_t)exponentBias << significandBits)
#define infRep          exponentMask
#define quietBit        (implicitBit >> 1)
#define qnanRep         (exponentMask | quietBit)

static __inline __int128 toRep(float128_t x) {
    const union { float128_t f; __int128 i; } rep = {.f = x};
    return rep.i;
}

#endif //__compiler_rt_fp_h__
