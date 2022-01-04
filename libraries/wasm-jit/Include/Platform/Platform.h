#pragma once

#include <vector>
#include <functional>

#include "Inline/BasicTypes.h"
#include "Inline/Errors.h"

// Use __thread instead of the C++11 thread_local because Apple's clang doesn't support thread_local yet.
#define THREAD_LOCAL __thread
#define DLL_EXPORT
#define DLL_IMPORT
#define FORCEINLINE inline __attribute__((always_inline))
#define SUPPRESS_UNUSED(variable) (void)(variable);
#define PACKED_STRUCT(definition) definition __attribute__((packed));

#ifndef PLATFORM_API
	#define PLATFORM_API DLL_IMPORT
#endif

namespace Platform
{
	// countLeadingZeroes/countTrailingZeroes returns the number of leading/trailing zeroes, or the bit width of the input if no bits are set.
		// __builtin_clz/__builtin_ctz are undefined if the input is 0.
	inline U64 countLeadingZeroes(U64 value) { return value == 0 ? 64 : __builtin_clzll(value); }
	inline U32 countLeadingZeroes(U32 value) { return value == 0 ? 32 : __builtin_clz(value); }
	inline U64 countTrailingZeroes(U64 value) { return value == 0 ? 64 : __builtin_ctzll(value); }
	inline U32 countTrailingZeroes(U32 value) { return value == 0 ? 32 : __builtin_ctz(value); }
	inline U64 floorLogTwo(U64 value) { return value <= 1 ? 0 : 63 - countLeadingZeroes(value); }
	inline U32 floorLogTwo(U32 value) { return value <= 1 ? 0 : 31 - countLeadingZeroes(value); }
	inline U64 ceilLogTwo(U64 value) { return floorLogTwo(value * 2 - 1); }
	inline U32 ceilLogTwo(U32 value) { return floorLogTwo(value * 2 - 1); }
}
