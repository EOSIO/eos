#include "IR/Module.h"
#include "IR/Operators.h"
#include "IR/OperatorPrinter.h"
#include "IR/Validate.h"
#include "Logging/Logging.h"
#include "Inline/Timing.h"

#include <set>

#define ENABLE_LOGGING 0

namespace IR
{
	#define VALIDATE_UNLESS(reason,comparison) \
		if(comparison) \
		{ \
			throw ValidationException(reason #comparison); \
		}

	#define VALIDATE_INDEX(index,arraySize) VALIDATE_UNLESS("invalid index: ",index>=arraySize)

	void validate(ValueType valueType)
	{
		if(valueType == ValueType::any || valueType > ValueType::max)
		{
			throw ValidationException("invalid value type (" + std::to_string((Uptr)valueType) + ")");
		}
	}

	void validate(ResultType returnType)
	{
		if(returnType > ResultType::max)
		{
			throw ValidationException("invalid return type (" + std::to_string((Uptr)returnType) + ")");
		}
	}

	void validate(ObjectKind kind)
	{
		if(kind > ObjectKind::max)
		{
			throw ValidationException("invalid external kind (" + std::to_string((Uptr)kind) + ")");
		}
	}

	void validate(SizeConstraints size,Uptr maxMax)
	{
		U64 max = size.max == UINT64_MAX ? maxMax : size.max;
		VALIDATE_UNLESS("disjoint size bounds: ",size.min>max);
		VALIDATE_UNLESS("maximum size exceeds limit: ",max>maxMax);
	}

	void validate(TableElementType type)
	{
		if(type != TableElementType::anyfunc) { throw ValidationException("invalid table element type (" + std::to_string((Uptr)type) + ")"); }
	}

	void validate(TableType type)
	{
		validate(type.elementType);
		validate(type.size,UINT32_MAX);
		if(ENABLE_THREADING_PROTOTYPE)
		{
			VALIDATE_UNLESS("shared tables must have a maximum size: ",type.isShared && type.size.max == UINT64_MAX);
		}
	}

	void validate(MemoryType type)
	{
		validate(type.size,IR::maxMemoryPages);
		if(ENABLE_THREADING_PROTOTYPE)
		{
			VALIDATE_UNLESS("shared tables must have a maximum size: ",type.isShared && type.size.max == UINT64_MAX);
		}
	}

	void validate(GlobalType type)
	{
		validate(type.valueType);
	}
	
	void validateImportKind(ObjectType importType,ObjectKind expectedKind)
	{
		if(importType.kind != expectedKind)
		{
			throw ValidationException("incorrect kind");
		}
	}

	template<typename Type>
	void validateType(Type expectedType,Type actualType,const char* context)
	{
		if(expectedType != actualType)
		{
			throw ValidationException(
				std::string("type mismatch: expected ") + asString(expectedType)
				+ " but got " + asString(actualType)
				+ " in " + context
				);
		}
	}

	void validateOperandType(ValueType expectedType,ValueType actualType,const char* context)
	{
		// Handle polymorphic values popped off the operand stack after unconditional branches.
		if(expectedType != actualType && expectedType != ValueType::any && actualType != ValueType::any)
		{
			throw ValidationException(
				std::string("type mismatch: expected ") + asString(expectedType)
				+ " but got " + asString(actualType)
				+ " in " + context + " operand"
				);
		}
	}

	ValueType validateGlobalIndex(const Module& module,Uptr globalIndex,bool mustBeMutable,bool mustBeImmutable,bool mustBeImport,const char* context)
	{
		VALIDATE_INDEX(globalIndex,module.globals.size());
		const GlobalType& globalType = module.globals.getType(globalIndex);
		if(mustBeMutable && !globalType.isMutable) { throw ValidationException("attempting to mutate immutable global"); }
		else if(mustBeImport && globalIndex >= module.globals.imports.size()) { throw ValidationException("global variable initializer expression may only access imported globals"); }
		else if(mustBeImmutable && globalType.isMutable) { throw ValidationException("global variable initializer expression may only access immutable globals"); }
		return globalType.valueType;
	}

	const FunctionType* validateFunctionIndex(const Module& module,Uptr functionIndex)
	{
		VALIDATE_INDEX(functionIndex,module.functions.size());
		return module.types[module.functions.getType(functionIndex).index];
	}
		
	void validateInitializer(const Module& module,const InitializerExpression& expression,ValueType expectedType,const char* context)
	{
		switch(expression.type)
		{
		case InitializerExpression::Type::i32_const: validateType(expectedType,ValueType::i32,context); break;
		case InitializerExpression::Type::i64_const: validateType(expectedType,ValueType::i64,context); break;
		case InitializerExpression::Type::f32_const: validateType(expectedType,ValueType::f32,context); break;
		case InitializerExpression::Type::f64_const: validateType(expectedType,ValueType::f64,context); break;
		case InitializerExpression::Type::get_global:
		{
			const ValueType globalValueType = validateGlobalIndex(module,expression.globalIndex,false,true,true,"initializer expression global index");
			validateType(expectedType,globalValueType,context);
			break;
		}
		default:
			throw ValidationException("invalid initializer expression");
		};
	}

	struct FunctionValidationContext
	{
		FunctionValidationContext(const Module& inModule,const FunctionDef& inFunctionDef)
		: module(inModule), functionDef(inFunctionDef), functionType(inModule.types[inFunctionDef.type.index])
		{
			// Initialize the local types.
			locals.reserve(functionType->parameters.size() + functionDef.nonParameterLocalTypes.size());
			locals = functionType->parameters;
			locals.insert(locals.end(),functionDef.nonParameterLocalTypes.begin(),functionDef.nonParameterLocalTypes.end());

			// Push the function context onto the control stack.
			pushControlStack(ControlContext::Type::function,functionType->ret,functionType->ret);
		}

		Uptr getControlStackSize() { return controlStack.size(); }
		
		void logOperator(const std::string& operatorDescription)
		{
			if(ENABLE_LOGGING)
			{
				std::string controlStackString;
				for(Uptr stackIndex = 0;stackIndex < controlStack.size();++stackIndex)
				{
					if(!controlStack[stackIndex].isReachable) { controlStackString += "("; }
					switch(controlStack[stackIndex].type)
					{
					case ControlContext::Type::function: controlStackString += "F"; break;
					case ControlContext::Type::block: controlStackString += "B"; break;
					case ControlContext::Type::ifThen: controlStackString += "T"; break;
					case ControlContext::Type::ifElse: controlStackString += "E"; break;
					case ControlContext::Type::loop: controlStackString += "L"; break;
					default: Errors::unreachable();
					};
					if(!controlStack[stackIndex].isReachable) { controlStackString += ")"; }
				}

				std::string stackString;
				const Uptr stackBase = controlStack.size() == 0 ? 0 : controlStack.back().outerStackSize;
				for(Uptr stackIndex = 0;stackIndex < stack.size();++stackIndex)
				{
					if(stackIndex == stackBase) { stackString += "| "; }
					stackString +=  asString(stack[stackIndex]);
					stackString +=  " ";
				}
				if(stack.size() == stackBase) { stackString += "|"; }

				Log::printf(Log::Category::debug,"%-50s %-50s %-50s\n",controlStackString.c_str(),operatorDescription.c_str(),stackString.c_str());
			}
		}

		// Operation dispatch methods.
		void unknown(Opcode opcode)
		{
			throw ValidationException("Unknown opcode: " + std::to_string((Uptr)opcode));
		}
		void block(ControlStructureImm imm)
		{
			validate(imm.resultType);
			pushControlStack(ControlContext::Type::block,imm.resultType,imm.resultType);
		}
		void loop(ControlStructureImm imm)
		{
			validate(imm.resultType);
			pushControlStack(ControlContext::Type::loop,ResultType::none,imm.resultType);
		}
		void if_(ControlStructureImm imm)
		{
			validate(imm.resultType);
			popAndValidateOperand("if condition",ValueType::i32);
			pushControlStack(ControlContext::Type::ifThen,imm.resultType,imm.resultType);
		}
		void else_(NoImm imm)
		{
			popAndValidateResultType("if result",controlStack.back().resultType);
			popControlStack(true);
		}
		void end(NoImm)
		{
			popAndValidateResultType("end result",controlStack.back().resultType);
			popControlStack();
		}
		
		void return_(NoImm)
		{
			popAndValidateResultType("ret",functionType->ret);
			enterUnreachable();
		}

		void br(BranchImm imm)
		{
			popAndValidateResultType("br argument",getBranchTargetByDepth(imm.targetDepth).branchArgumentType);
			enterUnreachable();
		}
		void br_table(BranchTableImm imm)
		{
			popAndValidateOperand("br_table index",ValueType::i32);
			const ResultType defaultTargetArgumentType = getBranchTargetByDepth(imm.defaultTargetDepth).branchArgumentType;
			popAndValidateResultType("br_table argument",defaultTargetArgumentType);

			assert(imm.branchTableIndex < functionDef.branchTables.size());
			const std::vector<U32>& targetDepths = functionDef.branchTables[imm.branchTableIndex];
			for(Uptr targetIndex = 0;targetIndex < targetDepths.size();++targetIndex)
			{
				const ResultType targetArgumentType = getBranchTargetByDepth(targetDepths[targetIndex]).branchArgumentType;
				VALIDATE_UNLESS("br_table target argument must match default target argument: ",targetArgumentType != defaultTargetArgumentType);
			}

			enterUnreachable();
		}
		void br_if(BranchImm imm)
		{
			popAndValidateOperand("br_if condition",ValueType::i32);
			popAndValidateResultType("br_if argument",getBranchTargetByDepth(imm.targetDepth).branchArgumentType);
			pushOperand(getBranchTargetByDepth(imm.targetDepth).branchArgumentType);
		}

		void unreachable(NoImm)
		{
			enterUnreachable();
		}
		void drop(NoImm)
		{
			popOperand();
		}

		void select(NoImm)
		{
			const ValueType condition = popOperand();
			const ValueType falseType = popOperand();
			const ValueType trueType = popOperand();
			validateOperandType(ValueType::i32,condition,"select condition");
			validateOperandType(falseType,trueType,"select operands");
			pushOperand(falseType);
		}

		void get_local(GetOrSetVariableImm<false> imm)
		{
			pushOperand(validateLocalIndex(imm.variableIndex));
		}
		void set_local(GetOrSetVariableImm<false> imm)
		{
			popAndValidateOperand("set_local",validateLocalIndex(imm.variableIndex));
		}
		void tee_local(GetOrSetVariableImm<false> imm)
		{
			const ValueType localType = validateLocalIndex(imm.variableIndex);
			popAndValidateOperand("tee_local",localType);
			pushOperand(localType);
		}
		
		void get_global(GetOrSetVariableImm<true> imm)
		{
			pushOperand(validateGlobalIndex(module,imm.variableIndex,false,false,false,"get_global"));
		}
		void set_global(GetOrSetVariableImm<true> imm)
		{
			popAndValidateOperand("set_global",validateGlobalIndex(module,imm.variableIndex,true,false,false,"set_global"));
		}

		void call(CallImm imm)
		{
			const FunctionType* calleeType = validateFunctionIndex(module,imm.functionIndex);
			popAndValidateOperands("call arguments",calleeType->parameters.data(),(Uptr)calleeType->parameters.size());
			pushOperand(calleeType->ret);
		}
		void call_indirect(CallIndirectImm imm)
		{
			VALIDATE_INDEX(imm.type.index,module.types.size());
			VALIDATE_UNLESS("call_indirect is only valid if there is a default function table: ",module.tables.size()==0);
			const FunctionType* calleeType = module.types[imm.type.index];
			popAndValidateOperand("call_indirect function index",ValueType::i32);
			popAndValidateOperands("call_indirect arguments",calleeType->parameters.data(),(Uptr)calleeType->parameters.size());
			pushOperand(calleeType->ret);
		}
		
		void validateImm(NoImm) {}

		template<typename nativeType>
		void validateImm(LiteralImm<nativeType> imm) {}

		template<Uptr naturalAlignmentLog2>
		void validateImm(LoadOrStoreImm<naturalAlignmentLog2> imm)
		{
			VALIDATE_UNLESS("load or store alignment greater than natural alignment: ",imm.alignmentLog2>naturalAlignmentLog2);
			VALIDATE_UNLESS("load or store in module without default memory: ",module.memories.size()==0);
		}
		
		void validateImm(MemoryImm)
		{
			VALIDATE_UNLESS("current_memory and grow_memory are only valid if there is a default memory",module.memories.size() == 0);
		}

		#if ENABLE_SIMD_PROTOTYPE
		template<Uptr numLanes>
		void validateImm(LaneIndexImm<numLanes> imm)
		{
			VALIDATE_UNLESS("swizzle invalid lane index",imm.laneIndex>=numLanes);
		}

		template<Uptr numLanes>
		void validateImm(ShuffleImm<numLanes> imm)
		{
			for(Uptr laneIndex = 0;laneIndex < numLanes;++laneIndex)
			{
				VALIDATE_UNLESS("shuffle invalid lane index",imm.laneIndices[laneIndex]>=numLanes*2);
			}
		}
		#endif

		#if ENABLE_THREADING_PROTOTYPE
		void validateImm(LaunchThreadImm)
		{
			VALIDATE_UNLESS("launch_thread is only valid if there is a default table",module.tables.size() == 0);
		}
		template<Uptr naturalAlignmentLog2>
		void validateImm(AtomicLoadOrStoreImm<naturalAlignmentLog2> imm)
		{
			VALIDATE_UNLESS("atomic memory operator in module without default memory: ",module.memories.size()==0);
			if(requireSharedFlagForAtomicOperators)
			{
				VALIDATE_UNLESS("atomic memory operators require a memory with the shared flag: ",!module.memories.getType(0).isShared);
			}
			VALIDATE_UNLESS("atomic memory operators must have natural alignment: ",imm.alignmentLog2 != naturalAlignmentLog2);
		}
		#endif

		#define LOAD(resultTypeId) \
			popAndValidateOperand(operatorName,ValueType::i32); \
			pushOperand(ResultType::resultTypeId);
		#define STORE(valueTypeId) \
			popAndValidateOperands(operatorName,ValueType::i32,ValueType::valueTypeId);
		#define NULLARY(resultTypeId) \
			pushOperand(ResultType::resultTypeId);
		#define BINARY(operandTypeId,resultTypeId) \
			popAndValidateOperands(operatorName,ValueType::operandTypeId,ValueType::operandTypeId); \
			pushOperand(ResultType::resultTypeId)
		#define UNARY(operandTypeId,resultTypeId) \
			popAndValidateOperand(operatorName,ValueType::operandTypeId); \
			pushOperand(ResultType::resultTypeId)

		#if ENABLE_SIMD_PROTOTYPE
		#define VECTORSELECT(vectorTypeId) \
			popAndValidateOperands(operatorName,ValueType::vectorTypeId,ValueType::vectorTypeId,ValueType::vectorTypeId); \
			pushOperand(ValueType::vectorTypeId);
		#define REPLACELANE(scalarTypeId,vectorTypeId) \
			popAndValidateOperands(operatorName,ValueType::vectorTypeId,ValueType::scalarTypeId); \
			pushOperand(ValueType::vectorTypeId);
		#endif

		#if ENABLE_THREADING_PROTOTYPE
		#define LAUNCHTHREAD \
			popAndValidateOperands(operatorName,ValueType::i32,ValueType::i32,ValueType::i32);
		#define COMPAREEXCHANGE(valueTypeId) \
			popAndValidateOperands(operatorName,ValueType::i32,ValueType::valueTypeId,ValueType::valueTypeId); \
			pushOperand(ValueType::valueTypeId);
		#define WAIT(valueTypeId) \
			popAndValidateOperands(operatorName,ValueType::i32,ValueType::valueTypeId,ValueType::f64); \
			pushOperand(ValueType::i32);
		#define ATOMICRMW(valueTypeId) \
			popAndValidateOperands(operatorName,ValueType::i32,ValueType::valueTypeId); \
			pushOperand(ValueType::valueTypeId);
		#endif

		#define VALIDATE_OP(opcode,name,nameString,Imm,validateOperands) \
			void name(Imm imm) \
			{ \
				const char* operatorName = nameString; SUPPRESS_UNUSED(operatorName); \
				validateImm(imm); \
				validateOperands; \
			}
		ENUM_NONCONTROL_NONPARAMETRIC_OPERATORS(VALIDATE_OP)
		#undef VALIDATE_OP

	private:
		
		struct ControlContext
		{
			enum class Type : U8
			{
				function,
				block,
				ifWithoutElse,
				ifThen,
				ifElse,
				loop
			};

			Type type;
			Uptr outerStackSize;
			
			ResultType branchArgumentType;
			ResultType resultType;
			bool isReachable;
		};

		const Module& module;
		const FunctionDef& functionDef;
		const FunctionType* functionType;

		std::vector<ValueType> locals;
		std::vector<ControlContext> controlStack;
		std::vector<ValueType> stack;

		void pushControlStack(ControlContext::Type type,ResultType branchArgumentType,ResultType resultType)
		{
			controlStack.push_back({type,stack.size(),branchArgumentType,resultType,true});
		}

		void popControlStack(bool isElse = false)
		{
			VALIDATE_UNLESS("stack was not empty at end of control structure: ",stack.size() > controlStack.back().outerStackSize);

			if(isElse && controlStack.back().type == ControlContext::Type::ifThen)
			{
				controlStack.back().type = ControlContext::Type::ifElse;
				controlStack.back().isReachable = true;
			}
			else
			{
				VALIDATE_UNLESS("else only allowed in if context: ",isElse);
				const ResultType resultType = controlStack.back().resultType;
				if(controlStack.back().type == ControlContext::Type::ifThen && resultType != ResultType::none)
				{
					throw ValidationException("else-less if may not yield a result");
				}
				controlStack.pop_back();
				if(controlStack.size()) { pushOperand(resultType); }
			}
		}

		void enterUnreachable()
		{
			stack.resize(controlStack.back().outerStackSize);
			controlStack.back().isReachable = false;
		}

		void validateBranchDepth(Uptr depth) const
		{
			VALIDATE_INDEX(depth,controlStack.size());
			if(depth >= controlStack.size()) { throw ValidationException("invalid branch depth"); }
		}

		const ControlContext& getBranchTargetByDepth(Uptr depth) const
		{
			validateBranchDepth(depth);
			return controlStack[controlStack.size() - depth - 1];
		}

		ValueType validateLocalIndex(Uptr localIndex)
		{
			VALIDATE_INDEX(localIndex,locals.size());
			return locals[localIndex];
		}
		
		ValueType popOperand()
		{
			if(stack.size() > controlStack.back().outerStackSize)
			{
				const ValueType result = stack.back();
				stack.pop_back();
				return result;
			}
			else if(controlStack.back().isReachable)
			{
				throw ValidationException("invalid stack access");
			}
			else
			{
				return ValueType::any;
			}
		}

		void popAndValidateOperands(const char* context,const ValueType* expectedTypes,Uptr num)
		{
			for(Uptr operandIndexFromEnd = 0;operandIndexFromEnd < num;++operandIndexFromEnd)
			{
				const Uptr operandIndex = num - operandIndexFromEnd - 1;
				const ValueType actualType = popOperand();
				validateOperandType(expectedTypes[operandIndex],actualType,context);
			}
		}

		template<Uptr num>
		void popAndValidateOperands(const char* context,const ValueType (&expectedTypes)[num]) { popAndValidateOperands(context,expectedTypes,num); }
		
		template<typename... OperandTypes>
		void popAndValidateOperands(const char* context,OperandTypes... operands)
		{
			ValueType operandTypes[] = {operands...};
			popAndValidateOperands(context,operandTypes);
		}

		void popAndValidateOperand(const char* context,const ValueType expectedType)
		{
			const ValueType actualType = popOperand();
			validateOperandType(expectedType,actualType,context);
		}

		void popAndValidateResultType(const char* context,ResultType expectedType)
		{
			if(expectedType != ResultType::none) { popAndValidateOperand(context,asValueType(expectedType)); }
		}

		void pushOperand(ValueType type)
		{
			stack.push_back(type);
		}
		void pushOperand(ResultType type)
		{
			if(type != ResultType::none) { pushOperand(asValueType(type)); }
		}
	};
	
	void validateDefinitions(const Module& module)
	{
		Timing::Timer timer;
		
		for(Uptr typeIndex = 0;typeIndex < module.types.size();++typeIndex)
		{
			const FunctionType* functionType = module.types[typeIndex];
			for(auto parameterType : functionType->parameters) { validate(parameterType); }
			validate(functionType->ret);
		}

		for(auto& functionImport : module.functions.imports)
		{
			VALIDATE_INDEX(functionImport.type.index,module.types.size());
		}
		for(auto& tableImport : module.tables.imports) { validate(tableImport.type); }
		for(auto& memoryImport : module.memories.imports) { validate(memoryImport.type); }
		for(auto& globalImport : module.globals.imports)
		{
			validate(globalImport.type);
			VALIDATE_UNLESS("mutable globals cannot be imported: ",globalImport.type.isMutable);
		}
		
		for(Uptr functionDefIndex = 0;functionDefIndex < module.functions.defs.size();++functionDefIndex)
		{
			const FunctionDef& functionDef = module.functions.defs[functionDefIndex];
			VALIDATE_INDEX(functionDef.type.index,module.types.size());
			for(auto localType : functionDef.nonParameterLocalTypes) { validate(localType); }
		}

		for(auto& globalDef : module.globals.defs)
		{
			validate(globalDef.type);
			validateInitializer(module,globalDef.initializer,globalDef.type.valueType,"global initializer expression");
		}
		
		for(auto& tableDef : module.tables.defs) { validate(tableDef.type); }
		VALIDATE_UNLESS("too many tables: ",module.tables.size()>1);

		for(auto& memoryDef : module.memories.defs) { validate(memoryDef.type); }
		VALIDATE_UNLESS("too many memories: ",module.memories.size()>1);

		std::set<std::string> exportNameMap;
		for(auto& exportIt : module.exports)
		{
			switch(exportIt.kind)
			{
			case ObjectKind::function:
				VALIDATE_INDEX(exportIt.index,module.functions.size());
				break;
			case ObjectKind::table:
				VALIDATE_INDEX(exportIt.index,module.tables.size());
				break;
			case ObjectKind::memory:
				VALIDATE_INDEX(exportIt.index,module.memories.size());
				break;
			case ObjectKind::global:
				validateGlobalIndex(module,exportIt.index,false,true,false,"exported global index");
				break;
			default: throw ValidationException("unknown export kind");
			};

			VALIDATE_UNLESS("duplicate export: ",exportNameMap.count(exportIt.name));
			exportNameMap.emplace(exportIt.name);
		}

		if(module.startFunctionIndex != UINTPTR_MAX)
		{
			VALIDATE_INDEX(module.startFunctionIndex,module.functions.size());
			const FunctionType* startFunctionType = module.types[module.functions.getType(module.startFunctionIndex).index];
			VALIDATE_UNLESS("start function must not have any parameters or results: ",startFunctionType != FunctionType::get());
		}
			
		for(auto& dataSegment : module.dataSegments)
		{
			VALIDATE_INDEX(dataSegment.memoryIndex,module.memories.size());
			validateInitializer(module,dataSegment.baseOffset,ValueType::i32,"data segment base initializer");
		}

		for(auto& tableSegment : module.tableSegments)
		{
			VALIDATE_INDEX(tableSegment.tableIndex,module.tables.size());
			validateInitializer(module,tableSegment.baseOffset,ValueType::i32,"table segment base initializer");
			for(auto functionIndex : tableSegment.indices) { VALIDATE_INDEX(functionIndex,module.functions.size()); }
		}

		Log::printf(Log::Category::metrics,"Validated WebAssembly module definitions in %.2fms\n",timer.getMilliseconds());
	}

	struct CodeValidationStreamImpl
	{
		FunctionValidationContext functionContext;
		OperatorPrinter operatorPrinter;

		CodeValidationStreamImpl(const Module& module,const FunctionDef& functionDef)
		: functionContext(module,functionDef)
		, operatorPrinter(module,functionDef)
		{}
	};

	CodeValidationStream::CodeValidationStream(const Module& module,const FunctionDef& functionDef)
	{
		impl = new CodeValidationStreamImpl(module,functionDef);
	}

	CodeValidationStream::~CodeValidationStream()
	{
		delete impl;
		impl = nullptr;
	}

	void CodeValidationStream::finish()
	{
		if(impl->functionContext.getControlStackSize()) { throw ValidationException("end of code reached before end of function"); }
	}

	#define VISIT_OPCODE(_,name,nameString,Imm,...) \
		void CodeValidationStream::name(Imm imm) \
		{ \
			if(ENABLE_LOGGING) { impl->functionContext.logOperator(impl->operatorPrinter.name(imm)); } \
			impl->functionContext.name(imm); \
		}
	ENUM_OPERATORS(VISIT_OPCODE)
	#undef VISIT_OPCODE
}