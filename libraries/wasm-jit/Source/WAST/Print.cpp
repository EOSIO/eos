#include "Inline/BasicTypes.h"
#include "Inline/Floats.h"
#include "WAST.h"
#include "IR/Module.h"
#include "IR/Operators.h"

#include <map>

using namespace IR;

namespace WAST
{
	#define INDENT_STRING "\xE0\x01"
	#define DEDENT_STRING "\xE0\x02"

	char nibbleToHexChar(U8 value) { return value < 10 ? ('0' + value) : 'a' + value - 10; }

	std::string escapeString(const char* string,Uptr numChars)
	{
		std::string result;
		for(Uptr charIndex = 0;charIndex < numChars;++charIndex)
		{
			auto c = string[charIndex];
			if(c == '\\') { result += "\\\\"; }
			else if(c == '\"') { result += "\\\""; }
			else if(c == '\n') { result += "\\n"; }
			else if(c < 0x20 || c > 0x7e)
			{
				result += '\\';
				result += nibbleToHexChar((c & 0xf0) >> 4);
				result += nibbleToHexChar((c & 0x0f) >> 0);
			}
			else { result += c; }
		}
		return result;
	}

	std::string expandIndentation(std::string&& inString,U8 spacesPerIndentLevel = 2)
	{
		std::string paddedInput = std::move(inString);
		paddedInput += '\0';

		std::string result;
		const char* next = paddedInput.data();
		const char* end = paddedInput.data() + paddedInput.size() - 1;
		Uptr indentDepth = 0;
		while(next < end)
		{
			// Absorb INDENT_STRING and DEDENT_STRING, but keep track of the indentation depth,
			// and insert a proportional number of spaces following newlines. 
			if(*(U16*)next == *(U16*)INDENT_STRING) { ++indentDepth; next += 2; }
			else if(*(U16*)next == *(U16*)DEDENT_STRING) { errorUnless(indentDepth > 0); --indentDepth; next += 2; }
			else if(*next == '\n')
			{
				result += '\n';
				result.insert(result.end(),indentDepth*2,' ');
				++next;
			}
			else { result += *next++; }
		}
		return result;
	}
	
	struct ScopedTagPrinter
	{
		ScopedTagPrinter(std::string& inString,const char* tag): string(inString)
		{
			string += "(";
			string += tag;
			string += INDENT_STRING;
		}

		~ScopedTagPrinter()
		{
			string += DEDENT_STRING ")";
		}

	private:
		std::string& string;
	};

	void print(std::string& string,ValueType type) { string += asString(type); }
	void print(std::string& string,ResultType type) { string += asString(type); }
	
	void print(std::string& string,const SizeConstraints& size)
	{
		string += std::to_string(size.min);
		if(size.max != UINT64_MAX) { string += ' '; string += std::to_string(size.max); }
	}

	void print(std::string& string,const FunctionType* functionType)
	{
		// Print the function parameters.
		if(functionType->parameters.size())
		{
			ScopedTagPrinter paramTag(string,"param");
			for(Uptr parameterIndex = 0;parameterIndex < functionType->parameters.size();++parameterIndex)
			{
				string += ' ';
				print(string,functionType->parameters[parameterIndex]);
			}
		}

		// Print the function return type.
		if(functionType->ret != ResultType::none)
		{
			string += ' ';
			ScopedTagPrinter resultTag(string,"result");
			string += ' ';
			print(string,functionType->ret);
		}
	}

	void print(std::string& string,const TableType& type)
	{
		print(string,type.size);
		string += " anyfunc";
	}

	void print(std::string& string,const MemoryType& type)
	{
		print(string,type.size);
	}

	void print(std::string& string,GlobalType type)
	{
		if(type.isMutable) { string += "(mut "; }
		print(string,type.valueType);
		if(type.isMutable) { string += ")"; }
	}

	void printControlSignature(std::string& string,ResultType resultType)
	{
		if(resultType != ResultType::none)
		{
			string += " (result ";
			print(string,resultType);
			string += ')';
		}
	}

	struct NameScope
	{
		NameScope(const char inSigil): sigil(inSigil) { nameToCountMap[""] = 0; }
		
		void map(std::string& name)
		{
			auto mapIt = nameToCountMap.find(name);
			if(mapIt == nameToCountMap.end()) { nameToCountMap[name] = 1; }
			else { name = name + std::to_string(mapIt->second++); }
			name = sigil + name;
		}

	private:

		char sigil;
		std::map<std::string,Uptr> nameToCountMap;
	};

