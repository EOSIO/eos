#pragma once
#include <cstdint>
#include <cstddef>

typedef uint8_t U8;
typedef int8_t I8;
typedef uint16_t U16;
typedef int16_t I16;
typedef uint32_t U32;
typedef int32_t I32;
typedef uint64_t U64;
typedef int64_t I64;
typedef float F32;
typedef double F64;

// The OSX libc defines uintptr_t to be a long where U32/U64 are int. This causes uintptr_t/uint64 to be treated as distinct types for e.g. overloading.
// Work around it by defining our own Uptr/Iptr that are always int type.
template<size_t pointerSize>
struct PointerIntHelper;
template<> struct PointerIntHelper<4> { typedef I32 IntType; typedef U32 UnsignedIntType; };
template<> struct PointerIntHelper<8> { typedef I64 IntType; typedef U64 UnsignedIntType; };
typedef PointerIntHelper<sizeof(size_t)>::UnsignedIntType Uptr;
typedef PointerIntHelper<sizeof(size_t)>::IntType Iptr;
