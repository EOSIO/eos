#include "Inline/BasicTypes.h"
#include "Inline/Timing.h"
#include "WAST.h"
#include "Lexer.h"
#include "IR/Module.h"
#include "IR/Validate.h"
#include "Parse.h"

using namespace WAST;
using namespace IR;

static bool tryParseSizeConstraints(ParseState& state,U64 maxMax,SizeConstraints& outSizeConstraints)
{
	outSizeConstraints.min = 0;
	outSizeConstraints.max = UINT64_MAX;

	// Parse a minimum.
	if(!tryParseI64(state,outSizeConstraints.min))
	{
		return false;
	}
	else
	{
		// Parse an optional maximum.
		if(!tryParseI64(state,outSizeConstraints.max)) { outSizeConstraints.max = UINT64_MAX; }
		else
		{
			// Validate that the maximum size is within the limit, and that the size contraints is not disjoint.
			if(outSizeConstraints.max > maxMax)
			{
				parseErrorf(state,state.nextToken-1,"maximum size exceeds limit (%u>%u)",outSizeConstraints.max,maxMax);
				outSizeConstraints.max = maxMax;
			}
			else if(outSizeConstraints.max < outSizeConstraints.min)
			{
				parseErrorf(state,state.nextToken-1,"maximum size is less than minimum size (%u<%u)",outSizeConstraints.max,outSizeConstraints.min);
				outSizeConstraints.max = outSizeConstraints.min;
			}
		}

		return true;
	}
}

static SizeConstraints parseSizeConstraints(ParseState& state,U64 maxMax)
{
	SizeConstraints result;
	if(!tryParseSizeConstraints(state,maxMax,result))
	{
		parseErrorf(state,state.nextToken,"expected size constraints");
	}
	return result;
}

static GlobalType parseGlobalType(ParseState& state)
{
	GlobalType result;
	result.isMutable = tryParseParenthesizedTagged(state,t_mut,[&]
	{
		result.valueType = parseValueType(state);
	});
	if(!result.isMutable)
	{
		result.valueType = parseValueType(state);
	}
	return result;
}

static InitializerExpression parseInitializerExpression(ModuleParseState& state)
{
	InitializerExpression result;
	parseParenthesized(state,[&]
	{
		switch(state.nextToken->type)
		{
		case t_i32_const: { ++state.nextToken; result = (I32)parseI32(state); break; }
		case t_i64_const: { ++state.nextToken; result = (I64)parseI64(state); break; }
		case t_f32_const: { ++state.nextToken; result = parseF32(state); break; }
		case t_f64_const: { ++state.nextToken; result = parseF64(state); break; }
		case t_get_global:
		{
			++state.nextToken;
			result.type = InitializerExpression::Type::get_global;
			result.globalIndex = parseAndResolveNameOrIndexRef(
				state,
				state.globalNameToIndexMap,
				state.module.globals.size(),
				"global"
				);
			break;
		}
		default:
			parseErrorf(state,state.nextToken,"expected initializer expression");
			result.type = InitializerExpression::Type::error;
			throw RecoverParseException();
		};
	});

	return result;
}

static void errorIfFollowsDefinitions(ModuleParseState& state)
{
	if(state.module.functions.defs.size()
	|| state.module.tables.defs.size()
	|| state.module.memories.defs.size()
	|| state.module.globals.defs.size())
	{
		parseErrorf(state,state.nextToken,"import declarations must precede all definitions");
	}
}

template<typename Def,typename Type,typename DisassemblyName>
static void createImport(
	ParseState& state,Name name,std::string&& moduleName,std::string&& exportName,
	NameToIndexMap& nameToIndexMap,IndexSpace<Def,Type>& indexSpace,std::vector<DisassemblyName>& disassemblyNameArray,
	Type type)
{
	bindName(state,nameToIndexMap,name,indexSpace.size());
	disassemblyNameArray.push_back({name.getString()});
	indexSpace.imports.push_back({type,std::move(moduleName),std::move(exportName)});
}

static bool parseOptionalSharedDeclaration(ModuleParseState& state)
{
	if(ENABLE_THREADING_PROTOTYPE && state.nextToken->type == t_shared) { ++state.nextToken; return true; }
	else { return false; }
}

