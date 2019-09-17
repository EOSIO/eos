#include "LLVMJIT.h"
#include "llvm/ADT/SmallVector.h"
#include "Inline/Timing.h"
#include "IR/Operators.h"
#include "IR/OperatorPrinter.h"
#include "Runtime/llvmWARshim.h"
#include "Logging/Logging.h"
#include "llvm/Support/raw_ostream.h"

#define ENABLE_LOGGING 0
#define ENABLE_FUNCTION_ENTER_EXIT_HOOKS 0

using namespace IR;

namespace LLVMJIT
{
	// The LLVM IR for a module.
	struct EmitModuleContext
	{
		const Module& module;
		ModuleInstance* moduleInstance;

		llvm::Module* llvmModule;
		std::vector<llvm::Function*> functionDefs;
		std::vector<llvm::Constant*> importedFunctionPointers;
		std::vector<llvm::Constant*> globalPointers;
		llvm::Constant* defaultTablePointer;
		llvm::Constant* defaultTableMaxElementIndex;
		llvm::Constant* defaultMemoryBase;
		llvm::Constant* defaultMemoryEndOffset;

		llvm::MDNode* likelyFalseBranchWeights;
		llvm::MDNode* likelyTrueBranchWeights;

		EmitModuleContext(const Module& inModule,ModuleInstance* inModuleInstance)
		: module(inModule)
		, moduleInstance(inModuleInstance)
		, llvmModule(new llvm::Module("",context))
		{
			auto zeroAsMetadata = llvm::ConstantAsMetadata::get(emitLiteral(I32(0)));
			auto i32MaxAsMetadata = llvm::ConstantAsMetadata::get(emitLiteral(I32(INT32_MAX)));
			likelyFalseBranchWeights = llvm::MDTuple::getDistinct(context,{llvm::MDString::get(context,"branch_weights"),zeroAsMetadata,i32MaxAsMetadata});
			likelyTrueBranchWeights = llvm::MDTuple::getDistinct(context,{llvm::MDString::get(context,"branch_weights"),i32MaxAsMetadata,zeroAsMetadata});

		}
		llvm::Module* emit();
	};

	// The context used by functions involved in JITing a single AST function.
	struct EmitFunctionContext
	{
		typedef void Result;

		EmitModuleContext& moduleContext;
		const Module& module;
		const FunctionDef& functionDef;
		const FunctionType* functionType;
		FunctionInstance* functionInstance;
		llvm::Function* llvmFunction;
		llvm::IRBuilder<> irBuilder;

		std::vector<llvm::Value*> localPointers;

		llvm::DISubprogram* diFunction;

		// Information about an in-scope control structure.
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
			llvm::BasicBlock* endBlock;
			llvm::PHINode* endPHI;
			llvm::BasicBlock* elseBlock;
			ResultType resultType;
			Uptr outerStackSize;
			Uptr outerBranchTargetStackSize;
			bool isReachable;
			bool isElseReachable;
		};

		struct BranchTarget
		{
			ResultType argumentType;
			llvm::BasicBlock* block;
			llvm::PHINode* phi;
		};

		std::vector<ControlContext> controlStack;
		std::vector<BranchTarget> branchTargetStack;
		std::vector<llvm::Value*> stack;

		EmitFunctionContext(EmitModuleContext& inEmitModuleContext,const Module& inModule,const FunctionDef& inFunctionDef,FunctionInstance* inFunctionInstance,llvm::Function* inLLVMFunction)
		: moduleContext(inEmitModuleContext)
		, module(inModule)
		, functionDef(inFunctionDef)
		, functionType(inModule.types[inFunctionDef.type.index])
		, functionInstance(inFunctionInstance)
		, llvmFunction(inLLVMFunction)
		, irBuilder(context)
		{}

		void emit();

		// Operand stack manipulation
		llvm::Value* pop()
		{
			WAVM_ASSERT_THROW(stack.size() - (controlStack.size() ? controlStack.back().outerStackSize : 0) >= 1);
			llvm::Value* result = stack.back();
			stack.pop_back();
			return result;
		}

		void popMultiple(llvm::Value** outValues,Uptr num)
		{
			WAVM_ASSERT_THROW(stack.size() - (controlStack.size() ? controlStack.back().outerStackSize : 0) >= num);
			std::copy(stack.end() - num,stack.end(),outValues);
			stack.resize(stack.size() - num);
		}

		llvm::Value* getTopValue() const
		{
			return stack.back();
		}

		void push(llvm::Value* value)
		{
			stack.push_back(value);
		}

		// Creates a PHI node for the argument of branches to a basic block.
		llvm::PHINode* createPHI(llvm::BasicBlock* basicBlock,ResultType type)
		{
			if(type == ResultType::none) { return nullptr; }
			else
			{
				auto originalBlock = irBuilder.GetInsertBlock();
				irBuilder.SetInsertPoint(basicBlock);
				auto phi = irBuilder.CreatePHI(asLLVMType(type),2);
				if(originalBlock) { irBuilder.SetInsertPoint(originalBlock); }
				return phi;
			}
		}

