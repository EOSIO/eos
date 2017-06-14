#include "Inline/BasicTypes.h"
#include "WAST.h"
#include "TestScript.h"
#include "Lexer.h"
#include "IR/Module.h"
#include "IR/Validate.h"
#include "Runtime/TaggedValue.h"
#include "Parse.h"
#include "Inline/Serialization.h"
#include "WASM/WASM.h"

using namespace WAST;
using namespace IR;

static Runtime::Value parseConstExpression(ParseState& state)
{
	Runtime::Value result;
	parseParenthesized(state,[&]
	{
		switch(state.nextToken->type)
		{
		case t_i32_const:
		{
			++state.nextToken;
			result = parseI32(state);
			break;
		}
		case t_i64_const:
		{
			++state.nextToken;
			result = parseI64(state);
			break;
		}
		case t_f32_const:
		{
			++state.nextToken;
			result = parseF32(state);
			break;
		}
		case t_f64_const:
		{
			++state.nextToken;
			result = parseF64(state);
			break;
		}
		default:
			parseErrorf(state,state.nextToken,"expected const expression");
			throw RecoverParseException();
		};
	});
	return result;
}

static std::string parseOptionalNameAsString(ParseState& state)
{
	Name name;
	return tryParseName(state,name) ? name.getString() : std::string();
}

static void parseTestScriptModule(ParseState& state,IR::Module& outModule,std::vector<UnresolvedError>& outErrors,std::string& outInternalModuleName)
{
	ModuleParseState moduleState(state.string,state.lineInfo,outErrors,state.nextToken,outModule);
	
	outInternalModuleName = parseOptionalNameAsString(moduleState);
	
	if(moduleState.nextToken->type == t_quote || moduleState.nextToken->type == t_binary)
	{
		// Parse a quoted module: (module quote|binary "...")
		const Token* quoteToken = moduleState.nextToken;
		++moduleState.nextToken;

		std::string moduleQuotedString;
		if(!tryParseString(moduleState,moduleQuotedString))
		{
			parseErrorf(moduleState,moduleState.nextToken,"expected string");
		}
		else
		{
			while(tryParseString(moduleState,moduleQuotedString)) {};
		}

		if(quoteToken->type == t_quote)
		{
			moduleQuotedString = "(module " + moduleQuotedString + ")";

			std::vector<Error> quotedErrors;
			parseModule(moduleQuotedString.c_str(),moduleQuotedString.size(),outModule,quotedErrors);
			for(auto&& error : quotedErrors)
			{
				outErrors.emplace_back(quoteToken->begin,std::move(error.message));
			}
		}
		else
		{
			try
			{
				Serialization::MemoryInputStream wasmInputStream((const U8*)moduleQuotedString.data(),moduleQuotedString.size());
				WASM::serialize(wasmInputStream,outModule);
			}
			catch(Serialization::FatalSerializationException exception)
			{
				parseErrorf(moduleState,quoteToken,"error deserializing binary module: %s",exception.message.c_str());
			}
			catch(ValidationException exception)
			{
				parseErrorf(moduleState,quoteToken,"error validating binary module: %s",exception.message.c_str());
			}
		}
	}
	else
	{
		try
		{
			parseModuleBody(moduleState);
		}
		catch(RecoverParseException)
		{
			outErrors.insert(outErrors.end(),std::make_move_iterator(moduleState.errors.begin()),std::make_move_iterator(moduleState.errors.end()));
			state.nextToken = moduleState.nextToken;
			throw RecoverParseException();
		}
	}

	state.nextToken = moduleState.nextToken;
}

