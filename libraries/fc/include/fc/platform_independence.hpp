#ifdef _MSC_VER
#include <intrin.h>
    #ifdef _M_X64
    #define __builtin_popcountll __popcnt64
    #else
    inline int __builtin_popcountll(unsigned __int64 value)
    {
       unsigned int lowBits = (unsigned int)value;
       int count = __popcnt(lowBits);
       unsigned int highBits = (unsigned int)(value >> 32);
       count += __popcnt(highBits);
       return count;
    }
    #endif
#endif