static void parseImport(ModuleParseState& state)
{
	errorIfFollowsDefinitions(state);

	require(state,t_import);

	std::string moduleName = parseUTF8String(state);
	std::string exportName = parseUTF8String(state);

	parseParenthesized(state,[&]
	{
		// Parse the import kind.
		const Token* importKindToken = state.nextToken;
		switch(importKindToken->type)
		{
		case t_func:
		case t_table:
		case t_memory:
		case t_global:
			++state.nextToken;
			break;
		default:
			parseErrorf(state,state.nextToken,"invalid import type");
			throw RecoverParseException();
		}
		
		// Parse an optional internal name for the import.
		Name name;
		tryParseName(state,name);

		// Parse the import type and create the import in the appropriate name/index spaces.
		switch(importKindToken->type)
		{
		case t_func:
		{
			NameToIndexMap localNameToIndexMap;
			std::vector<std::string> localDissassemblyNames;
			const IndexedFunctionType functionType = parseFunctionTypeRef(state,localNameToIndexMap,localDissassemblyNames);
			createImport(state,name,std::move(moduleName),std::move(exportName),
				state.functionNameToIndexMap,state.module.functions,state.disassemblyNames.functions,
				functionType);
			state.disassemblyNames.functions.back().locals = localDissassemblyNames;
			break;
		}
		case t_table:
		{
			const SizeConstraints sizeConstraints = parseSizeConstraints(state,UINT32_MAX);
			const TableElementType elementType = TableElementType::anyfunc;
			require(state,t_anyfunc);
			const bool isShared = parseOptionalSharedDeclaration(state);
			createImport(state,name,std::move(moduleName),std::move(exportName),
				state.tableNameToIndexMap,state.module.tables,state.disassemblyNames.tables,
				{elementType,isShared,sizeConstraints});
			break;
		}
		case t_memory:
		{
			const SizeConstraints sizeConstraints = parseSizeConstraints(state,IR::maxMemoryPages);
			const bool isShared = parseOptionalSharedDeclaration(state);
			createImport(state,name,std::move(moduleName),std::move(exportName),
				state.memoryNameToIndexMap,state.module.memories,state.disassemblyNames.memories,
				MemoryType{isShared,sizeConstraints});
			break;
		}
		case t_global:
		{
			const GlobalType globalType = parseGlobalType(state);
			createImport(state,name,std::move(moduleName),std::move(exportName),
				state.globalNameToIndexMap,state.module.globals,state.disassemblyNames.globals,
				globalType);
			break;
		}
		default: Errors::unreachable();
		};
	});
}

static void parseExport(ModuleParseState& state)
{
	require(state,t_export);

	const std::string exportName = parseUTF8String(state);

	parseParenthesized(state,[&]
	{
		ObjectKind exportKind;
		switch(state.nextToken->type)
		{
		case t_func: exportKind = ObjectKind::function; break;
		case t_table: exportKind = ObjectKind::table; break;
		case t_memory: exportKind = ObjectKind::memory; break;
		case t_global: exportKind = ObjectKind::global; break;
		default:
			parseErrorf(state,state.nextToken,"invalid export kind");
			throw RecoverParseException();
		};
		++state.nextToken;
	
		Reference exportRef;
		if(!tryParseNameOrIndexRef(state,exportRef))
		{
			parseErrorf(state,state.nextToken,"expected name or index");
			throw RecoverParseException();
		}

		const Uptr exportIndex = state.module.exports.size();
		state.module.exports.push_back({std::move(exportName),exportKind,0});

		state.postDeclarationCallbacks.push_back([=](ModuleParseState& state)
		{
			Uptr& exportedObjectIndex = state.module.exports[exportIndex].index;
			switch(exportKind)
			{
			case ObjectKind::function: exportedObjectIndex = resolveRef(state,state.functionNameToIndexMap,state.module.functions.size(),exportRef); break;
			case ObjectKind::table: exportedObjectIndex = resolveRef(state,state.tableNameToIndexMap,state.module.tables.size(),exportRef); break;
			case ObjectKind::memory: exportedObjectIndex = resolveRef(state,state.memoryNameToIndexMap,state.module.memories.size(),exportRef); break;
			case ObjectKind::global: exportedObjectIndex = resolveRef(state,state.globalNameToIndexMap,state.module.globals.size(),exportRef); break;
			default:
				Errors::unreachable();
			}
		});
	});
}