	struct ModulePrintContext
	{
		const Module& module;
		std::string& string;
		
		DisassemblyNames names;
		
		ModulePrintContext(const Module& inModule,std::string& inString)
		: module(inModule), string(inString)
		{
			// Start with the names from the module's user name section, but make sure they are unique, and add the "$" sigil.
			IR::getDisassemblyNames(module,names);
			NameScope globalNameScope('$');
			for(auto& name : names.types) { globalNameScope.map(name); }
			for(auto& name : names.tables) { globalNameScope.map(name); }
			for(auto& name : names.memories) { globalNameScope.map(name); }
			for(auto& name : names.globals) { globalNameScope.map(name); }
			for(auto& function : names.functions)
			{
				globalNameScope.map(function.name);

				NameScope localNameScope('$');
				for(auto& name : function.locals) { localNameScope.map(name); }
			}
		}

		void printModule();
		
		void printInitializerExpression(const InitializerExpression& expression)
		{
			switch(expression.type)
			{
			case InitializerExpression::Type::i32_const: string += "(i32.const " + std::to_string(expression.i32) + ')'; break;
			case InitializerExpression::Type::i64_const: string += "(i64.const " + std::to_string(expression.i64) + ')'; break;
			case InitializerExpression::Type::f32_const: string += "(f32.const " + Floats::asString(expression.f32) + ')'; break;
			case InitializerExpression::Type::f64_const: string += "(f64.const " + Floats::asString(expression.f64) + ')'; break;
			case InitializerExpression::Type::get_global: string += "(get_global " + names.globals[expression.globalIndex] + ')'; break;
			default: Errors::unreachable();
			};
		}
	};
	
	struct FunctionPrintContext
	{
		typedef void Result;

		ModulePrintContext& moduleContext;
		const Module& module;
		const FunctionDef& functionDef;
		const FunctionType* functionType;
		std::string& string;

		const std::vector<std::string>& localNames;
		NameScope labelNameScope;

		FunctionPrintContext(ModulePrintContext& inModuleContext,Uptr functionDefIndex)
		: moduleContext(inModuleContext)
		, module(inModuleContext.module)
		, functionDef(inModuleContext.module.functions.defs[functionDefIndex])
		, functionType(inModuleContext.module.types[functionDef.type.index])
		, string(inModuleContext.string)
		, localNames(inModuleContext.names.functions[module.functions.imports.size() + functionDefIndex].locals)
		, labelNameScope('$')
		{}

		void printFunctionBody();
		
		void unknown(Opcode)
		{
			Errors::unreachable();
		}
		void block(ControlStructureImm imm)
		{
			string += "\nblock";
			pushControlStack(ControlContext::Type::block,"block");
			printControlSignature(string,imm.resultType);
		}
		void loop(ControlStructureImm imm)
		{
			string += "\nloop";
			pushControlStack(ControlContext::Type::loop,"loop");
			printControlSignature(string,imm.resultType);
		}
		void if_(ControlStructureImm imm)
		{
			string += "\nif";
			pushControlStack(ControlContext::Type::ifThen,"if");
			printControlSignature(string,imm.resultType);
		}
		void else_(NoImm imm)
		{
			string += DEDENT_STRING;
			controlStack.back().type = ControlContext::Type::ifElse;
			string += "\nelse" INDENT_STRING;
		}
		void end(NoImm)
		{
			string += DEDENT_STRING;
			if(controlStack.back().type != ControlContext::Type::function) { string += "\nend ;; "; string += controlStack.back().labelId; }
			controlStack.pop_back();
		}
		
		void return_(NoImm)
		{
			string += "\nreturn";
			enterUnreachable();
		}

		void br(BranchImm imm)
		{
			string += "\nbr " + getBranchTargetId(imm.targetDepth);
			enterUnreachable();
		}
		void br_table(BranchTableImm imm)
		{
			string += "\nbr_table" INDENT_STRING;
			enum { numTargetsPerLine = 16 };
			WAVM_ASSERT_THROW(imm.branchTableIndex < functionDef.branchTables.size());
			const std::vector<U32>& targetDepths = functionDef.branchTables[imm.branchTableIndex];
			for(Uptr targetIndex = 0;targetIndex < targetDepths.size();++targetIndex)
			{
				if(targetIndex % numTargetsPerLine == 0) { string += '\n'; }
				else { string += ' '; }
				string += getBranchTargetId(targetDepths[targetIndex]);
			}
			string += '\n';
			string += getBranchTargetId(imm.defaultTargetDepth);
			string += " ;; default" DEDENT_STRING;

			enterUnreachable();
		}
		void br_if(BranchImm imm)
		{
			string += "\nbr_if " + getBranchTargetId(imm.targetDepth);
		}

