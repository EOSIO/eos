#include "Inline/BasicTypes.h"
#include "Inline/UTF8.h"
#include "WAST.h"
#include "Lexer.h"
#include "IR/Module.h"
#include "Parse.h"

#include <cstdarg>
#include <cstdio>

#define XXH_FORCE_NATIVE_FORMAT 1
#define XXH_PRIVATE_API
#include "../ThirdParty/xxhash/xxhash.h"

using namespace IR;

namespace WAST
{
	void findClosingParenthesis(ParseState& state,const Token* openingParenthesisToken)
	{
		// Skip over tokens until the ')' closing the current parentheses nesting depth is found.
		Uptr depth = 1;
		while(depth > 0)
		{
			switch(state.nextToken->type)
			{
			default:
				++state.nextToken;
				break;
			case t_leftParenthesis:
				++state.nextToken;
				++depth;
				break;
			case t_rightParenthesis:
				++state.nextToken;
				--depth;
				break;
			case t_eof:
				parseErrorf(state,openingParenthesisToken,"reached end of input while trying to find closing parenthesis");
				throw FatalParseException();
			}
		}
	}

	void parseErrorf(ParseState& state,Uptr charOffset,const char* messageFormat,va_list messageArguments)
	{
		// Format the message.
		char messageBuffer[1024];
		int numPrintedChars = std::vsnprintf(messageBuffer,sizeof(messageBuffer),messageFormat,messageArguments);
		if(numPrintedChars >= 1023 || numPrintedChars < 0) { Errors::unreachable(); }
		messageBuffer[numPrintedChars] = 0;

		// Add the error to the state's error list.
		state.errors.emplace_back(charOffset,messageBuffer);
	}
	void parseErrorf(ParseState& state,Uptr charOffset,const char* messageFormat,...)
	{
		va_list messageArguments;
		va_start(messageArguments,messageFormat);
		parseErrorf(state,charOffset,messageFormat,messageArguments);
		va_end(messageArguments);
	}
	void parseErrorf(ParseState& state,const char* nextChar,const char* messageFormat,...)
	{
		va_list messageArguments;
		va_start(messageArguments,messageFormat);
		parseErrorf(state,nextChar - state.string,messageFormat,messageArguments);
		va_end(messageArguments);
	}
	void parseErrorf(ParseState& state,const Token* nextToken,const char* messageFormat,...)
	{
		va_list messageArguments;
		va_start(messageArguments,messageFormat);
		parseErrorf(state,nextToken->begin,messageFormat,messageArguments);
		va_end(messageArguments);
	}

	void require(ParseState& state,TokenType type)
	{
		if(state.nextToken->type != type)
		{
			parseErrorf(state,state.nextToken,"expected %s",describeToken(type));
			throw RecoverParseException();
		}
		++state.nextToken;
	}
	
	bool tryParseValueType(ParseState& state,ValueType& outValueType)
	{
		switch(state.nextToken->type)
		{
		case t_i32: ++state.nextToken; outValueType = ValueType::i32; return true;
		case t_i64: ++state.nextToken; outValueType = ValueType::i64; return true;
		case t_f32: ++state.nextToken; outValueType = ValueType::f32; return true;
		case t_f64: ++state.nextToken; outValueType = ValueType::f64; return true;
		#if ENABLE_SIMD_PROTOTYPE
		case t_v128: ++state.nextToken; outValueType = ValueType::v128; return true;
		#endif
		default:
			outValueType = ValueType::any;
			return false;
		};
	}
	
	bool tryParseResultType(ParseState& state,ResultType& outResultType)
	{
		return tryParseValueType(state,*(ValueType*)&outResultType);
	}

	ValueType parseValueType(ParseState& state)
	{
		ValueType result;
		if(!tryParseValueType(state,result))
		{
			parseErrorf(state,state.nextToken,"expected value type");
			throw RecoverParseException();
		}
		return result;
	}