static void parseType(ModuleParseState& state)
{
	require(state,t_type);

	Name name;
	tryParseName(state,name);

	parseParenthesized(state,[&]
	{
		require(state,t_func);
		
		NameToIndexMap parameterNameToIndexMap;
		std::vector<std::string> localDisassemblyNames;
		const FunctionType* functionType = parseFunctionType(state,parameterNameToIndexMap,localDisassemblyNames);

		Uptr functionTypeIndex = state.module.types.size();
		state.module.types.push_back(functionType);
		errorUnless(functionTypeIndex < UINT32_MAX);
		state.functionTypeToIndexMap[functionType] = (U32)functionTypeIndex;

		bindName(state,state.typeNameToIndexMap,name,functionTypeIndex);
		state.disassemblyNames.types.push_back(name.getString());
	});
}

static void parseData(ModuleParseState& state)
{
	const Token* firstToken = state.nextToken;
	require(state,t_data);

	// Parse an optional memory name.
	Reference memoryRef;
	bool hasMemoryRef = tryParseNameOrIndexRef(state,memoryRef);

	// Parse an initializer expression for the base address of the data.
	const InitializerExpression baseAddress = parseInitializerExpression(state);

	// Parse a list of strings that contains the segment's data.
	std::string dataString;
	while(tryParseString(state,dataString)) {};
	
	// Create the data segment.
	std::vector<U8> dataVector((const U8*)dataString.data(),(const U8*)dataString.data() + dataString.size());
	const Uptr dataSegmentIndex = state.module.dataSegments.size();
	state.module.dataSegments.push_back({UINTPTR_MAX,baseAddress,std::move(dataVector)});

	// Enqueue a callback that is called after all declarations are parsed to resolve the memory to put the data segment in.
	state.postDeclarationCallbacks.push_back([=](ModuleParseState& state)
	{
		if(!state.module.memories.size())
		{
			parseErrorf(state,firstToken,"data segments aren't allowed in modules without any memory declarations");
		}
		else
		{
			state.module.dataSegments[dataSegmentIndex].memoryIndex =
				hasMemoryRef ? resolveRef(state,state.memoryNameToIndexMap,state.module.memories.size(),memoryRef) : 0;
		}
	});
}

static Uptr parseElemSegmentBody(ModuleParseState& state,Reference tableRef,InitializerExpression baseIndex,const Token* elemToken)
{
	// Allocate the elementReferences array on the heap so it doesn't need to be copied for the post-declaration callback.
	std::vector<Reference>* elementReferences = new std::vector<Reference>();
	
	Reference elementRef;
	while(tryParseNameOrIndexRef(state,elementRef))
	{
		elementReferences->push_back(elementRef);
	};
	
	// Create the table segment.
	const Uptr tableSegmentIndex = state.module.tableSegments.size();
	state.module.tableSegments.push_back({UINTPTR_MAX,baseIndex,std::vector<Uptr>()});

	// Enqueue a callback that is called after all declarations are parsed to resolve the table elements' references.
	state.postDeclarationCallbacks.push_back([tableRef,tableSegmentIndex,elementReferences,elemToken](ModuleParseState& state)
	{
		if(!state.module.tables.size())
		{
			parseErrorf(state,elemToken,"data segments aren't allowed in modules without any memory declarations");
		}
		else
		{
			TableSegment& tableSegment = state.module.tableSegments[tableSegmentIndex];
			tableSegment.tableIndex = tableRef ? resolveRef(state,state.tableNameToIndexMap,state.module.tables.size(),tableRef) : 0;

			tableSegment.indices.resize(elementReferences->size());
			for(Uptr elementIndex = 0;elementIndex < elementReferences->size();++elementIndex)
			{
				tableSegment.indices[elementIndex] = resolveRef(
					state,
					state.functionNameToIndexMap,
					state.module.functions.size(),
					(*elementReferences)[elementIndex]
					);
			}
		}
		
		// Free the elementReferences array that was allocated on the heap.
		delete elementReferences;
	});

	return elementReferences->size();
}

static void parseElem(ModuleParseState& state)
{
	const Token* elemToken = state.nextToken;
	require(state,t_elem);

	// Parse an optional table name.
	Reference tableRef;
	tryParseNameOrIndexRef(state,tableRef);

	// Parse an initializer expression for the base index of the elements.
	const InitializerExpression baseIndex = parseInitializerExpression(state);

	parseElemSegmentBody(state,tableRef,baseIndex,elemToken);
}