		void unreachable(NoImm) { string += "\nunreachable"; enterUnreachable(); }
		void drop(NoImm) { string += "\ndrop"; }

		void select(NoImm)
		{
			string += "\nselect";
		}

		void get_local(GetOrSetVariableImm<false> imm)
		{
			string += "\nget_local " + localNames[imm.variableIndex];
		}
		void set_local(GetOrSetVariableImm<false> imm)
		{
			string += "\nset_local " + localNames[imm.variableIndex];
		}
		void tee_local(GetOrSetVariableImm<false> imm)
		{
			string += "\ntee_local " + localNames[imm.variableIndex];
		}
		
		void get_global(GetOrSetVariableImm<true> imm)
		{
			string += "\nget_global " + moduleContext.names.globals[imm.variableIndex];
		}
		void set_global(GetOrSetVariableImm<true> imm)
		{
			string += "\nset_global " + moduleContext.names.globals[imm.variableIndex];
		}

		void call(CallImm imm)
		{
			string += "\ncall " + moduleContext.names.functions[imm.functionIndex].name;
		}
		void call_indirect(CallIndirectImm imm)
		{
			string += "\ncall_indirect " + moduleContext.names.types[imm.type.index];
		}

		void printImm(NoImm) {}
		void printImm(MemoryImm) {}

		void printImm(LiteralImm<I32> imm) { string += ' '; string += std::to_string(imm.value); }
		void printImm(LiteralImm<I64> imm) { string += ' '; string += std::to_string(imm.value); }
		void printImm(LiteralImm<F32> imm) { string += ' '; string += Floats::asString(imm.value); }
		void printImm(LiteralImm<F64> imm) { string += ' '; string += Floats::asString(imm.value); }

		template<Uptr naturalAlignmentLog2>
		void printImm(LoadOrStoreImm<naturalAlignmentLog2> imm)
		{
			if(imm.offset != 0)
			{
				string += " offset=";
				string += std::to_string(imm.offset);
			}
			if(imm.alignmentLog2 != naturalAlignmentLog2)
			{
				string += " align=";
				string += std::to_string(1 << imm.alignmentLog2);
			}
		}

		#if ENABLE_SIMD_PROTOTYPE
		
		void printImm(LiteralImm<V128> imm) { string += ' '; string += asString(imm.value); }

		template<Uptr numLanes>
		void printImm(LaneIndexImm<numLanes> imm)
		{
			string += ' ';
			string += imm.laneIndex;
		}
		
		template<Uptr numLanes>
		void printImm(ShuffleImm<numLanes> imm)
		{
			string += " (";
			for(Uptr laneIndex = 0;laneIndex < numLanes;++laneIndex)
			{
				if(laneIndex != 0) { string += ' '; }
				string += std::to_string(imm.laneIndices[laneIndex]);
			}
			string += ')';
		}
		#endif

		#if ENABLE_THREADING_PROTOTYPE
		void printImm(LaunchThreadImm) {}
		
		template<Uptr naturalAlignmentLog2>
		void printImm(AtomicLoadOrStoreImm<naturalAlignmentLog2> imm)
		{
			if(imm.offset != 0)
			{
				string += " offset=";
				string += std::to_string(imm.offset);
			}
			WAVM_ASSERT_THROW(imm.alignmentLog2 == naturalAlignmentLog2);
		}
		#endif

		#define PRINT_OP(opcode,name,nameString,Imm,printOperands) \
			void name(Imm imm) \
			{ \
				string += "\n" nameString; \
				printImm(imm); \
			}
		ENUM_NONCONTROL_NONPARAMETRIC_OPERATORS(PRINT_OP)
		#undef VALIDATE_OP

	private:
		
		struct ControlContext
		{
			enum class Type : U8
			{
				function,
				block,
				ifThen,
				ifElse,
				loop
			};
			Type type;
			std::string labelId;
		};

		std::vector<ControlContext> controlStack;

		std::string getBranchTargetId(Uptr depth)
		{
			const ControlContext& controlContext = controlStack[controlStack.size() - depth - 1];
			return controlContext.type == ControlContext::Type::function ? std::to_string(depth) : controlContext.labelId;
		}

