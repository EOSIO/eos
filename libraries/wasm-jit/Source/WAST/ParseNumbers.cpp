#include "Inline/BasicTypes.h"
#include "Inline/Floats.h"
#include "WAST.h"
#include "Lexer.h"
#include "Parse.h"

#include <climits>
#include <cerrno>

// Include the David Gay's dtoa code.
// #define strtod and dtoa to avoid conflicting with the C standard library versions
// This is complicated by dtoa.c including stdlib.h, so we need to also include stdlib.h
// here to ensure that it isn't included with the strtod and dtoa #define.
#include <stdlib.h>

#define IEEE_8087
#define NO_HEX_FP
#define NO_INFNAN_CHECK
#define strtod parseDecimalF64
#define dtoa printDecimalF64

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4244 4083 4706 4701 4703)
#elif defined(__GNUC__) && !defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wsign-compare"
	#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
	#define Long int
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wsign-compare"
	#define Long int
#endif

#include "../ThirdParty/dtoa.c"

#ifdef _MSC_VER
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
	#undef Long
#endif

#undef IEEE_8087
#undef NO_HEX_FP
#undef NO_STRTOD_BIGCOMP
#undef NO_INFNAN_CHECK
#undef strtod
#undef dtoa

using namespace WAST;

// Parses an optional + or - sign and returns true if a - sign was parsed.
// If either a + or - sign is parsed, nextChar is advanced past it.
static bool parseSign(const char*& nextChar)
{
	if(*nextChar == '-') { ++nextChar; return true; }
	else if(*nextChar == '+') { ++nextChar; }
	return false;
}

// Parses an unsigned integer from hexits, starting with "0x", and advancing nextChar past the parsed hexits.
// be called for input that's already been accepted by the lexer as a hexadecimal integer.
static U64 parseHexUnsignedInt(const char*& nextChar,ParseState& state,U64 maxValue)
{
	const char* firstHexit = nextChar;
	assert(nextChar[0] == '0' && (nextChar[1] == 'x' || nextChar[1] == 'X'));
	nextChar += 2;
	
	U64 result = 0;
	U8 hexit = 0;
	while(tryParseHexit(nextChar,hexit))
	{
		if(result > (maxValue - hexit) / 16)
		{
			parseErrorf(state,firstHexit,"integer literal is too large");
			result = maxValue;
			while(tryParseHexit(nextChar,hexit)) {};
			break;
		}
		assert(result * 16 + hexit >= result);
		result = result * 16 + hexit;
	}
	return result;
}

// Parses an unsigned integer from digits, advancing nextChar past the parsed digits.
// Assumes it will only be called for input that's already been accepted by the lexer as a decimal integer.
static U64 parseDecimalUnsignedInt(const char*& nextChar,ParseState& state,U64 maxValue,const char* context)
{
	U64 result = 0;
	const char* firstDigit = nextChar;
	while(*nextChar >= '0' && *nextChar <= '9')
	{
		const U8 digit = *nextChar - '0';
		++nextChar;

		if(result > U64(maxValue - digit) / 10)
		{
			parseErrorf(state,firstDigit,"%s is too large",context);
			result = maxValue;
			while(*nextChar >= '0' && *nextChar <= '9') { ++nextChar; };
			break;
		}
		assert(result * 10 + digit >= result);
		result = result * 10 + digit;
	};
	return result;
}

// Parses a floating-point NaN, advancing nextChar past the parsed characters.
// Assumes it will only be called for input that's already been accepted by the lexer as a literal NaN.
template<typename Float>
Float parseNaN(const char*& nextChar,ParseState& state)
{
	typedef typename Floats::FloatComponents<Float> FloatComponents;
	FloatComponents resultComponents;
	resultComponents.bits.sign = parseSign(nextChar) ? 1 : 0;
	resultComponents.bits.exponent = FloatComponents::maxExponentBits;

	assert(nextChar[0] == 'n'
	&& nextChar[1] == 'a'
	&& nextChar[2] == 'n');
	nextChar += 3;
	
	if(*nextChar == ':')
	{
		++nextChar;

		const U64 significandBits = parseHexUnsignedInt(nextChar,state,FloatComponents::maxSignificand);
		resultComponents.bits.significand = typename FloatComponents::Bits(significandBits);
	}
	else
	{
		// If the NaN's significand isn't specified, just set the top bit.
		resultComponents.bits.significand = typename FloatComponents::Bits(1) << (FloatComponents::numSignificandBits-1);
	}
	
	return resultComponents.value;
}


