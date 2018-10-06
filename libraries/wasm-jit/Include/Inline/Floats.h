#pragma once

#include "Inline/BasicTypes.h"
#include "Inline/Errors.h"

#include <string>
#include <cstdio>

namespace Floats
{	
	template<typename Float>
	struct FloatComponents;

	// The components of a 64-bit float
	template<>
	struct FloatComponents<F64>
	{
		typedef U64 Bits;
		typedef F64 Float;

		enum Constants : I64
		{
			maxSignificand = 0xfffffffffffff,
			numSignificandBits = 52,
			numSignificandHexits = 13,
			canonicalSignificand = 0x8000000000000ull,

			denormalExponent = -1023,
			minNormalExponent = -1022,
			maxNormalExponent = 1023,
			exponentBias = 1023,
			maxExponentBits = 0x7ff,
		};

		union
		{
			struct
			{
				U64 significand : 52;
				U64 exponent : 11;
				U64 sign : 1;
			} bits;
			Float value;
			Bits bitcastInt;
		};
	};

	// The components of a 32-bit float.
	template<>
	struct FloatComponents<F32>
	{
		typedef U32 Bits;
		typedef F32 Float;
		
		enum Constants : I32
		{
			maxSignificand = 0x7fffff,
			numSignificandBits = 23,
			numSignificandHexits = 6,
			canonicalSignificand = 0x400000,

			denormalExponent = -127,
			minNormalExponent = -126,
			maxNormalExponent = 127,
			exponentBias = 127,
			maxExponentBits = 0xff,
		};

		union
		{
			struct
			{
				U32 significand : 23;
				U32 exponent : 8;
				U32 sign : 1;
			} bits;
			Float value;
			Bits bitcastInt;
		};
	};

	// Prints a floating point value to a string, using the WebAssembly syntax for text floats.
	template<typename Float>
	std::string asString(Float f)
	{
		FloatComponents<Float> components;
		components.value = f;
	
		auto sign = std::string(components.bits.sign ? "-" : "+");

		if(components.bits.exponent == FloatComponents<Float>::maxExponentBits)
		{
			// Handle infinity.
			if(components.bits.significand == 0) { return sign + "infinity"; }
			else
			{
				// Handle NaN.
				char significandString[FloatComponents<Float>::numSignificandHexits + 1];
				for(Uptr hexitIndex = 0;hexitIndex < FloatComponents<Float>::numSignificandHexits;++hexitIndex)
				{
					auto hexitValue = char((components.bits.significand >> ((FloatComponents<Float>::numSignificandHexits - hexitIndex - 1) * 4)) & 0xf);
					significandString[hexitIndex] = hexitValue >= 10 ? ('a' + hexitValue - 10) : ('0' + hexitValue);
				}
				significandString[FloatComponents<Float>::numSignificandHexits] = 0;
				return sign + "nan:0x" + significandString + "";
			}
		}
		else
		{
			// If it's not infinite or NaN, just use the STL hexadecimal float printing.
			char buffer[32];
			auto numChars = std::sprintf(buffer,FloatComponents<Float>::numSignificandHexits == 6 ? "%.6a" : "%.13a",f);
			if(unsigned(numChars) + 1 > sizeof(buffer))
			{
				Errors::fatal("not enough space in Floats::asString buffer");
			}
			return buffer;
		}
	}
}
