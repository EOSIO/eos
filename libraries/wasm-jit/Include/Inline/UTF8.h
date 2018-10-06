#pragma once

#include "BasicTypes.h"
#include "Errors.h"

namespace UTF8
{
	inline const U8* validateString(const U8* nextChar,const U8* endChar)
	{
		// Check that the string is a valid UTF-8 encoding.
		// The valid ranges are taken from table 3-7 in the Unicode Standard 9.0:
		// "Well-Formed UTF-8 Byte Sequences"
		while(nextChar != endChar)
		{
			if(*nextChar < 0x80) { ++nextChar; }
			else if(*nextChar >= 0xc2 && *nextChar <= 0xdf)
			{
				if(nextChar + 1 >= endChar
				|| nextChar[1] < 0x80 || nextChar[1] > 0xbf) { break; }
				nextChar += 2;
			}
			else if(*nextChar == 0xe0)
			{
				if(nextChar + 2 >= endChar
				|| nextChar[1] < 0xa0 || nextChar[1] > 0xbf
				|| nextChar[2] < 0x80 || nextChar[2] > 0xbf) { break; }
				nextChar += 3;
			}
			else if(*nextChar == 0xed)
			{
				if(nextChar + 2 >= endChar
				|| nextChar[1] < 0xa0 || nextChar[1] > 0x9f
				|| nextChar[2] < 0x80 || nextChar[2] > 0xbf) { break; }
				nextChar += 3;
			}
			else if(*nextChar >= 0xe1 && *nextChar <= 0xef)
			{
				if(nextChar + 2 >= endChar
				|| nextChar[1] < 0x80 || nextChar[1] > 0xbf
				|| nextChar[2] < 0x80 || nextChar[2] > 0xbf) { break; }
				nextChar += 3;
			}
			else if(*nextChar == 0xf0)
			{
				if(nextChar + 3 >= endChar
				|| nextChar[1] < 0x90 || nextChar[1] > 0xbf
				|| nextChar[2] < 0x80 || nextChar[2] > 0xbf
				|| nextChar[3] < 0x80 || nextChar[3] > 0xbf) { break; }
				nextChar += 4;
			}
			else if(*nextChar >= 0xf1 && *nextChar <= 0xf3)
			{
				if(nextChar + 3 >= endChar
				|| nextChar[1] < 0x80 || nextChar[1] > 0xbf
				|| nextChar[2] < 0x80 || nextChar[2] > 0xbf
				|| nextChar[3] < 0x80 || nextChar[3] > 0xbf) { break; }
				nextChar += 4;
			}
			else if(*nextChar == 0xf4)
			{
				if(nextChar + 3 >= endChar
				|| nextChar[1] < 0x80 || nextChar[1] > 0x8f
				|| nextChar[2] < 0x80 || nextChar[2] > 0xbf
				|| nextChar[3] < 0x80 || nextChar[3] > 0xbf) { break; }
				nextChar += 4;
			}
			else { break; }
		}
		return nextChar;
	}

	template<typename String>
	inline void encodeCodepoint(U32 codepoint,String& outString)
	{
		if(codepoint < 0x80)
		{
			outString += char(codepoint);
		}
		else if(codepoint < 0x800)
		{
			outString += char((codepoint >> 6) & 0x1F) | 0xC0;
			outString += char((codepoint & 0x3F) | 0x80);
		}
		else if(codepoint < 0x10000)
		{
			outString += char((codepoint >> 12) & 0x0F) | 0xE0;
			outString += char((codepoint >> 6) & 0x3F) | 0x80;
			outString += char((codepoint & 0x3F) | 0x80);
		}
		else
		{
			WAVM_ASSERT_THROW(codepoint < 0x200000);
			outString += char((codepoint >> 18) & 0x07) | 0xF0;
			outString += char((codepoint >> 12) & 0x3F) | 0x80;
			outString += char((codepoint >> 6) & 0x3F) | 0x80;
			outString += char((codepoint & 0x3F) | 0x80);
		}
	}
}