// Parses a floating-point infinity. Does not advance nextChar.
// Assumes it will only be called for input that's already been accepted by the lexer as a literal infinity.
template<typename Float>
Float parseInfinity(const char* nextChar)
{
	// Floating point infinite is represented by max exponent with a zero significand.
	typedef typename Floats::FloatComponents<Float> FloatComponents;
	FloatComponents resultComponents;
	resultComponents.bits.sign = parseSign(nextChar) ? 1 : 0;
	resultComponents.bits.exponent = FloatComponents::maxExponentBits;
	resultComponents.bits.significand = 0;
	return resultComponents.value;
}

// Parses a decimal floating point literal, advancing nextChar past the parsed characters.
// Assumes it will only be called for input that's already been accepted by the lexer as a decimal float literal.
template<typename Float>
Float parseDecimalFloat(const char*& nextChar,ParseState& state)
{
	// Use David Gay's strtod to parse a floating point number.
	const char* firstChar = nextChar;
	F64 f64 = parseDecimalF64(nextChar,const_cast<char**>(&nextChar));
	if(nextChar == firstChar)
	{
		++nextChar;
		Errors::fatalf("strtod failed to parse number accepted by lexer");
	}
	if(f64 < std::numeric_limits<Float>::lowest() || f64 > std::numeric_limits<Float>::max())
	{
		parseErrorf(state,firstChar,"float literal is too large");
	}

	return (Float)f64;
}

// Parses a hexadecimal floating point literal, advancing nextChar past the parsed characters.
// Assumes it will only be called for input that's already been accepted by the lexer as a hexadecimal float literal.
template<typename Float>
Float parseHexFloat(const char*& nextChar,ParseState& state)
{
	typedef typename Floats::FloatComponents<Float> FloatComponents;
	typedef typename FloatComponents::Bits FloatBits;
	FloatComponents resultComponents;

	resultComponents.bits.sign = parseSign(nextChar) ? 1 : 0;

	assert(nextChar[0] == '0' && (nextChar[1] == 'x' || nextChar[1] == 'X'));
	nextChar += 2;

	// Parse hexits into a 64-bit fixed point number, keeping track of where the point is in exponent.
	U64 fixedPoI64 = 0;
	bool hasSeenPoint = false;
	I64 exponent = 0;
	while(true)
	{
		U8 hexit = 0;
		if(tryParseHexit(nextChar,hexit))
		{
			// Once there are too many hexits to accumulate in the 64-bit fixed point number, ignore
			// the hexits, but continue to update exponent so we get an accurate but imprecise number.
			if(fixedPoI64 <= (UINT64_MAX - 15) / 16)
			{
				assert(fixedPoI64 * 16 + hexit >= fixedPoI64);
				fixedPoI64 = fixedPoI64 * 16 + hexit;
				exponent -= hasSeenPoint ? 4 : 0;
			}
			else
			{
				exponent += hasSeenPoint ? 0 : 4;
			}
		}
		else if(*nextChar == '.')
		{
			assert(!hasSeenPoint);
			hasSeenPoint = true;
			++nextChar;
		}
		else { break; }
	}

	// Parse an optional exponent.
	if(*nextChar == 'p' || *nextChar == 'P')
	{
		++nextChar;
		const bool isExponentNegative = parseSign(nextChar);
		const U64 userExponent = parseDecimalUnsignedInt(nextChar,state,U64(-INT32_MIN),"float literal exponent");
		exponent = isExponentNegative ? exponent - userExponent : exponent + userExponent;
	}

	if(!fixedPoI64)
	{
		// If both the integer and fractional part are zero, just return zero.
		resultComponents.bits.exponent = 0;
		resultComponents.bits.significand = 0;
	}
	else
	{
		// Shift the fixed point number's most significant set bit into the MSB.
		const Uptr leadingZeroes = Platform::countLeadingZeroes(fixedPoI64);
		fixedPoI64 <<= leadingZeroes;
		exponent += 64;
		exponent -= leadingZeroes;
		
		const I64 exponentWithImplicitLeadingOne = exponent - 1;
		if(exponentWithImplicitLeadingOne > FloatComponents::maxNormalExponent)
		{
			// If the number is out of range, produce an error and return infinity.
			resultComponents.bits.exponent = FloatComponents::maxExponentBits;
			resultComponents.bits.significand = FloatComponents::maxSignificand;
			parseErrorf(state,state.nextToken,"hexadecimal float literal is out of range");
		}
		else if(exponentWithImplicitLeadingOne < FloatComponents::minNormalExponent)
		{
			// Denormals are encoded as if their exponent is minNormalExponent, but
			// with the significand shifted down to include the leading 1 that is implicit for
			// normal numbers, and with the encoded exponent = 0.
			const Uptr denormalShift = FloatComponents::minNormalExponent - exponent;
			fixedPoI64 = denormalShift >= 64 ? 0 : (fixedPoI64 >> denormalShift);
			resultComponents.bits.exponent = 0;
			resultComponents.bits.significand = FloatBits(fixedPoI64 >> (64 - FloatComponents::numSignificandBits));
		}
		else
		{
			// Encode a normal floating point value.
			assert(exponentWithImplicitLeadingOne >= FloatComponents::minNormalExponent);
			assert(exponentWithImplicitLeadingOne <= FloatComponents::maxNormalExponent);
			resultComponents.bits.exponent = FloatBits(exponentWithImplicitLeadingOne + FloatComponents::exponentBias);
			resultComponents.bits.significand = FloatBits(fixedPoI64 >> (63 - FloatComponents::numSignificandBits));
		}
	}

	return resultComponents.value;
}