	const FunctionType* parseFunctionType(ModuleParseState& state,NameToIndexMap& outLocalNameToIndexMap,std::vector<std::string>& outLocalDisassemblyNames)
	{
		std::vector<ValueType> parameters;
		ResultType ret = ResultType::none;

		// Parse the function parameters.
		while(tryParseParenthesizedTagged(state,t_param,[&]
		{
			Name parameterName;
			if(tryParseName(state,parameterName))
			{
				// (param <name> <type>)
				bindName(state,outLocalNameToIndexMap,parameterName,parameters.size());
				parameters.push_back(parseValueType(state));
				outLocalDisassemblyNames.push_back(parameterName.getString());
			}
			else
			{
				// (param <type>*)
				ValueType parameterType;
				while(tryParseValueType(state,parameterType))
				{
					parameters.push_back(parameterType);
					outLocalDisassemblyNames.push_back(std::string());
				};
			}
		}));

		// Parse <= 1 result type: (result <value type>*)*
		while(state.nextToken[0].type == t_leftParenthesis
		&& state.nextToken[1].type == t_result)
		{
			parseParenthesized(state,[&]
			{
				require(state,t_result);

				ResultType resultElementType;
				const Token* elementToken = state.nextToken;
				while(tryParseResultType(state,resultElementType))
				{
					if(ret != ResultType::none) { parseErrorf(state,elementToken,"function type cannot have more than 1 result element"); }
					ret = resultElementType;
				};
			});
		};

		return FunctionType::get(ret,parameters);
	}
	
	UnresolvedFunctionType parseFunctionTypeRefAndOrDecl(ModuleParseState& state,NameToIndexMap& outLocalNameToIndexMap,std::vector<std::string>& outLocalDisassemblyNames)
	{
		// Parse an optional function type reference.
		Reference functionTypeRef;
		if(state.nextToken[0].type == t_leftParenthesis
		&& state.nextToken[1].type == t_type)
		{
			parseParenthesized(state,[&]
			{
				require(state,t_type);
				if(!tryParseNameOrIndexRef(state,functionTypeRef))
				{
					parseErrorf(state,state.nextToken,"expected type name or index");
					throw RecoverParseException();
				}
			});
		}

		// Parse the explicit function parameters and result type.
		const FunctionType* explicitFunctionType = parseFunctionType(state,outLocalNameToIndexMap,outLocalDisassemblyNames);

		UnresolvedFunctionType result;
		result.reference = functionTypeRef;
		result.explicitType = explicitFunctionType;
		return result;
	}

	IndexedFunctionType resolveFunctionType(ModuleParseState& state,const UnresolvedFunctionType& unresolvedType)
	{
		if(!unresolvedType.reference)
		{
			return getUniqueFunctionTypeIndex(state,unresolvedType.explicitType);
		}
		else
		{
			// Resolve the referenced type.
			const U32 referencedFunctionTypeIndex = resolveRef(state,state.typeNameToIndexMap,state.module.types.size(),unresolvedType.reference);

			// Validate that if the function definition has both a type reference and explicit parameter/result type declarations, they match.
			const bool hasExplicitParametersOrResultType = unresolvedType.explicitType != FunctionType::get();
			if(hasExplicitParametersOrResultType)
			{
				if(referencedFunctionTypeIndex != UINT32_MAX
				&& state.module.types[referencedFunctionTypeIndex] != unresolvedType.explicitType)
				{
					parseErrorf(state,unresolvedType.reference.token,"referenced function type (%s) does not match declared parameters and results (%s)",
						asString(state.module.types[referencedFunctionTypeIndex]).c_str(),
						asString(unresolvedType.explicitType).c_str()
						);
				}
			}

			return {referencedFunctionTypeIndex};
		}
	}

	IndexedFunctionType getUniqueFunctionTypeIndex(ModuleParseState& state,const FunctionType* functionType)
	{
		// If this type is not in the module's type table yet, add it.
		auto functionTypeToIndexMapIt = state.functionTypeToIndexMap.find(functionType);
		if(functionTypeToIndexMapIt != state.functionTypeToIndexMap.end())
		{
			return IndexedFunctionType {functionTypeToIndexMapIt->second};
		}
		else
		{
			const Uptr functionTypeIndex = state.module.types.size();
			state.module.types.push_back(functionType);
			state.disassemblyNames.types.push_back(std::string());
			errorUnless(functionTypeIndex < UINT32_MAX);
			state.functionTypeToIndexMap.emplace(functionType,(U32)functionTypeIndex);
			return IndexedFunctionType {(U32)functionTypeIndex};
		}
	}