template<typename Def,typename Type,typename ParseImport,typename ParseDef,typename DisassemblyName>
static void parseObjectDefOrImport(
	ModuleParseState& state,
	NameToIndexMap& nameToIndexMap,
	IR::IndexSpace<Def,Type>& indexSpace,
	std::vector<DisassemblyName>& disassemblyNameArray,
	TokenType declarationTag,
	IR::ObjectKind kind,
	ParseImport parseImportFunc,
	ParseDef parseDefFunc)
{
	const Token* declarationTagToken = state.nextToken;
	require(state,declarationTag);

	Name name;
	tryParseName(state,name);
	
	// Handle inline export declarations.
	while(true)
	{
		const bool isExport = tryParseParenthesizedTagged(state,t_export,[&]
		{
			state.module.exports.push_back({parseUTF8String(state),kind,indexSpace.size()});
		});
		if(!isExport) { break; }
	};

	// Handle an inline import declaration.
	std::string importModuleName;
	std::string exportName;
	const bool isImport = tryParseParenthesizedTagged(state,t_import,[&]
	{
		errorIfFollowsDefinitions(state);

		importModuleName = parseUTF8String(state);
		exportName = parseUTF8String(state);
	});
	if(isImport)
	{
		Type importType = parseImportFunc(state);
		createImport(state,name,std::move(importModuleName),std::move(exportName),
			nameToIndexMap,indexSpace,disassemblyNameArray,
			importType);
	}
	else
	{
		Def def = parseDefFunc(state,declarationTagToken);
		bindName(state,nameToIndexMap,name,indexSpace.size());
		indexSpace.defs.push_back(std::move(def));
		disassemblyNameArray.push_back({name.getString()});
	}
}

static void parseFunc(ModuleParseState& state)
{
	parseObjectDefOrImport(state,state.functionNameToIndexMap,state.module.functions,state.disassemblyNames.functions,t_func,ObjectKind::function,
		[&](ModuleParseState& state)
		{
			NameToIndexMap localNameToIndexMap;
			std::vector<std::string> localDisassemblyNames;
			return parseFunctionTypeRef(state,localNameToIndexMap,localDisassemblyNames);
		},
		parseFunctionDef);
}

static void parseTable(ModuleParseState& state)
{
	parseObjectDefOrImport(state,state.tableNameToIndexMap,state.module.tables,state.disassemblyNames.tables,t_table,ObjectKind::table,
		// Parse a table import.
		[](ModuleParseState& state)
		{
			const SizeConstraints sizeConstraints = parseSizeConstraints(state,UINT32_MAX);
			const TableElementType elementType = TableElementType::anyfunc;
			require(state,t_anyfunc);
			const bool isShared = parseOptionalSharedDeclaration(state);
			return TableType {elementType,isShared,sizeConstraints};
		},
		// Parse a table definition.
		[](ModuleParseState& state,const Token*)
		{
			// Parse the table type.
			SizeConstraints sizeConstraints;
			const bool hasSizeConstraints = tryParseSizeConstraints(state,UINT32_MAX,sizeConstraints);
		
			const TableElementType elementType = TableElementType::anyfunc;
			require(state,t_anyfunc);

			// If we couldn't parse an explicit size constraints, the table definition must contain an table segment that implicitly defines the size.
			if(!hasSizeConstraints)
			{
				parseParenthesized(state,[&]
				{
					require(state,t_elem);

					const Uptr tableIndex = state.module.tables.size();
					errorUnless(tableIndex < UINT32_MAX);
					const Uptr numElements = parseElemSegmentBody(state,Reference(U32(tableIndex)),InitializerExpression((I32)0),state.nextToken-1);
					sizeConstraints.min = sizeConstraints.max = numElements;
				});
			}
			
			const bool isShared = parseOptionalSharedDeclaration(state);
			return TableDef {TableType(elementType,isShared,sizeConstraints)};
		});
}

static void parseMemory(ModuleParseState& state)
{
	parseObjectDefOrImport(state,state.memoryNameToIndexMap,state.module.memories,state.disassemblyNames.memories,t_memory,ObjectKind::memory,
		// Parse a memory import.
		[](ModuleParseState& state)
		{
			const SizeConstraints sizeConstraints = parseSizeConstraints(state,IR::maxMemoryPages);
			const bool isShared = parseOptionalSharedDeclaration(state);
			return MemoryType {isShared,sizeConstraints};
		},
		// Parse a memory definition
		[](ModuleParseState& state,const Token*)
		{
			SizeConstraints sizeConstraints;
			if(!tryParseSizeConstraints(state,IR::maxMemoryPages,sizeConstraints))
			{
				std::string dataString;

				parseParenthesized(state,[&]
				{
					require(state,t_data);
				
					while(tryParseString(state,dataString)) {};
				});

				std::vector<U8> dataVector((const U8*)dataString.data(),(const U8*)dataString.data() + dataString.size());
				sizeConstraints.min = sizeConstraints.max = (dataVector.size() + IR::numBytesPerPage - 1) / IR::numBytesPerPage;
				state.module.dataSegments.push_back({state.module.memories.size(),InitializerExpression(I32(0)),std::move(dataVector)});
			}

			const bool isShared = parseOptionalSharedDeclaration(state);
			return MemoryDef {MemoryType(isShared,sizeConstraints)};
		});
}