		void pushControlStack(ControlContext::Type type,const char* labelIdBase)
		{
			std::string labelId;
			if(type != ControlContext::Type::function)
			{
				labelId = labelIdBase;
				labelNameScope.map(labelId);
				string += ' ';
				string += labelId;
			}
			controlStack.push_back({type,labelId});
			string += INDENT_STRING;
		}

		void enterUnreachable()
		{}
	};

	template<typename Type>
	void printImportType(std::string& string,const Module& module,Type type)
	{
		print(string,type);
	}
	template<>
	void printImportType<IndexedFunctionType>(std::string& string,const Module& module,IndexedFunctionType type)
	{
		print(string,module.types[type.index]);
	}

	template<typename Type>
	void printImport(std::string& string,const Module& module,const Import<Type>& import,Uptr importIndex,const char* name,const char* typeTag)
	{
		string += '\n';
		ScopedTagPrinter importTag(string,"import");
		string += " \"";
		string += escapeString(import.moduleName.c_str(),import.moduleName.length());
		string += "\" \"";
		string += escapeString(import.exportName.c_str(),import.exportName.length());
		string += "\" (";
		string += typeTag;
		string += ' ';
		string += name;
		string += ' ';
		printImportType(string,module,import.type);
		string += ')';
	}

	void ModulePrintContext::printModule()
	{
		ScopedTagPrinter moduleTag(string,"module");
		
		// Print the types.
		for(Uptr typeIndex = 0;typeIndex < module.types.size();++typeIndex)
		{
			string += '\n';
			ScopedTagPrinter typeTag(string,"type");
			string += ' ';
			string += names.types[typeIndex];
			string += " (func ";
			print(string,module.types[typeIndex]);
			string += ')';
		}

		// Print the module imports.
		for(Uptr importIndex = 0;importIndex < module.functions.imports.size();++importIndex)
		{
			printImport(string,module,module.functions.imports[importIndex],importIndex,names.functions[importIndex].name.c_str(),"func");
		}
		for(Uptr importIndex = 0;importIndex < module.tables.imports.size();++importIndex)
		{
			printImport(string,module,module.tables.imports[importIndex],importIndex,names.tables[importIndex].c_str(),"table");
		}
		for(Uptr importIndex = 0;importIndex < module.memories.imports.size();++importIndex)
		{
			printImport(string,module,module.memories.imports[importIndex],importIndex,names.memories[importIndex].c_str(),"memory");
		}
		for(Uptr importIndex = 0;importIndex < module.globals.imports.size();++importIndex)
		{
			printImport(string,module,module.globals.imports[importIndex],importIndex,names.globals[importIndex].c_str(),"global");
		}
		// Print the module exports.
		for(auto export_ : module.exports)
		{
			string += '\n';
			ScopedTagPrinter exportTag(string,"export");
			string += " \"";
			string += escapeString(export_.name.c_str(),export_.name.length());
			string += "\" (";
			switch(export_.kind)
			{
			case ObjectKind::function: string += "func " + names.functions[export_.index].name; break;
			case ObjectKind::table: string += "table " + names.tables[export_.index]; break;
			case ObjectKind::memory: string += "memory " + names.memories[export_.index]; break;
			case ObjectKind::global: string += "global " + names.globals[export_.index]; break;
			default: Errors::unreachable();
			};
			string += ')';
		}

		// Print the module memory definitions.
		for(Uptr memoryDefIndex = 0;memoryDefIndex < module.memories.defs.size();++memoryDefIndex)
		{
			const MemoryDef& memoryDef = module.memories.defs[memoryDefIndex];
			string += '\n';
			ScopedTagPrinter memoryTag(string,"memory");
			string += ' ';
			string += names.memories[module.memories.imports.size() + memoryDefIndex];
			string += ' ';
			print(string,memoryDef.type);
		}
		
		// Print the module table definitions and elem segments.
		for(Uptr tableDefIndex = 0;tableDefIndex < module.tables.defs.size();++tableDefIndex)
		{
			const TableDef& tableDef = module.tables.defs[tableDefIndex];
			string += '\n';
			ScopedTagPrinter memoryTag(string,"table");
			string += ' ';
			string += names.tables[module.tables.imports.size() + tableDefIndex];
			string += ' ';
			print(string,tableDef.type);
		}
		
		// Print the module global definitions.
		for(Uptr globalDefIndex = 0;globalDefIndex < module.globals.defs.size();++globalDefIndex)
		{
			const GlobalDef& globalDef = module.globals.defs[globalDefIndex];
			string += '\n';
			ScopedTagPrinter memoryTag(string,"global");
			string += ' ';
			string += names.globals[module.globals.imports.size() + globalDefIndex];
			string += ' ';
			print(string,globalDef.type);
			string += ' ';
			printInitializerExpression(globalDef.initializer);
		}
		
		// Print the data and table segment definitions.
		for(auto tableSegment : module.tableSegments)
		{
			string += '\n';
			ScopedTagPrinter dataTag(string,"elem");
			string += ' ';
			string += names.tables[tableSegment.tableIndex];
			string += ' ';
			printInitializerExpression(tableSegment.baseOffset);
			enum { numElemsPerLine = 8 };
			for(Uptr elementIndex = 0;elementIndex < tableSegment.indices.size();++elementIndex)
			{
				if(elementIndex % numElemsPerLine == 0) { string += '\n'; }
				else { string += ' '; }
				string += names.functions[tableSegment.indices[elementIndex]].name;
			}
		}
		for(auto dataSegment : module.dataSegments)
		{
			string += '\n';
			ScopedTagPrinter dataTag(string,"data");
			string += ' ';
			string += names.memories[dataSegment.memoryIndex];
			string += ' ';
			printInitializerExpression(dataSegment.baseOffset);
			enum { numBytesPerLine = 64 };
			for(Uptr offset = 0;offset < dataSegment.data.size();offset += numBytesPerLine)
			{
				string += "\n\"";
				string += escapeString((const char*)dataSegment.data.data() + offset,std::min(dataSegment.data.size() - offset,(Uptr)numBytesPerLine));
				string += "\"";
			}
		}

		for(Uptr functionDefIndex = 0;functionDefIndex < module.functions.defs.size();++functionDefIndex)
		{
			const Uptr functionIndex = module.functions.imports.size() + functionDefIndex;
			const FunctionDef& functionDef = module.functions.defs[functionDefIndex];
			const FunctionType* functionType = module.types[functionDef.type.index];
			FunctionPrintContext functionContext(*this,functionDefIndex);
		
			string += "\n\n";
			ScopedTagPrinter funcTag(string,"func");

			string += ' ';
			string += names.functions[functionIndex].name;
			
			// Print the function parameters.
			if(functionType->parameters.size())
			{
				for(Uptr parameterIndex = 0;parameterIndex < functionType->parameters.size();++parameterIndex)
				{
					string += '\n';
					ScopedTagPrinter paramTag(string,"param");
					string += ' ';
					string += functionContext.localNames[parameterIndex];
					string += ' ';
					print(string,functionType->parameters[parameterIndex]);
				}
			}

			// Print the function return type.
			if(functionType->ret != ResultType::none)
			{
				string += '\n';
				ScopedTagPrinter resultTag(string,"result");
				string += ' ';
				print(string,functionType->ret);
			}

			// Print the function's locals.
			for(Uptr localIndex = 0;localIndex < functionDef.nonParameterLocalTypes.size();++localIndex)
			{
				string += '\n';
				ScopedTagPrinter localTag(string,"local");
				string += ' ';
				string += functionContext.localNames[functionType->parameters.size() + localIndex];
				string += ' ';
				print(string,functionDef.nonParameterLocalTypes[localIndex]);
			}

			functionContext.printFunctionBody();
		}

		// Print user sections (other than the name section).
		for(const auto& userSection : module.userSections)
		{
			if(userSection.name != "name")
			{
				string += '\n';
				ScopedTagPrinter dataTag(string,"user_section");
				string += " \"";
				string += escapeString(userSection.name.c_str(),userSection.name.length());
				string += "\" ";
				enum { numBytesPerLine = 64 };
				for(Uptr offset = 0;offset < userSection.data.size();offset += numBytesPerLine)
				{
					string += "\n\"";
					string += escapeString((const char*)userSection.data.data() + offset,std::min(userSection.data.size() - offset,(Uptr)numBytesPerLine));
					string += "\"";
				}
			}
		}
	}

	void FunctionPrintContext::printFunctionBody()
	{
		//string += "(";
		pushControlStack(ControlContext::Type::function,nullptr);
		string += DEDENT_STRING;

		OperatorDecoderStream decoder(functionDef.code);
		while(decoder && controlStack.size())
		{
			decoder.decodeOp(*this);
		};

		string += INDENT_STRING "\n";
	}

	std::string print(const Module& module)
	{
		std::string string;
		ModulePrintContext context(module,string);
		context.printModule();
		return expandIndentation(std::move(string));
	}
}