static Action* parseAction(ParseState& state)
{
	Action* result = nullptr;
	parseParenthesized(state,[&]
	{
		TextFileLocus locus = calcLocusFromOffset(state.string,state.lineInfo,state.nextToken->begin);

		switch(state.nextToken->type)
		{
		case t_get:
		{
			++state.nextToken;

			std::string nameString = parseOptionalNameAsString(state);
			std::string exportName = parseUTF8String(state);
			
			result = new GetAction(std::move(locus),std::move(nameString),std::move(exportName));
			break;
		}
		case t_invoke:
		{
			++state.nextToken;

			std::string nameString = parseOptionalNameAsString(state);
			std::string exportName = parseUTF8String(state);

			std::vector<Runtime::Value> arguments;
			while(state.nextToken->type == t_leftParenthesis)
			{
				arguments.push_back(parseConstExpression(state));
			};

			result = new InvokeAction(std::move(locus),std::move(nameString),std::move(exportName),std::move(arguments));
			break;
		}
		case t_module:
		{
			++state.nextToken;

			std::string internalModuleName;
			Module* module = new Module;
			parseTestScriptModule(state,*module,state.errors,internalModuleName);

			result = new ModuleAction(std::move(locus),std::move(internalModuleName),module);
			break;
		}
		default:
			parseErrorf(state,state.nextToken,"expected 'get' or 'invoke'");
			throw RecoverParseException();
		};
	});

	return result;
}