static void parseGlobal(ModuleParseState& state)
{
	parseObjectDefOrImport(state,state.globalNameToIndexMap,state.module.globals,state.disassemblyNames.globals,t_global,ObjectKind::global,
		// Parse a global import.
		parseGlobalType,
		// Parse a global definition
		[](ModuleParseState& state,const Token*)
		{
			const GlobalType globalType = parseGlobalType(state);
			const InitializerExpression initializerExpression = parseInitializerExpression(state);
			return GlobalDef {globalType,initializerExpression};
		});
}

static void parseStart(ModuleParseState& state)
{
	require(state,t_start);

	Reference functionRef;
	if(!tryParseNameOrIndexRef(state,functionRef))
	{
		parseErrorf(state,state.nextToken,"expected function name or index");
	}

	state.postDeclarationCallbacks.push_back([functionRef](ModuleParseState& state)
	{
		state.module.startFunctionIndex = resolveRef(state,state.functionNameToIndexMap,state.module.functions.size(),functionRef);
	});
}

static void parseDeclaration(ModuleParseState& state)
{
	parseParenthesized(state,[&]
	{
		switch(state.nextToken->type)
		{
		case t_import: parseImport(state); return true;
		case t_export: parseExport(state); return true;
		case t_global: parseGlobal(state); return true;
		case t_memory: parseMemory(state); return true;
		case t_table: parseTable(state); return true;
		case t_type: parseType(state); return true;
		case t_data: parseData(state); return true;
		case t_elem: parseElem(state); return true;
		case t_func: parseFunc(state); return true;
		case t_start: parseStart(state); return true;
		default:
			parseErrorf(state,state.nextToken,"unrecognized definition in module");
			throw RecoverParseException();
		};
	});
}

namespace WAST
{
	void parseModuleBody(ModuleParseState& state)
	{
		const Token* firstToken = state.nextToken;

		// Parse the module's declarations.
		while(state.nextToken->type != t_rightParenthesis)
		{
			parseDeclaration(state);
		};

		// Process the callbacks requested after all declarations have been parsed.
		if(!state.errors.size())
		{
			for(const auto& callback : state.postDeclarationCallbacks)
			{
				callback(state);
			}
		}
		
		// Validate the module's definitions (excluding function code, which is validated as it is parsed).
		if(!state.errors.size())
		{
			try
			{
				IR::validateDefinitions(state.module);
			}
			catch(ValidationException validationException)
			{
				parseErrorf(state,firstToken,"validation exception: %s",validationException.message.c_str());
			}
		}

		// Set the module's disassembly names.
		assert(state.module.functions.size() == state.disassemblyNames.functions.size());
		assert(state.module.tables.size() == state.disassemblyNames.tables.size());
		assert(state.module.memories.size() == state.disassemblyNames.memories.size());
		assert(state.module.globals.size() == state.disassemblyNames.globals.size());
		IR::setDisassemblyNames(state.module,state.disassemblyNames);
	}

	bool parseModule(const char* string,Uptr stringLength,IR::Module& outModule,std::vector<Error>& outErrors)
	{
		Timing::Timer timer;
		
		// Lex the string.
		LineInfo* lineInfo = nullptr;
		std::vector<UnresolvedError> unresolvedErrors;
		Token* tokens = lex(string,stringLength,lineInfo);
		ModuleParseState state(string,lineInfo,unresolvedErrors,tokens,outModule);

		try
		{
			// Parse (module ...)<eof>
			parseParenthesized(state,[&]
			{
				require(state,t_module);
				parseModuleBody(state);
			});
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

		Timing::logRatePerSecond("lexed and parsed WAST",timer,stringLength / 1024.0 / 1024.0,"MB");

		return outErrors.size() == 0;
	}
}