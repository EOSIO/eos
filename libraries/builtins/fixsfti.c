/* ===-- fixsfti.c - Implement __fixsfti -----------------------------------===
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 */

#include "fp32.h"

typedef __int128 fixint_t;
typedef unsigned __int128 fixuint_t;

fixint_t ___fixsfti(uint32_t a) {
    const fixint_t fixint_max = (fixint_t)((~(fixuint_t)0) / 2);
    const fixint_t fixint_min = -fixint_max - 1;
    // Break a into sign, exponent, significand
    const rep_t aRep = a;
    const rep_t aAbs = aRep & absMask;
    const fixint_t sign = aRep & signBit ? -1 : 1;
    const int exponent = (aAbs >> significandBits) - exponentBias;
    const rep_t significand = (aAbs & significandMask) | implicitBit;

    // If exponent is negative, the result is zero.
    if (exponent < 0)
        return 0;

    // If the value is too large for the integer type, saturate.
    if ((unsigned)exponent >= sizeof(fixint_t) * CHAR_BIT)
        return sign == 1 ? fixint_max : fixint_min;

    // If 0 <= exponent < significandBits, right shift to get the result.
    // Otherwise, shift left.
    if (exponent < significandBits)
        return sign * (significand >> (significandBits - exponent));
    else
        return sign * ((fixint_t)significand << (exponent - significandBits));
}