static Command* parseCommand(ParseState& state)
{
	Command* result = nullptr;

	if(state.nextToken[0].type == t_leftParenthesis
	&& (state.nextToken[1].type == t_module
		|| state.nextToken[1].type == t_invoke
		|| state.nextToken[1].type == t_get))
	{
		Action* action = parseAction(state);
		TextFileLocus locus = action->locus;
		result = new ActionCommand(std::move(locus),action);
	}
	else
	{
		parseParenthesized(state,[&]
		{
			TextFileLocus locus = calcLocusFromOffset(state.string,state.lineInfo,state.nextToken->begin);

			switch(state.nextToken->type)
			{
			case t_register:
			{
				++state.nextToken;

				std::string moduleName = parseUTF8String(state);
				std::string nameString = parseOptionalNameAsString(state);

				result = new RegisterCommand(std::move(locus),std::move(moduleName),std::move(nameString));
				break;
			}
			case t_assert_return:
			{
				++state.nextToken;

				Action* action = parseAction(state);
				Runtime::Result expectedReturn = state.nextToken->type == t_leftParenthesis ? parseConstExpression(state) : Runtime::Result();
				result = new AssertReturnCommand(std::move(locus),action,expectedReturn);
				break;
			}
			case t_assert_return_canonical_nan:
			case t_assert_return_arithmetic_nan:
			{
				const Command::Type commandType = state.nextToken->type == t_assert_return_canonical_nan
					? Command::assert_return_canonical_nan
					: Command::assert_return_arithmetic_nan;
				++state.nextToken;

				Action* action = parseAction(state);
				result = new AssertReturnNaNCommand(commandType,std::move(locus),action);
				break;
			}
			case t_assert_exhaustion:
			case t_assert_trap:
			{
				++state.nextToken;

				Action* action = parseAction(state);

				std::string expectedErrorMessage;
				if(!tryParseString(state,expectedErrorMessage))
				{
					parseErrorf(state,state.nextToken,"expected string literal");
					throw RecoverParseException();
				}
				Runtime::Exception::Cause expectedCause = Runtime::Exception::Cause::unknown;
				if(!strcmp(expectedErrorMessage.c_str(),"out of bounds memory access")) { expectedCause = Runtime::Exception::Cause::accessViolation; }
				else if(!strcmp(expectedErrorMessage.c_str(),"call stack exhausted")) { expectedCause = Runtime::Exception::Cause::stackOverflow; }
				else if(!strcmp(expectedErrorMessage.c_str(),"integer overflow")) { expectedCause = Runtime::Exception::Cause::integerDivideByZeroOrIntegerOverflow; }
				else if(!strcmp(expectedErrorMessage.c_str(),"integer divide by zero")) { expectedCause = Runtime::Exception::Cause::integerDivideByZeroOrIntegerOverflow; }
				else if(!strcmp(expectedErrorMessage.c_str(),"invalid conversion to integer")) { expectedCause = Runtime::Exception::Cause::invalidFloatOperation; }
				else if(!strcmp(expectedErrorMessage.c_str(),"unreachable executed")) { expectedCause = Runtime::Exception::Cause::reachedUnreachable; }
				else if(!strcmp(expectedErrorMessage.c_str(),"unreachable")) { expectedCause = Runtime::Exception::Cause::reachedUnreachable; }
				else if(!strcmp(expectedErrorMessage.c_str(),"indirect call signature mismatch")) { expectedCause = Runtime::Exception::Cause::indirectCallSignatureMismatch; }
				else if(!strcmp(expectedErrorMessage.c_str(),"indirect call")) { expectedCause = Runtime::Exception::Cause::indirectCallSignatureMismatch; }
				else if(!strcmp(expectedErrorMessage.c_str(),"undefined element")) { expectedCause = Runtime::Exception::Cause::undefinedTableElement; }
				else if(!strcmp(expectedErrorMessage.c_str(),"undefined")) { expectedCause = Runtime::Exception::Cause::undefinedTableElement; }
				else if(!strcmp(expectedErrorMessage.c_str(),"uninitialized")) { expectedCause = Runtime::Exception::Cause::undefinedTableElement; }
				else if(!strcmp(expectedErrorMessage.c_str(),"uninitialized element")) { expectedCause = Runtime::Exception::Cause::undefinedTableElement; }

				result = new AssertTrapCommand(std::move(locus),action,expectedCause);
				break;
			}
			case t_assert_unlinkable:
			{
				++state.nextToken;

				if(state.nextToken[0].type != t_leftParenthesis || state.nextToken[1].type != t_module)
				{
					parseErrorf(state,state.nextToken,"expected module");
					throw RecoverParseException();
				}

				ModuleAction* moduleAction = (ModuleAction*)parseAction(state);
						
				std::string expectedErrorMessage;
				if(!tryParseString(state,expectedErrorMessage))
				{
					parseErrorf(state,state.nextToken,"expected string literal");
					throw RecoverParseException();
				}

				result = new AssertUnlinkableCommand(std::move(locus),moduleAction);
				break;
			}
			case t_assert_invalid:
			case t_assert_malformed:
			{
				const Command::Type commandType = state.nextToken->type == t_assert_invalid
					? Command::assert_invalid
					: Command::assert_malformed;
				++state.nextToken;

				std::string internalModuleName;
				Module module;
				std::vector<UnresolvedError> moduleErrors;

				parseParenthesized(state,[&]
				{
					require(state,t_module);

					parseTestScriptModule(state,module,moduleErrors,internalModuleName);
				});

				std::string expectedErrorMessage;
				if(!tryParseString(state,expectedErrorMessage))
				{
					parseErrorf(state,state.nextToken,"expected string literal");
					throw RecoverParseException();
				}

				result = new AssertInvalidOrMalformedCommand(commandType,std::move(locus),moduleErrors.size() != 0);
				break;
			};
			default:
				parseErrorf(state,state.nextToken,"unknown script command");
				throw RecoverParseException();
			};
		});
	}
	
	return result;
}

namespace WAST
{
	void parseTestCommands(const char* string,Uptr stringLength,std::vector<std::unique_ptr<Command>>& outTestCommands,std::vector<Error>& outErrors)
	{
		// Lex the input string.
		LineInfo* lineInfo = nullptr;
		std::vector<UnresolvedError> unresolvedErrors;
		Token* tokens = lex(string,stringLength,lineInfo);
		ParseState state(string,lineInfo,unresolvedErrors,tokens);

		try
		{
			// (command)*<eof>
			while(state.nextToken->type == t_leftParenthesis)
			{
				outTestCommands.emplace_back(parseCommand(state));
			};
			require(state,t_eof);
		}
		catch(RecoverParseException) {}
		catch(FatalParseException) {}
		
		// Resolve line information for any errors, and write them to outErrors.
		for(auto& unresolvedError : unresolvedErrors)
		{
			TextFileLocus locus = calcLocusFromOffset(state.string,lineInfo,unresolvedError.charOffset);
			outErrors.push_back({std::move(locus),std::move(unresolvedError.message)});
		}

		// Free the tokens and line info.
		freeTokens(tokens);
		freeLineInfo(lineInfo);
	}
}