// Tries to parse an numeric literal token as an integer, advancing state.nextToken.
// Returns true if it matched a token.
template<typename UnsignedInt>
bool tryParseInt(ParseState& state,UnsignedInt& outUnsignedInt,I64 minSignedValue,U64 maxUnsignedValue)
{
	bool isNegative = false;
	U64 u64 = 0;

	const char* nextChar = state.string + state.nextToken->begin;
	switch(state.nextToken->type)
	{
	case t_decimalInt:
		isNegative = parseSign(nextChar);
		u64 = parseDecimalUnsignedInt(nextChar,state,isNegative ? U64(-minSignedValue) : maxUnsignedValue,"int literal");
		break;
	case t_hexInt:
		isNegative = parseSign(nextChar);
		u64 = parseHexUnsignedInt(nextChar,state,isNegative ? U64(-minSignedValue) : maxUnsignedValue);
		break;
	default:
		return false;
	};

	outUnsignedInt = isNegative ? UnsignedInt(-I64(u64)) : UnsignedInt(u64);
		
	++state.nextToken;
	assert(nextChar <= state.string + state.nextToken->begin);

	return true;
}

// Tries to parse a numeric literal literal token as a float, advancing state.nextToken.
// Returns true if it matched a token.
template<typename Float>
bool tryParseFloat(ParseState& state,Float& outFloat)
{
	const char* nextChar = state.string + state.nextToken->begin;
	switch(state.nextToken->type)
	{
	case t_decimalInt:
	case t_decimalFloat: outFloat = parseDecimalFloat<Float>(nextChar,state); break;
	case t_hexInt:
	case t_hexFloat: outFloat = parseHexFloat<Float>(nextChar,state); break;
	case t_floatNaN: outFloat = parseNaN<Float>(nextChar,state); break;
	case t_floatInf: outFloat = parseInfinity<Float>(nextChar); break;
	default:
		parseErrorf(state,state.nextToken,"expected float literal");
		return false;
	};

	++state.nextToken;
	assert(nextChar <= state.string + state.nextToken->begin);

	return true;
}

namespace WAST
{
	bool tryParseI32(ParseState& state,U32& outI32)
	{
		return tryParseInt<U32>(state,outI32,INT32_MIN,UINT32_MAX);
	}

	bool tryParseI64(ParseState& state,U64& outI64)
	{
		return tryParseInt<U64>(state,outI64,INT64_MIN,UINT64_MAX);
	}
	
	U8 parseI8(ParseState& state)
	{
		U32 result;
		if(!tryParseInt<U32>(state,result,INT8_MIN,UINT8_MAX))
		{
			parseErrorf(state,state.nextToken,"expected i8 literal");
			throw RecoverParseException();
		}
		return U8(result);
	}

	U32 parseI32(ParseState& state)
	{
		U32 result;
		if(!tryParseI32(state,result))
		{
			parseErrorf(state,state.nextToken,"expected i32 literal");
			throw RecoverParseException();
		}
		return result;
	}

	U64 parseI64(ParseState& state)
	{
		U64 result;
		if(!tryParseI64(state,result))
		{
			parseErrorf(state,state.nextToken,"expected i64 literal");
			throw RecoverParseException();
		}
		return result;
	}

	F32 parseF32(ParseState& state)
	{
		F32 result;
		if(!tryParseFloat(state,result))
		{
			parseErrorf(state,state.nextToken,"expected f32 literal");
			throw RecoverParseException();
		}
		return result;
	}

	F64 parseF64(ParseState& state)
	{
		F64 result;
		if(!tryParseFloat(state,result))
		{
			parseErrorf(state,state.nextToken,"expected f64 literal");
			throw RecoverParseException();
		}
		return result;
	}
}