		// Debug logging.
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
					{
						llvm::raw_string_ostream stackTypeStream(stackString);
						stack[stackIndex]->getType()->print(stackTypeStream,true);
					}
					stackString += " ";
				}
				if(stack.size() == stackBase) { stackString += "|"; }

				Log::printf(Log::Category::debug,"%-50s %-50s %-50s\n",controlStackString.c_str(),operatorDescription.c_str(),stackString.c_str());
			}
		}
		
		// Coerces an I32 value to an I1, and vice-versa.
		llvm::Value* coerceI32ToBool(llvm::Value* i32Value)
		{
			return irBuilder.CreateICmpNE(i32Value,typedZeroConstants[(Uptr)ValueType::i32]);
		}
		llvm::Value* coerceBoolToI32(llvm::Value* boolValue)
		{
			return irBuilder.CreateZExt(boolValue,llvmI32Type);
		}
		
		// Bounds checks and converts a memory operation I32 address operand to a LLVM pointer.
		llvm::Value* coerceByteIndexToPointer(llvm::Value* byteIndex,U32 offset,llvm::Type* memoryType)
		{

			// On a 64 bit runtime, if the address is 32-bits, zext it to 64-bits.
			// This is crucial for security, as LLVM will otherwise implicitly sign extend it to 64-bits in the GEP below,
			// interpreting it as a signed offset and allowing access to memory outside the sandboxed memory range.
			// There are no 'far addresses' in a 32 bit runtime.
			byteIndex = irBuilder.CreateZExt(byteIndex,llvmI64Type);

			// Add the offset to the byte index.
			if(offset)
			{
				byteIndex = irBuilder.CreateAdd(byteIndex,irBuilder.CreateZExt(emitLiteral(offset),llvmI64Type));
			}

			// If HAS_64BIT_ADDRESS_SPACE, the memory has enough virtual address space allocated to
			// ensure that any 32-bit byte index + 32-bit offset will fall within the virtual address sandbox,
			// so no explicit bounds check is necessary.
		

			// Cast the pointer to the appropriate type.
			auto bytePointer = CreateInBoundsGEPWAR(irBuilder, moduleContext.defaultMemoryBase, byteIndex);

			return irBuilder.CreatePointerCast(bytePointer,memoryType->getPointerTo());
		}

		// Traps a divide-by-zero
		void trapDivideByZero(ValueType type,llvm::Value* divisor)
		{
			emitConditionalTrapIntrinsic(
				irBuilder.CreateICmpEQ(divisor,typedZeroConstants[(Uptr)type]),
				"wavmIntrinsics.divideByZeroOrIntegerOverflowTrap",FunctionType::get(),{});
		}

		// Traps on (x / 0) or (INT_MIN / -1).
		void trapDivideByZeroOrIntegerOverflow(ValueType type,llvm::Value* left,llvm::Value* right)
		{
			emitConditionalTrapIntrinsic(
				irBuilder.CreateOr(
					irBuilder.CreateAnd(
						irBuilder.CreateICmpEQ(left,type == ValueType::i32 ? emitLiteral((U32)INT32_MIN) : emitLiteral((U64)INT64_MIN)),
						irBuilder.CreateICmpEQ(right,type == ValueType::i32 ? emitLiteral((U32)-1) : emitLiteral((U64)-1))
						),
					irBuilder.CreateICmpEQ(right,typedZeroConstants[(Uptr)type])
					),
				"wavmIntrinsics.divideByZeroOrIntegerOverflowTrap",FunctionType::get(),{});
		}

		llvm::Value* getLLVMIntrinsic(const std::initializer_list<llvm::Type*>& argTypes,llvm::Intrinsic::ID id)
		{
			return llvm::Intrinsic::getDeclaration(moduleContext.llvmModule,id,llvm::ArrayRef<llvm::Type*>(argTypes.begin(),argTypes.end()));
		}
		
		// Emits a call to a WAVM intrinsic function.
		llvm::Value* emitRuntimeIntrinsic(const char* intrinsicName,const FunctionType* intrinsicType,const std::initializer_list<llvm::Value*>& args)
		{
			ObjectInstance* intrinsicObject = Intrinsics::find(intrinsicName,intrinsicType);
			WAVM_ASSERT_THROW(intrinsicObject);
			FunctionInstance* intrinsicFunction = asFunction(intrinsicObject);
			WAVM_ASSERT_THROW(intrinsicFunction->type == intrinsicType);
			auto intrinsicFunctionPointer = emitLiteralPointer(intrinsicFunction->nativeFunction,asLLVMType(intrinsicType)->getPointerTo());
			return irBuilder.CreateCall(intrinsicFunctionPointer,llvm::ArrayRef<llvm::Value*>(args.begin(),args.end()));
		}

		// A helper function to emit a conditional call to a non-returning intrinsic function.
		void emitConditionalTrapIntrinsic(llvm::Value* booleanCondition,const char* intrinsicName,const FunctionType* intrinsicType,const std::initializer_list<llvm::Value*>& args)
		{
			auto trueBlock = llvm::BasicBlock::Create(context,llvm::Twine(intrinsicName) + "Trap",llvmFunction);
			auto endBlock = llvm::BasicBlock::Create(context,llvm::Twine(intrinsicName) + "Skip",llvmFunction);

			irBuilder.CreateCondBr(booleanCondition,trueBlock,endBlock,moduleContext.likelyFalseBranchWeights);

			irBuilder.SetInsertPoint(trueBlock);
			emitRuntimeIntrinsic(intrinsicName,intrinsicType,args);
			irBuilder.CreateUnreachable();

			irBuilder.SetInsertPoint(endBlock);
		}

		//
		// Misc operators
		//

		void nop(NoImm) {}
		void unknown(Opcode opcode) { Errors::unreachable(); }
		
		//
		// Control structure operators
		//
		
		void pushControlStack(
			ControlContext::Type type,
			ResultType resultType,
			llvm::BasicBlock* endBlock,
			llvm::PHINode* endPHI,
			llvm::BasicBlock* elseBlock = nullptr
			)
		{
			// The unreachable operator filtering should filter out any opcodes that call pushControlStack.
			if(controlStack.size()) { errorUnless(controlStack.back().isReachable); }

			controlStack.push_back({type,endBlock,endPHI,elseBlock,resultType,stack.size(),branchTargetStack.size(),true,true});
		}

		void pushBranchTarget(ResultType branchArgumentType,llvm::BasicBlock* branchTargetBlock,llvm::PHINode* branchTargetPHI)
		{
			branchTargetStack.push_back({branchArgumentType,branchTargetBlock,branchTargetPHI});
		}

		void block(ControlStructureImm imm)
		{
			// Create an end block+phi for the block result.
			auto endBlock = llvm::BasicBlock::Create(context,"blockEnd",llvmFunction);
			auto endPHI = createPHI(endBlock,imm.resultType);

			// Push a control context that ends at the end block/phi.
			pushControlStack(ControlContext::Type::block,imm.resultType,endBlock,endPHI);
			
			// Push a branch target for the end block/phi.
			pushBranchTarget(imm.resultType,endBlock,endPHI);
		}
		void loop(ControlStructureImm imm)
		{
			// Create a loop block, and an end block+phi for the loop result.
			auto loopBodyBlock = llvm::BasicBlock::Create(context,"loopBody",llvmFunction);
			auto endBlock = llvm::BasicBlock::Create(context,"loopEnd",llvmFunction);
			auto endPHI = createPHI(endBlock,imm.resultType);
			
			// Branch to the loop body and switch the IR builder to emit there.
			irBuilder.CreateBr(loopBodyBlock);
			irBuilder.SetInsertPoint(loopBodyBlock);

			// Push a control context that ends at the end block/phi.
			pushControlStack(ControlContext::Type::loop,imm.resultType,endBlock,endPHI);
			
			// Push a branch target for the loop body start.
			pushBranchTarget(ResultType::none,loopBodyBlock,nullptr);
		}
		void if_(ControlStructureImm imm)
		{
			// Create a then block and else block for the if, and an end block+phi for the if result.
			auto thenBlock = llvm::BasicBlock::Create(context,"ifThen",llvmFunction);
			auto elseBlock = llvm::BasicBlock::Create(context,"ifElse",llvmFunction);
			auto endBlock = llvm::BasicBlock::Create(context,"ifElseEnd",llvmFunction);
			auto endPHI = createPHI(endBlock,imm.resultType);

			// Pop the if condition from the operand stack.
			auto condition = pop();
			irBuilder.CreateCondBr(coerceI32ToBool(condition),thenBlock,elseBlock);
			
			// Switch the IR builder to emit the then block.
			irBuilder.SetInsertPoint(thenBlock);

			// Push an ifThen control context that ultimately ends at the end block/phi, but may
			// be terminated by an else operator that changes the control context to the else block.
			pushControlStack(ControlContext::Type::ifThen,imm.resultType,endBlock,endPHI,elseBlock);
			
			// Push a branch target for the if end.
			pushBranchTarget(imm.resultType,endBlock,endPHI);
			
		}
		void else_(NoImm imm)
		{
			WAVM_ASSERT_THROW(controlStack.size());
			ControlContext& currentContext = controlStack.back();

			if(currentContext.isReachable)
			{
				// If the control context expects a result, take it from the operand stack and add it to the
				// control context's end PHI.
				if(currentContext.resultType != ResultType::none)
				{
					llvm::Value* result = pop();
					currentContext.endPHI->addIncoming(result,irBuilder.GetInsertBlock());
				}

				// Branch to the control context's end.
				irBuilder.CreateBr(currentContext.endBlock);
			}
			WAVM_ASSERT_THROW(stack.size() == currentContext.outerStackSize);

			// Switch the IR emitter to the else block.
			WAVM_ASSERT_THROW(currentContext.elseBlock);
			WAVM_ASSERT_THROW(currentContext.type == ControlContext::Type::ifThen);
			currentContext.elseBlock->moveAfter(irBuilder.GetInsertBlock());
			irBuilder.SetInsertPoint(currentContext.elseBlock);

			// Change the top of the control stack to an else clause.
			currentContext.type = ControlContext::Type::ifElse;
			currentContext.isReachable = currentContext.isElseReachable;
			currentContext.elseBlock = nullptr;
		}
		void end(NoImm)
		{
			WAVM_ASSERT_THROW(controlStack.size());
			ControlContext& currentContext = controlStack.back();

			if(currentContext.isReachable)
			{
				// If the control context yields a result, take the top of the operand stack and
				// add it to the control context's end PHI.
				if(currentContext.resultType != ResultType::none)
				{
					llvm::Value* result = pop();
					currentContext.endPHI->addIncoming(result,irBuilder.GetInsertBlock());
				}

				// Branch to the control context's end.
				irBuilder.CreateBr(currentContext.endBlock);
			}
			WAVM_ASSERT_THROW(stack.size() == currentContext.outerStackSize);

			if(currentContext.elseBlock)
			{
				// If this is the end of an if without an else clause, create a dummy else clause.
				currentContext.elseBlock->moveAfter(irBuilder.GetInsertBlock());
				irBuilder.SetInsertPoint(currentContext.elseBlock);
				irBuilder.CreateBr(currentContext.endBlock);
			}

			// Switch the IR emitter to the end block.
			currentContext.endBlock->moveAfter(irBuilder.GetInsertBlock());
			irBuilder.SetInsertPoint(currentContext.endBlock);

			if(currentContext.endPHI)
			{
				// If the control context yields a result, take the PHI that merges all the control flow
				// to the end and push it onto the operand stack.
				if(currentContext.endPHI->getNumIncomingValues()) { push(currentContext.endPHI); }
				else
				{
					// If there weren't any incoming values for the end PHI, remove it and push a dummy value.
					currentContext.endPHI->eraseFromParent();
					WAVM_ASSERT_THROW(currentContext.resultType != ResultType::none);
					push(typedZeroConstants[(Uptr)asValueType(currentContext.resultType)]);
				}
			}

			// Pop and branch targets introduced by this control context.
			WAVM_ASSERT_THROW(currentContext.outerBranchTargetStackSize <= branchTargetStack.size());
			branchTargetStack.resize(currentContext.outerBranchTargetStackSize);

			// Pop this control context.
			controlStack.pop_back();
		}
		
		//
		// Control flow operators
		//
		
		BranchTarget& getBranchTargetByDepth(Uptr depth)
		{
			WAVM_ASSERT_THROW(depth < branchTargetStack.size());
			return branchTargetStack[branchTargetStack.size() - depth - 1];
		}
		
		// This is called after unconditional control flow to indicate that operators following it are unreachable until the control stack is popped.
		void enterUnreachable()
		{
			// Unwind the operand stack to the outer control context.
			WAVM_ASSERT_THROW(controlStack.back().outerStackSize <= stack.size());
			stack.resize(controlStack.back().outerStackSize);

			// Mark the current control context as unreachable: this will cause the outer loop to stop dispatching operators to us
			// until an else/end for the current control context is reached.
			controlStack.back().isReachable = false;
		}
		
		void br_if(BranchImm imm)
		{
			// Pop the condition from operand stack.
			auto condition = pop();

			BranchTarget& target = getBranchTargetByDepth(imm.targetDepth);
			if(target.argumentType != ResultType::none)
			{
				// Use the stack top as the branch argument (don't pop it) and add it to the target phi's incoming values.
				llvm::Value* argument = getTopValue();
				target.phi->addIncoming(argument,irBuilder.GetInsertBlock());
			}

			// Create a new basic block for the case where the branch is not taken.
			auto falseBlock = llvm::BasicBlock::Create(context,"br_ifElse",llvmFunction);

			// Emit a conditional branch to either the falseBlock or the target block.
			irBuilder.CreateCondBr(coerceI32ToBool(condition),target.block,falseBlock);

			// Resume emitting instructions in the falseBlock.
			irBuilder.SetInsertPoint(falseBlock);
		}
		
		void br(BranchImm imm)
		{
			BranchTarget& target = getBranchTargetByDepth(imm.targetDepth);
			if(target.argumentType != ResultType::none)
			{
				// Pop the branch argument from the stack and add it to the target phi's incoming values.
				llvm::Value* argument = pop();
				target.phi->addIncoming(argument,irBuilder.GetInsertBlock());
			}

			// Branch to the target block.
			irBuilder.CreateBr(target.block);

			enterUnreachable();
		}
		void br_table(BranchTableImm imm)
		{
			// Pop the table index from the operand stack.
			auto index = pop();
			
			// Look up the default branch target, and assume its argument type applies to all targets.
			// (this is guaranteed by the validator)
			BranchTarget& defaultTarget = getBranchTargetByDepth(imm.defaultTargetDepth);
			const ResultType argumentType = defaultTarget.argumentType;
			llvm::Value* argument = nullptr;
			if(argumentType != ResultType::none)
			{
				// Pop the branch argument from the stack and add it to the default target phi's incoming values.
				argument = pop();
				defaultTarget.phi->addIncoming(argument,irBuilder.GetInsertBlock());
			}

			// Create a LLVM switch instruction.
			WAVM_ASSERT_THROW(imm.branchTableIndex < functionDef.branchTables.size());
			const std::vector<U32>& targetDepths = functionDef.branchTables[imm.branchTableIndex];
			auto llvmSwitch = irBuilder.CreateSwitch(index,defaultTarget.block,(unsigned int)targetDepths.size());

			for(Uptr targetIndex = 0;targetIndex < targetDepths.size();++targetIndex)
			{
				BranchTarget& target = getBranchTargetByDepth(targetDepths[targetIndex]);

				// Add this target to the switch instruction.
				llvmSwitch->addCase(emitLiteral((U32)targetIndex),target.block);

				if(argumentType != ResultType::none)
				{
					// If this is the first case in the table for this branch target, add the branch argument to
					// the target phi's incoming values.
					target.phi->addIncoming(argument,irBuilder.GetInsertBlock());
				}
			}

			enterUnreachable();
		}
		void return_(NoImm)
		{
			if(functionType->ret != ResultType::none)
			{
				// Pop the return value from the stack and add it to the return phi's incoming values.
				llvm::Value* result = pop();
				controlStack[0].endPHI->addIncoming(result,irBuilder.GetInsertBlock());
			}

			// Branch to the return block.
			irBuilder.CreateBr(controlStack[0].endBlock);

			enterUnreachable();
		}

		void unreachable(NoImm)
		{
			// Call an intrinsic that causes a trap, and insert the LLVM unreachable terminator.
			emitRuntimeIntrinsic("wavmIntrinsics.unreachableTrap",FunctionType::get(),{});
			irBuilder.CreateUnreachable();

			enterUnreachable();
		}

		//
		// Polymorphic operators
		//

		void drop(NoImm) { stack.pop_back(); }

		void select(NoImm)
		{
			auto condition = pop();
			auto falseValue = pop();
			auto trueValue = pop();
			push(irBuilder.CreateSelect(coerceI32ToBool(condition),trueValue,falseValue));
		}

		//
		// Call operators
		//

		void call(CallImm imm)
		{
			// Map the callee function index to either an imported function pointer or a function in this module.
			llvm::Value* callee;
			const FunctionType* calleeType;
			if(imm.functionIndex < moduleContext.importedFunctionPointers.size())
			{
				WAVM_ASSERT_THROW(imm.functionIndex < moduleContext.moduleInstance->functions.size());
				callee = moduleContext.importedFunctionPointers[imm.functionIndex];
				calleeType = moduleContext.moduleInstance->functions[imm.functionIndex]->type;
			}
			else
			{
				const Uptr calleeIndex = imm.functionIndex - moduleContext.importedFunctionPointers.size();
				WAVM_ASSERT_THROW(calleeIndex < moduleContext.functionDefs.size());
				callee = moduleContext.functionDefs[calleeIndex];
				calleeType = module.types[module.functions.defs[calleeIndex].type.index];
			}

			// Pop the call arguments from the operand stack.
			auto llvmArgs = (llvm::Value**)alloca(sizeof(llvm::Value*) * calleeType->parameters.size());
			popMultiple(llvmArgs,calleeType->parameters.size());

			// Call the function.
			auto result = irBuilder.CreateCall(callee,llvm::ArrayRef<llvm::Value*>(llvmArgs,calleeType->parameters.size()));

			// Push the result on the operand stack.
			if(calleeType->ret != ResultType::none) { push(result); }
		}
		void call_indirect(CallIndirectImm imm)
		{
			WAVM_ASSERT_THROW(imm.type.index < module.types.size());
			
			auto calleeType = module.types[imm.type.index];
			auto functionPointerType = asLLVMType(calleeType)->getPointerTo()->getPointerTo();

			// Compile the function index.
			auto tableElementIndex = pop();
			
			// Compile the call arguments.
			auto llvmArgs = (llvm::Value**)alloca(sizeof(llvm::Value*) * calleeType->parameters.size());
			popMultiple(llvmArgs,calleeType->parameters.size());

			// Zero extend the function index to the pointer size.
			auto functionIndexZExt = irBuilder.CreateZExt(tableElementIndex,sizeof(Uptr) == 4 ? llvmI32Type : llvmI64Type);
			
			// If the function index is larger than the function table size, trap.
			emitConditionalTrapIntrinsic(
				irBuilder.CreateICmpUGE(functionIndexZExt,moduleContext.defaultTableMaxElementIndex),
				"wavmIntrinsics.indirectCallIndexOutOfBounds",FunctionType::get(),{});

			// Load the type for this table entry.
			auto functionTypePointerPointer = CreateInBoundsGEPWAR(irBuilder, moduleContext.defaultTablePointer, functionIndexZExt, emitLiteral((U32)0));
			auto functionTypePointer = irBuilder.CreateLoad(functionTypePointerPointer);
			auto llvmCalleeType = emitLiteralPointer(calleeType,llvmI8PtrType);
			
			// If the function type doesn't match, trap.
			emitConditionalTrapIntrinsic(
				irBuilder.CreateICmpNE(llvmCalleeType,functionTypePointer),
				"wavmIntrinsics.indirectCallSignatureMismatch",
				FunctionType::get(ResultType::none,{ValueType::i32,ValueType::i64,ValueType::i64}),
				{	tableElementIndex,
					irBuilder.CreatePtrToInt(llvmCalleeType,llvmI64Type),
					emitLiteral(reinterpret_cast<U64>(moduleContext.moduleInstance->defaultTable))	}
				);

			// Call the function loaded from the table.
			auto functionPointerPointer = CreateInBoundsGEPWAR(irBuilder, moduleContext.defaultTablePointer, functionIndexZExt, emitLiteral((U32)1));
			auto functionPointer = irBuilder.CreateLoad(irBuilder.CreatePointerCast(functionPointerPointer,functionPointerType));
			auto result = irBuilder.CreateCall(functionPointer,llvm::ArrayRef<llvm::Value*>(llvmArgs,calleeType->parameters.size()));

			// Push the result on the operand stack.
			if(calleeType->ret != ResultType::none) { push(result); }
		}
		
		//
		// Local/global operators
		//

		void get_local(GetOrSetVariableImm<false> imm)
		{
			WAVM_ASSERT_THROW(imm.variableIndex < localPointers.size());
			push(irBuilder.CreateLoad(localPointers[imm.variableIndex]));
		}
		void set_local(GetOrSetVariableImm<false> imm)
		{
			WAVM_ASSERT_THROW(imm.variableIndex < localPointers.size());
			auto value = irBuilder.CreateBitCast(pop(),localPointers[imm.variableIndex]->getType()->getPointerElementType());
			irBuilder.CreateStore(value,localPointers[imm.variableIndex]);
		}
		void tee_local(GetOrSetVariableImm<false> imm)
		{
			WAVM_ASSERT_THROW(imm.variableIndex < localPointers.size());
			auto value = irBuilder.CreateBitCast(getTopValue(),localPointers[imm.variableIndex]->getType()->getPointerElementType());
			irBuilder.CreateStore(value,localPointers[imm.variableIndex]);
		}
		
		void get_global(GetOrSetVariableImm<true> imm)
		{
			WAVM_ASSERT_THROW(imm.variableIndex < moduleContext.globalPointers.size());
			push(irBuilder.CreateLoad(moduleContext.globalPointers[imm.variableIndex]));
		}
		void set_global(GetOrSetVariableImm<true> imm)
		{
			WAVM_ASSERT_THROW(imm.variableIndex < moduleContext.globalPointers.size());
			auto value = irBuilder.CreateBitCast(pop(),moduleContext.globalPointers[imm.variableIndex]->getType()->getPointerElementType());
			irBuilder.CreateStore(value,moduleContext.globalPointers[imm.variableIndex]);
		}

		//
		// Memory size operators
		// These just call out to wavmIntrinsics.growMemory/currentMemory, passing a pointer to the default memory for the module.
		//

		void grow_memory(MemoryImm)
		{
			auto deltaNumPages = pop();
			auto defaultMemoryObjectAsI64 = emitLiteral(reinterpret_cast<U64>(moduleContext.moduleInstance->defaultMemory));
			auto previousNumPages = emitRuntimeIntrinsic(
				"wavmIntrinsics.growMemory",
				FunctionType::get(ResultType::i32,{ValueType::i32,ValueType::i64}),
				{deltaNumPages,defaultMemoryObjectAsI64});
			push(previousNumPages);
		}
		void current_memory(MemoryImm)
		{
			auto defaultMemoryObjectAsI64 = emitLiteral(reinterpret_cast<U64>(moduleContext.moduleInstance->defaultMemory));
			auto currentNumPages = emitRuntimeIntrinsic(
				"wavmIntrinsics.currentMemory",
				FunctionType::get(ResultType::i32,{ValueType::i64}),
				{defaultMemoryObjectAsI64});
			push(currentNumPages);
		}

		//
		// Constant operators
		//

		#define EMIT_CONST(typeId,nativeType) void typeId##_const(LiteralImm<nativeType> imm) { push(emitLiteral(imm.value)); }
		EMIT_CONST(i32,I32) EMIT_CONST(i64,I64)
		EMIT_CONST(f32,F32) EMIT_CONST(f64,F64)

		//
		// Load/store operators
		//

		#define EMIT_LOAD_OP(valueTypeId,name,llvmMemoryType,naturalAlignmentLog2,conversionOp) \
			void valueTypeId##_##name(LoadOrStoreImm<naturalAlignmentLog2> imm) \
			{ \
				auto byteIndex = pop(); \
				auto pointer = coerceByteIndexToPointer(byteIndex,imm.offset,llvmMemoryType); \
				auto load = irBuilder.CreateLoad(pointer); \
				load->setAlignment(1<<imm.alignmentLog2); \
				load->setVolatile(true); \
				push(conversionOp(load,asLLVMType(ValueType::valueTypeId))); \
			}
		#define EMIT_STORE_OP(valueTypeId,name,llvmMemoryType,naturalAlignmentLog2,conversionOp) \
			void valueTypeId##_##name(LoadOrStoreImm<naturalAlignmentLog2> imm) \
			{ \
				auto value = pop(); \
				auto byteIndex = pop(); \
				auto pointer = coerceByteIndexToPointer(byteIndex,imm.offset,llvmMemoryType); \
				auto memoryValue = conversionOp(value,llvmMemoryType); \
				auto store = irBuilder.CreateStore(memoryValue,pointer); \
				store->setVolatile(true); \
				store->setAlignment(1<<imm.alignmentLog2); \
			}
			
		llvm::Value* identityConversion(llvm::Value* value,llvm::Type* type) { return value; }

		EMIT_LOAD_OP(i32,load8_s,llvmI8Type,0,irBuilder.CreateSExt)  EMIT_LOAD_OP(i32,load8_u,llvmI8Type,0,irBuilder.CreateZExt)
		EMIT_LOAD_OP(i32,load16_s,llvmI16Type,1,irBuilder.CreateSExt) EMIT_LOAD_OP(i32,load16_u,llvmI16Type,1,irBuilder.CreateZExt)
		EMIT_LOAD_OP(i64,load8_s,llvmI8Type,0,irBuilder.CreateSExt)  EMIT_LOAD_OP(i64,load8_u,llvmI8Type,0,irBuilder.CreateZExt)
		EMIT_LOAD_OP(i64,load16_s,llvmI16Type,1,irBuilder.CreateSExt)  EMIT_LOAD_OP(i64,load16_u,llvmI16Type,1,irBuilder.CreateZExt)
		EMIT_LOAD_OP(i64,load32_s,llvmI32Type,2,irBuilder.CreateSExt)  EMIT_LOAD_OP(i64,load32_u,llvmI32Type,2,irBuilder.CreateZExt)

		EMIT_LOAD_OP(i32,load,llvmI32Type,2,identityConversion) EMIT_LOAD_OP(i64,load,llvmI64Type,3,identityConversion)
		EMIT_LOAD_OP(f32,load,llvmF32Type,2,identityConversion) EMIT_LOAD_OP(f64,load,llvmF64Type,3,identityConversion)

		EMIT_STORE_OP(i32,store8,llvmI8Type,0,irBuilder.CreateTrunc) EMIT_STORE_OP(i64,store8,llvmI8Type,0,irBuilder.CreateTrunc)
		EMIT_STORE_OP(i32,store16,llvmI16Type,1,irBuilder.CreateTrunc) EMIT_STORE_OP(i64,store16,llvmI16Type,1,irBuilder.CreateTrunc)
		EMIT_STORE_OP(i32,store,llvmI32Type,2,irBuilder.CreateTrunc) EMIT_STORE_OP(i64,store32,llvmI32Type,2,irBuilder.CreateTrunc)
		EMIT_STORE_OP(i64,store,llvmI64Type,3,identityConversion)
		EMIT_STORE_OP(f32,store,llvmF32Type,2,identityConversion) EMIT_STORE_OP(f64,store,llvmF64Type,3,identityConversion)

		//
		// Numeric operator macros
		//

		#define EMIT_BINARY_OP(typeId,name,emitCode) void typeId##_##name(NoImm) \
			{ \
				const ValueType type = ValueType::typeId; SUPPRESS_UNUSED(type); \
				auto right = pop(); \
				auto left = pop(); \
				push(emitCode); \
			}
		#define EMIT_INT_BINARY_OP(name,emitCode) EMIT_BINARY_OP(i32,name,emitCode) EMIT_BINARY_OP(i64,name,emitCode)
		#define EMIT_FP_BINARY_OP(name,emitCode) EMIT_BINARY_OP(f32,name,emitCode) EMIT_BINARY_OP(f64,name,emitCode)

		#define EMIT_UNARY_OP(typeId,name,emitCode) void typeId##_##name(NoImm) \
			{ \
				const ValueType type = ValueType::typeId; SUPPRESS_UNUSED(type); \
				auto operand = pop(); \
				push(emitCode); \
			}
		#define EMIT_INT_UNARY_OP(name,emitCode) EMIT_UNARY_OP(i32,name,emitCode) EMIT_UNARY_OP(i64,name,emitCode)
		#define EMIT_FP_UNARY_OP(name,emitCode) EMIT_UNARY_OP(f32,name,emitCode) EMIT_UNARY_OP(f64,name,emitCode)

		//
		// Int operators
		//

		llvm::Value* emitSRem(ValueType type,llvm::Value* left,llvm::Value* right)
		{
			// Trap if the dividend is zero.
			trapDivideByZero(type,right); 

			// LLVM's srem has undefined behavior where WebAssembly's rem_s defines that it should not trap if the corresponding
			// division would overflow a signed integer. To avoid this case, we just branch around the srem if the INT_MAX%-1 case
			// that overflows is detected.
			auto preOverflowBlock = irBuilder.GetInsertBlock();
			auto noOverflowBlock = llvm::BasicBlock::Create(context,"sremNoOverflow",llvmFunction);
			auto endBlock = llvm::BasicBlock::Create(context,"sremEnd",llvmFunction);
			auto noOverflow = irBuilder.CreateOr(
				irBuilder.CreateICmpNE(left,type == ValueType::i32 ? emitLiteral((U32)INT32_MIN) : emitLiteral((U64)INT64_MIN)),
				irBuilder.CreateICmpNE(right,type == ValueType::i32 ? emitLiteral((U32)-1) : emitLiteral((U64)-1))
				);
			irBuilder.CreateCondBr(noOverflow,noOverflowBlock,endBlock,moduleContext.likelyTrueBranchWeights);

			irBuilder.SetInsertPoint(noOverflowBlock);
			auto noOverflowValue = irBuilder.CreateSRem(left,right);
			irBuilder.CreateBr(endBlock);

			irBuilder.SetInsertPoint(endBlock);
			auto phi = irBuilder.CreatePHI(asLLVMType(type),2);
			phi->addIncoming(typedZeroConstants[(Uptr)type],preOverflowBlock);
			phi->addIncoming(noOverflowValue,noOverflowBlock);
			return phi;
		}
		
		llvm::Value* emitShiftCountMask(ValueType type,llvm::Value* shiftCount)
		{
			// LLVM's shifts have undefined behavior where WebAssembly specifies that the shift count will wrap numbers
			// grather than the bit count of the operands. This matches x86's native shift instructions, but explicitly mask
			// the shift count anyway to support other platforms, and ensure the optimizer doesn't take advantage of the UB.
			auto bitsMinusOne = irBuilder.CreateZExt(emitLiteral((U8)(getTypeBitWidth(type) - 1)),asLLVMType(type));
			return irBuilder.CreateAnd(shiftCount,bitsMinusOne);
		}

		llvm::Value* emitRotl(ValueType type,llvm::Value* left,llvm::Value* right)
		{
			auto bitWidthMinusRight = irBuilder.CreateSub(
				irBuilder.CreateZExt(emitLiteral(getTypeBitWidth(type)),asLLVMType(type)),
				right
				);
			return irBuilder.CreateOr(
				irBuilder.CreateShl(left,emitShiftCountMask(type,right)),
				irBuilder.CreateLShr(left,emitShiftCountMask(type,bitWidthMinusRight))
				);
		}
		
		llvm::Value* emitRotr(ValueType type,llvm::Value* left,llvm::Value* right)
		{
			auto bitWidthMinusRight = irBuilder.CreateSub(
				irBuilder.CreateZExt(emitLiteral(getTypeBitWidth(type)),asLLVMType(type)),
				right
				);
			return irBuilder.CreateOr(
				irBuilder.CreateShl(left,emitShiftCountMask(type,bitWidthMinusRight)),
				irBuilder.CreateLShr(left,emitShiftCountMask(type,right))
				);
		}

		EMIT_INT_BINARY_OP(add,irBuilder.CreateAdd(left,right))
		EMIT_INT_BINARY_OP(sub,irBuilder.CreateSub(left,right))
		EMIT_INT_BINARY_OP(mul,irBuilder.CreateMul(left,right))
		EMIT_INT_BINARY_OP(and,irBuilder.CreateAnd(left,right))
		EMIT_INT_BINARY_OP(or,irBuilder.CreateOr(left,right))
		EMIT_INT_BINARY_OP(xor,irBuilder.CreateXor(left,right))
		EMIT_INT_BINARY_OP(rotr,emitRotr(type,left,right))
		EMIT_INT_BINARY_OP(rotl,emitRotl(type,left,right))
			
		// Divides use trapDivideByZero to avoid the undefined behavior in LLVM's division instructions.
		EMIT_INT_BINARY_OP(div_s, (trapDivideByZeroOrIntegerOverflow(type,left,right), irBuilder.CreateSDiv(left,right)) )
		EMIT_INT_BINARY_OP(rem_s, emitSRem(type,left,right) )
		EMIT_INT_BINARY_OP(div_u, (trapDivideByZero(type,right), irBuilder.CreateUDiv(left,right)) )
		EMIT_INT_BINARY_OP(rem_u, (trapDivideByZero(type,right), irBuilder.CreateURem(left,right)) )

		// Explicitly mask the shift amount operand to the word size to avoid LLVM's undefined behavior.
		EMIT_INT_BINARY_OP(shl,irBuilder.CreateShl(left,emitShiftCountMask(type,right)))
		EMIT_INT_BINARY_OP(shr_s,irBuilder.CreateAShr(left,emitShiftCountMask(type,right)))
		EMIT_INT_BINARY_OP(shr_u,irBuilder.CreateLShr(left,emitShiftCountMask(type,right)))
		
		EMIT_INT_BINARY_OP(eq,coerceBoolToI32(irBuilder.CreateICmpEQ(left,right)))
		EMIT_INT_BINARY_OP(ne,coerceBoolToI32(irBuilder.CreateICmpNE(left,right)))
		EMIT_INT_BINARY_OP(lt_s,coerceBoolToI32(irBuilder.CreateICmpSLT(left,right)))
		EMIT_INT_BINARY_OP(lt_u,coerceBoolToI32(irBuilder.CreateICmpULT(left,right)))
		EMIT_INT_BINARY_OP(le_s,coerceBoolToI32(irBuilder.CreateICmpSLE(left,right)))
		EMIT_INT_BINARY_OP(le_u,coerceBoolToI32(irBuilder.CreateICmpULE(left,right)))
		EMIT_INT_BINARY_OP(gt_s,coerceBoolToI32(irBuilder.CreateICmpSGT(left,right)))
		EMIT_INT_BINARY_OP(gt_u,coerceBoolToI32(irBuilder.CreateICmpUGT(left,right)))
		EMIT_INT_BINARY_OP(ge_s,coerceBoolToI32(irBuilder.CreateICmpSGE(left,right)))
		EMIT_INT_BINARY_OP(ge_u,coerceBoolToI32(irBuilder.CreateICmpUGE(left,right)))

		EMIT_INT_UNARY_OP(clz,irBuilder.CreateCall(getLLVMIntrinsic({operand->getType()},llvm::Intrinsic::ctlz),llvm::ArrayRef<llvm::Value*>({operand,emitLiteral(false)})))
		EMIT_INT_UNARY_OP(ctz,irBuilder.CreateCall(getLLVMIntrinsic({operand->getType()},llvm::Intrinsic::cttz),llvm::ArrayRef<llvm::Value*>({operand,emitLiteral(false)})))
		EMIT_INT_UNARY_OP(popcnt,irBuilder.CreateCall(getLLVMIntrinsic({operand->getType()},llvm::Intrinsic::ctpop),llvm::ArrayRef<llvm::Value*>({operand})))
		EMIT_INT_UNARY_OP(eqz,coerceBoolToI32(irBuilder.CreateICmpEQ(operand,typedZeroConstants[(Uptr)type])))

		//
		// FP operators
		//

		EMIT_FP_BINARY_OP(add,irBuilder.CreateFAdd(left,right))
		EMIT_FP_BINARY_OP(sub,irBuilder.CreateFSub(left,right))
		EMIT_FP_BINARY_OP(mul,irBuilder.CreateFMul(left,right))
		EMIT_FP_BINARY_OP(div,irBuilder.CreateFDiv(left,right))
		EMIT_FP_BINARY_OP(copysign,irBuilder.CreateCall(getLLVMIntrinsic({left->getType()},llvm::Intrinsic::copysign),llvm::ArrayRef<llvm::Value*>({left,right})))

		EMIT_FP_UNARY_OP(neg,irBuilder.CreateFNeg(operand))
		EMIT_FP_UNARY_OP(abs,irBuilder.CreateCall(getLLVMIntrinsic({operand->getType()},llvm::Intrinsic::fabs),llvm::ArrayRef<llvm::Value*>({operand})))
		EMIT_FP_UNARY_OP(sqrt,irBuilder.CreateCall(getLLVMIntrinsic({operand->getType()},llvm::Intrinsic::sqrt),llvm::ArrayRef<llvm::Value*>({operand})))

		EMIT_FP_BINARY_OP(eq,coerceBoolToI32(irBuilder.CreateFCmpOEQ(left,right)))
		EMIT_FP_BINARY_OP(ne,coerceBoolToI32(irBuilder.CreateFCmpUNE(left,right)))
		EMIT_FP_BINARY_OP(lt,coerceBoolToI32(irBuilder.CreateFCmpOLT(left,right)))
		EMIT_FP_BINARY_OP(le,coerceBoolToI32(irBuilder.CreateFCmpOLE(left,right)))
		EMIT_FP_BINARY_OP(gt,coerceBoolToI32(irBuilder.CreateFCmpOGT(left,right)))
		EMIT_FP_BINARY_OP(ge,coerceBoolToI32(irBuilder.CreateFCmpOGE(left,right)))

		EMIT_UNARY_OP(i32,wrap_i64,irBuilder.CreateTrunc(operand,llvmI32Type))
		EMIT_UNARY_OP(i64,extend_s_i32,irBuilder.CreateSExt(operand,llvmI64Type))
		EMIT_UNARY_OP(i64,extend_u_i32,irBuilder.CreateZExt(operand,llvmI64Type))

		EMIT_FP_UNARY_OP(convert_s_i32,irBuilder.CreateSIToFP(operand,asLLVMType(type)))
		EMIT_FP_UNARY_OP(convert_s_i64,irBuilder.CreateSIToFP(operand,asLLVMType(type)))
		EMIT_FP_UNARY_OP(convert_u_i32,irBuilder.CreateUIToFP(operand,asLLVMType(type)))
		EMIT_FP_UNARY_OP(convert_u_i64,irBuilder.CreateUIToFP(operand,asLLVMType(type)))

		EMIT_UNARY_OP(f32,demote_f64,irBuilder.CreateFPTrunc(operand,llvmF32Type))
		EMIT_UNARY_OP(f64,promote_f32,irBuilder.CreateFPExt(operand,llvmF64Type))
		EMIT_UNARY_OP(f32,reinterpret_i32,irBuilder.CreateBitCast(operand,llvmF32Type))
		EMIT_UNARY_OP(f64,reinterpret_i64,irBuilder.CreateBitCast(operand,llvmF64Type))
		EMIT_UNARY_OP(i32,reinterpret_f32,irBuilder.CreateBitCast(operand,llvmI32Type))
		EMIT_UNARY_OP(i64,reinterpret_f64,irBuilder.CreateBitCast(operand,llvmI64Type))

		// These operations don't match LLVM's semantics exactly, so just call out to C++ implementations.
		EMIT_FP_BINARY_OP(min,emitRuntimeIntrinsic("wavmIntrinsics.floatMin",FunctionType::get(asResultType(type),{type,type}),{left,right}))
		EMIT_FP_BINARY_OP(max,emitRuntimeIntrinsic("wavmIntrinsics.floatMax",FunctionType::get(asResultType(type),{type,type}),{left,right}))
		EMIT_FP_UNARY_OP(ceil,emitRuntimeIntrinsic("wavmIntrinsics.floatCeil",FunctionType::get(asResultType(type),{type}),{operand}))
		EMIT_FP_UNARY_OP(floor,emitRuntimeIntrinsic("wavmIntrinsics.floatFloor",FunctionType::get(asResultType(type),{type}),{operand}))
		EMIT_FP_UNARY_OP(trunc,emitRuntimeIntrinsic("wavmIntrinsics.floatTrunc",FunctionType::get(asResultType(type),{type}),{operand}))
		EMIT_FP_UNARY_OP(nearest,emitRuntimeIntrinsic("wavmIntrinsics.floatNearest",FunctionType::get(asResultType(type),{type}),{operand}))
		EMIT_INT_UNARY_OP(trunc_s_f32,emitRuntimeIntrinsic("wavmIntrinsics.floatToSignedInt",FunctionType::get(asResultType(type),{ValueType::f32}),{operand}))
		EMIT_INT_UNARY_OP(trunc_s_f64,emitRuntimeIntrinsic("wavmIntrinsics.floatToSignedInt",FunctionType::get(asResultType(type),{ValueType::f64}),{operand}))
		EMIT_INT_UNARY_OP(trunc_u_f32,emitRuntimeIntrinsic("wavmIntrinsics.floatToUnsignedInt",FunctionType::get(asResultType(type),{ValueType::f32}),{operand}))
		EMIT_INT_UNARY_OP(trunc_u_f64,emitRuntimeIntrinsic("wavmIntrinsics.floatToUnsignedInt",FunctionType::get(asResultType(type),{ValueType::f64}),{operand}))
	};
	
	// A do-nothing visitor used to decode past unreachable operators (but supporting logging, and passing the end operator through).
	struct UnreachableOpVisitor
	{
		typedef void Result;

		UnreachableOpVisitor(EmitFunctionContext& inContext): context(inContext), unreachableControlDepth(0) {}
		#define VISIT_OP(opcode,name,nameString,Imm,...) void name(Imm imm) {}
		ENUM_NONCONTROL_OPERATORS(VISIT_OP)
		VISIT_OP(_,unknown,"unknown",Opcode)
		#undef VISIT_OP

		// Keep track of control structure nesting level in unreachable code, so we know when we reach the end of the unreachable code.
		void block(ControlStructureImm) { ++unreachableControlDepth; }
		void loop(ControlStructureImm) { ++unreachableControlDepth; }
		void if_(ControlStructureImm) { ++unreachableControlDepth; }

		// If an else or end opcode would signal an end to the unreachable code, then pass it through to the IR emitter.
		void else_(NoImm imm)
		{
			if(!unreachableControlDepth) { context.else_(imm); }
		}
		void end(NoImm imm)
		{
			if(!unreachableControlDepth) { context.end(imm); }
			else { --unreachableControlDepth; }
		}

	private:
		EmitFunctionContext& context;
		Uptr unreachableControlDepth;
	};

	void EmitFunctionContext::emit()
	{
		// Create the return basic block, and push the root control context for the function.
		auto returnBlock = llvm::BasicBlock::Create(context,"return",llvmFunction);
		auto returnPHI = createPHI(returnBlock,functionType->ret);
		pushControlStack(ControlContext::Type::function,functionType->ret,returnBlock,returnPHI);
		pushBranchTarget(functionType->ret,returnBlock,returnPHI);

		// Create an initial basic block for the function.
		auto entryBasicBlock = llvm::BasicBlock::Create(context,"entry",llvmFunction);
		irBuilder.SetInsertPoint(entryBasicBlock);

		// If enabled, emit a call to the WAVM function enter hook (for debugging).
		if(ENABLE_FUNCTION_ENTER_EXIT_HOOKS)
		{
			emitRuntimeIntrinsic(
				"wavmIntrinsics.debugEnterFunction",
				FunctionType::get(ResultType::none,{ValueType::i64}),
				{emitLiteral(reinterpret_cast<U64>(functionInstance))}
				);
		}

		// Create and initialize allocas for all the locals and parameters.
		auto llvmArgIt = llvmFunction->arg_begin();
		for(Uptr localIndex = 0;localIndex < functionType->parameters.size() + functionDef.nonParameterLocalTypes.size();++localIndex)
		{
			auto localType = localIndex < functionType->parameters.size()
				? functionType->parameters[localIndex]
				: functionDef.nonParameterLocalTypes[localIndex - functionType->parameters.size()];
			auto localPointer = irBuilder.CreateAlloca(asLLVMType(localType),nullptr,"");
			localPointers.push_back(localPointer);

			if(localIndex < functionType->parameters.size())
			{
				// Copy the parameter value into the local that stores it.
				irBuilder.CreateStore((llvm::Argument*)&(*llvmArgIt),localPointer);
				++llvmArgIt;
			}
			else
			{
				// Initialize non-parameter locals to zero.
				irBuilder.CreateStore(typedZeroConstants[(Uptr)localType],localPointer);
			}
		}

		// Decode the WebAssembly opcodes and emit LLVM IR for them.
		OperatorDecoderStream decoder(functionDef.code);
		UnreachableOpVisitor unreachableOpVisitor(*this);
		OperatorPrinter operatorPrinter(module,functionDef);
		while(decoder && controlStack.size())
		{
			if(ENABLE_LOGGING)
			{
				logOperator(decoder.decodeOpWithoutConsume(operatorPrinter));
			}

			if(controlStack.back().isReachable) { decoder.decodeOp(*this); }
			else { decoder.decodeOp(unreachableOpVisitor); }
		};
		WAVM_ASSERT_THROW(irBuilder.GetInsertBlock() == returnBlock);
		
		// If enabled, emit a call to the WAVM function enter hook (for debugging).
		if(ENABLE_FUNCTION_ENTER_EXIT_HOOKS)
		{
			emitRuntimeIntrinsic(
				"wavmIntrinsics.debugExitFunction",
				FunctionType::get(ResultType::none,{ValueType::i64}),
				{emitLiteral(reinterpret_cast<U64>(functionInstance))}
				);
		}

		// Emit the function return.
		if(functionType->ret == ResultType::none) { irBuilder.CreateRetVoid(); }
		else { irBuilder.CreateRet(pop()); }
	}

	llvm::Module* EmitModuleContext::emit()
	{
		Timing::Timer emitTimer;

		// Create literals for the default memory base and mask.
		if(moduleInstance->defaultMemory)
		{
			defaultMemoryBase = emitLiteralPointer(moduleInstance->defaultMemory->baseAddress,llvmI8PtrType);
			const Uptr defaultMemoryEndOffsetValue = Uptr(moduleInstance->defaultMemory->endOffset);
			defaultMemoryEndOffset = emitLiteral(defaultMemoryEndOffsetValue);
		}
		else { defaultMemoryBase = defaultMemoryEndOffset = nullptr; }

		// Set up the LLVM values used to access the global table.
		if(moduleInstance->defaultTable)
		{
			auto tableElementType = llvm::StructType::get(context,{
				llvmI8PtrType,
				llvmI8PtrType
				});
			defaultTablePointer = emitLiteralPointer(moduleInstance->defaultTable->baseAddress,tableElementType->getPointerTo());
			defaultTableMaxElementIndex = emitLiteral(((Uptr)moduleInstance->defaultTable->endOffset)/sizeof(TableInstance::FunctionElement));
		}
		else
		{
			defaultTablePointer = defaultTableMaxElementIndex = nullptr;
		}

		// Create LLVM pointer constants for the module's imported functions.
		for(Uptr functionIndex = 0;functionIndex < module.functions.imports.size();++functionIndex)
		{
			const FunctionInstance* functionInstance = moduleInstance->functions[functionIndex];
			importedFunctionPointers.push_back(emitLiteralPointer(functionInstance->nativeFunction,asLLVMType(functionInstance->type)->getPointerTo()));
		}

		// Create LLVM pointer constants for the module's globals.
		for(auto global : moduleInstance->globals)
		{ globalPointers.push_back(emitLiteralPointer(&global->value,asLLVMType(global->type.valueType)->getPointerTo())); }
		
		// Create the LLVM functions.
		functionDefs.resize(module.functions.defs.size());
		for(Uptr functionDefIndex = 0;functionDefIndex < module.functions.defs.size();++functionDefIndex)
		{
			auto llvmFunctionType = asLLVMType(module.types[module.functions.defs[functionDefIndex].type.index]);
			auto externalName = getExternalFunctionName(moduleInstance,functionDefIndex);
			functionDefs[functionDefIndex] = llvm::Function::Create(llvmFunctionType,llvm::Function::ExternalLinkage,externalName,llvmModule);
		}

		// Compile each function in the module.
		for(Uptr functionDefIndex = 0;functionDefIndex < module.functions.defs.size();++functionDefIndex)
		{ EmitFunctionContext(*this,module,module.functions.defs[functionDefIndex],moduleInstance->functionDefs[functionDefIndex],functionDefs[functionDefIndex]).emit(); }

		Timing::logRatePerSecond("Emitted LLVM IR",emitTimer,(F64)llvmModule->size(),"functions");

		return llvmModule;
	}

	llvm::Module* emitModule(const Module& module,ModuleInstance* moduleInstance)
	{
		return EmitModuleContext(module,moduleInstance).emit();
	}
}
