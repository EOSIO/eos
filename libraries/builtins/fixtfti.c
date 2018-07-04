/* ===-- fixtfti.c - Implement __fixtfti -----------------------------------===
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 */

#include "fp128.h"

__int128 ___fixtfti( float128_t a) {
    const __int128 fixint_max = (__int128)((~(unsigned __int128)0) / 2);
    const __int128 fixint_min = -fixint_max - 1;
    // Break a into sign, exponent, significand
    const __int128 aRep = toRep(a);
    const __int128 aAbs = aRep & absMask;
    const __int128 sign = aRep & signBit ? -1 : 1;
    const int exponent = (aAbs >> significandBits) - exponentBias;
    const __int128 significand = (aAbs & significandMask) | implicitBit;

    // If exponent is negative, the result is zero.
    if (exponent < 0)
        return 0;

    // If the value is too large for the integer type, saturate.
    if ((unsigned)exponent >= sizeof(__int128) * CHAR_BIT)
        return sign == 1 ? fixint_max : fixint_min;

    // If 0 <= exponent < significandBits, right shift to get the result.
    // Otherwise, shift left.
    if (exponent < significandBits)
        return sign * (significand >> (significandBits - exponent));
    else
        return sign * ((__int128)significand << (exponent - significandBits));
}