	U32 Name::calcHash(const char* begin,U32 numChars)
	{
		// Use xxHash32 to hash names. xxHash64 is theoretically faster for long strings on 64-bit machines,
		// but I did not find it to be faster for typical name lengths.
		return XXH32(begin,numChars,0);
	}

	bool tryParseName(ParseState& state,Name& outName)
	{
		if(state.nextToken->type != t_name) { return false; }

		// Find the first non-name character.
		const char* firstChar = state.string + state.nextToken->begin;;
		const char* nextChar = firstChar;
		assert(*nextChar == '$');
		++nextChar;
		while(true)
		{
			const char c = *nextChar;
			if((c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9')
			|| c=='_' || c=='\'' || c=='+' || c=='-' || c=='*' || c=='/' || c=='\\' || c=='^' || c=='~' || c=='='
			|| c=='<' || c=='>' || c=='!' || c=='?' || c=='@' || c=='#' || c=='$' || c=='%' || c=='&' || c=='|'
			|| c==':' || c=='`' || c=='.')
			{
				++nextChar;
			}
			else
			{
				break;
			}
		};

		assert(U32(nextChar - state.string) > state.nextToken->begin + 1);
		++state.nextToken;
		assert(U32(nextChar - state.string) <= state.nextToken->begin);
		assert(U32(nextChar - firstChar) <= UINT32_MAX);
		outName = Name(firstChar,U32(nextChar - firstChar));
		return true;
	}

	bool tryParseNameOrIndexRef(ParseState& state,Reference& outRef)
	{
		outRef.token = state.nextToken;
		if(tryParseName(state,outRef.name)) { outRef.type = Reference::Type::name; return true; }
		else if(tryParseI32(state,outRef.index)) { outRef.type = Reference::Type::index; return true; }
		return false;
	}

	U32 parseAndResolveNameOrIndexRef(ParseState& state,const NameToIndexMap& nameToIndexMap,Uptr maxIndex,const char* context)
	{
		Reference ref;

		if(strcmp(context, "type") == 0     //limits this block strictly to call_indirect
		&& state.nextToken[0].type == t_leftParenthesis
		&& state.nextToken[1].type == t_type)
		{
			parseParenthesized(state,[&]
			{
				require(state,t_type);
				if(!tryParseNameOrIndexRef(state,ref))
				{
					parseErrorf(state,state.nextToken,"expected type name or index");
					throw RecoverParseException();
				}
			});
		}

		if(ref.type == Reference::Type::invalid && !tryParseNameOrIndexRef(state,ref))
		{
			parseErrorf(state,state.nextToken,"expected %s name or index",context);
			throw RecoverParseException();
		}
		return resolveRef(state,nameToIndexMap,maxIndex,ref);
	}

	void bindName(ParseState& state,NameToIndexMap& nameToIndexMap,const Name& name,Uptr index)
	{
		errorUnless(index <= UINT32_MAX);

		if(name)
		{
			auto mapIt = nameToIndexMap.find(name);
			if(mapIt != nameToIndexMap.end())
			{
				const TextFileLocus previousDefinitionLocus = calcLocusFromOffset(state.string,	state.lineInfo,mapIt->first.getCharOffset(state.string));
				parseErrorf(state,name.getCharOffset(state.string),"redefinition of name defined at %s",previousDefinitionLocus.describe().c_str());
			}
			nameToIndexMap.emplace(name,U32(index));
		}
	}

	U32 resolveRef(ParseState& state,const NameToIndexMap& nameToIndexMap,Uptr maxIndex,const Reference& ref)
	{
		switch(ref.type)
		{
		case Reference::Type::index:
		{
			if(ref.index >= maxIndex)
			{
				parseErrorf(state,ref.token,"invalid index");
				return UINT32_MAX;
			}
			return ref.index;
		}
		case Reference::Type::name:
		{
			auto nameToIndexMapIt = nameToIndexMap.find(ref.name);
			if(nameToIndexMapIt == nameToIndexMap.end())
			{
				parseErrorf(state,ref.token,"unknown name");
				return UINT32_MAX;
			}
			else
			{
				return nameToIndexMapIt->second;
			}
		}
		default: Errors::unreachable();
		};
	}
	
	bool tryParseHexit(const char*& nextChar,U8& outValue)
	{
		if(*nextChar >= '0' && *nextChar <= '9') { outValue = *nextChar - '0'; }
		else if(*nextChar >= 'a' && *nextChar <= 'f') { outValue = *nextChar - 'a' + 10; }
		else if(*nextChar >= 'A' && *nextChar <= 'F') { outValue = *nextChar - 'A' + 10; }
		else
		{
			outValue = 0;
			return false;
		}
		++nextChar;
		return true;
	}
	
	static void parseCharEscapeCode(const char*& nextChar,ParseState& state,std::string& outString)
	{
		U8 firstNibble;
		if(tryParseHexit(nextChar,firstNibble))
		{
			// Parse an 8-bit literal from two hexits.
			U8 secondNibble;
			if(!tryParseHexit(nextChar,secondNibble)) { parseErrorf(state,nextChar,"expected hexit"); }
			outString += char(firstNibble * 16 + secondNibble);
		}
		else
		{
			switch(*nextChar)
			{
			case 't':	outString += '\t'; ++nextChar; break;
			case 'n':	outString += '\n'; ++nextChar; break;
			case 'r':	outString += '\r'; ++nextChar; break;
			case '\"':	outString += '\"'; ++nextChar; break;
			case '\'':	outString += '\''; ++nextChar; break;
			case '\\':	outString += '\\'; ++nextChar; break;
			case 'u':
			{
				// \u{...} - Unicode codepoint from hexadecimal number
				if(nextChar[1] != '{') { parseErrorf(state,nextChar,"expected '{'"); }
				nextChar += 2;
				
				// Parse the hexadecimal number.
				const char* firstHexit = nextChar;
				U32 codepoint = 0;
				U8 hexit = 0;
				while(tryParseHexit(nextChar,hexit))
				{
					if(codepoint > (UINT32_MAX - hexit) / 16)
					{
						codepoint = UINT32_MAX;
						while(tryParseHexit(nextChar,hexit)) {};
						break;
					}
					assert(codepoint * 16 + hexit >= codepoint);
					codepoint = codepoint * 16 + hexit;
				}

				// Check that it denotes a valid Unicode codepoint.
				if((codepoint >= 0xD800 && codepoint <= 0xDFFF)
				|| codepoint >= 0x110000)
				{
					parseErrorf(state,firstHexit,"invalid Unicode codepoint");
					codepoint = 0x1F642;
				}

				// Encode the codepoint as UTF-8.
				UTF8::encodeCodepoint(codepoint,outString);

				if(*nextChar != '}') { parseErrorf(state,nextChar,"expected '}'"); }
				++nextChar;
				break;
			}
			default:
				outString += '\\';
				++nextChar;
				parseErrorf(state,nextChar,"invalid escape code");
				break;
			}
		}
	}

	bool tryParseString(ParseState& state,std::string& outString)
	{
		if(state.nextToken->type != t_string)
		{
			return false;
		}

		// Parse a string literal; the lexer has already rejected unterminated strings,
		// so this just needs to copy the characters and evaluate escape codes.
		const char* nextChar = state.string + state.nextToken->begin;
		assert(*nextChar == '\"');
		++nextChar;
		while(true)
		{
			switch(*nextChar)
			{
			case '\\':
			{
				++nextChar;
				parseCharEscapeCode(nextChar,state,outString);
				break;
			}
			case '\"':
				++state.nextToken;
				assert(state.string + state.nextToken->begin > nextChar);
				return true;
			default:
				outString += *nextChar++;
				break;
			};
		};
	}

	std::string parseUTF8String(ParseState& state)
	{
		const Token* stringToken = state.nextToken;
		std::string result;
		if(!tryParseString(state,result))
		{
			parseErrorf(state,stringToken,"expected string literal");
			throw RecoverParseException();
		}

		// Check that the string is a valid UTF-8 encoding.
		const U8* endChar = (const U8*)result.data() + result.size();
		const U8* nextChar = UTF8::validateString((const U8*)result.data(),endChar);
		if(nextChar != endChar)
		{
			const Uptr charOffset = stringToken->begin + (nextChar - (const U8*)result.data()) + 1;
			parseErrorf(state,charOffset,"invalid UTF-8 encoding");
		}

		return result;
	}
}