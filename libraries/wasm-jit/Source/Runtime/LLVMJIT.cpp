#include "LLVMJIT.h"
#include "Inline/BasicTypes.h"
#include "Inline/Timing.h"
#include "Logging/Logging.h"
#include "RuntimePrivate.h"
#include "IR/Validate.h"

#if LLVM_VERSION_MAJOR == 7
namespace llvm { namespace orc {
	using LegacyRTDyldObjectLinkingLayer = RTDyldObjectLinkingLayer;
	template<typename A, typename B>
	using LegacyIRCompileLayer = IRCompileLayer<A, B>;
}}
#endif

#ifdef _DEBUG
	// This needs to be 1 to allow debuggers such as Visual Studio to place breakpoints and step through the JITed code.
	#define USE_WRITEABLE_JIT_CODE_PAGES 1

	#define DUMP_UNOPTIMIZED_MODULE 1
	#define VERIFY_MODULE 1
	#define DUMP_OPTIMIZED_MODULE 1
	#define PRINT_DISASSEMBLY 0
#else
	#define USE_WRITEABLE_JIT_CODE_PAGES 0
	#define DUMP_UNOPTIMIZED_MODULE 0
	#define VERIFY_MODULE 0
	#define DUMP_OPTIMIZED_MODULE 0
	#define PRINT_DISASSEMBLY 0
#endif

#include "llvm-c/Disassembler.h"
void disassembleFunction(U8* bytes,Uptr numBytes)
{
	LLVMDisasmContextRef disasmRef = LLVMCreateDisasm(llvm::sys::getProcessTriple().c_str(),nullptr,0,nullptr,nullptr);

	U8* nextByte = bytes;
	Uptr numBytesRemaining = numBytes;
	while(numBytesRemaining)
	{
		char instructionBuffer[256];
		const Uptr numInstructionBytes = LLVMDisasmInstruction(
			disasmRef,
			nextByte,
			numBytesRemaining,
			reinterpret_cast<Uptr>(nextByte),
			instructionBuffer,
			sizeof(instructionBuffer)
			);
		if(numInstructionBytes == 0 || numInstructionBytes > numBytesRemaining)
			break;
		numBytesRemaining -= numInstructionBytes;
		nextByte += numInstructionBytes;

		printf("\t\t%s\n",instructionBuffer);
	};

	LLVMDisasmDispose(disasmRef);
}

namespace LLVMJIT
{
	llvm::LLVMContext context;
	llvm::TargetMachine* targetMachine = nullptr;
	llvm::Type* llvmResultTypes[(Uptr)ResultType::num];

	llvm::Type* llvmI8Type;
	llvm::Type* llvmI16Type;
	llvm::Type* llvmI32Type;
	llvm::Type* llvmI64Type;
	llvm::Type* llvmF32Type;
	llvm::Type* llvmF64Type;
	llvm::Type* llvmVoidType;
	llvm::Type* llvmBoolType;
	llvm::Type* llvmI8PtrType;

	llvm::Constant* typedZeroConstants[(Uptr)ValueType::num];
	
	// A map from address to loaded JIT symbols.
	Platform::Mutex* addressToSymbolMapMutex = Platform::createMutex();
	std::map<Uptr,struct JITSymbol*> addressToSymbolMap;

	// A map from function types to function indices in the invoke thunk unit.
	std::map<const FunctionType*,struct JITSymbol*> invokeThunkTypeToSymbolMap;

	// Information about a JIT symbol, used to map instruction pointers to descriptive names.
	struct JITSymbol
	{
		enum class Type
		{
			functionInstance,
			invokeThunk
		};
		Type type;
		union
		{
			FunctionInstance* functionInstance;
			const FunctionType* invokeThunkType;
		};
		Uptr baseAddress;
		Uptr numBytes;
		std::map<U32,U32> offsetToOpIndexMap;
		
		JITSymbol(FunctionInstance* inFunctionInstance,Uptr inBaseAddress,Uptr inNumBytes,std::map<U32,U32>&& inOffsetToOpIndexMap)
		: type(Type::functionInstance), functionInstance(inFunctionInstance), baseAddress(inBaseAddress), numBytes(inNumBytes), offsetToOpIndexMap(inOffsetToOpIndexMap) {}

		JITSymbol(const FunctionType* inInvokeThunkType,Uptr inBaseAddress,Uptr inNumBytes,std::map<U32,U32>&& inOffsetToOpIndexMap)
		: type(Type::invokeThunk), invokeThunkType(inInvokeThunkType), baseAddress(inBaseAddress), numBytes(inNumBytes), offsetToOpIndexMap(inOffsetToOpIndexMap) {}
	};

	// Allocates memory for the LLVM object loader.
	struct UnitMemoryManager : llvm::RTDyldMemoryManager
	{
		UnitMemoryManager()
		: imageBaseAddress(nullptr)
		, numAllocatedImagePages(0)
		, isFinalized(false)
		, codeSection({0})
		, readOnlySection({0})
		, readWriteSection({0})
		{}
		virtual ~UnitMemoryManager() override
		{
			// Decommit the image pages, but leave them reserved to catch any references to them that might erroneously remain.
			if(numAllocatedImagePages)
				Platform::decommitVirtualPages(imageBaseAddress,numAllocatedImagePages);
		}
		
		virtual bool needsToReserveAllocationSpace() override { return true; }
		virtual void reserveAllocationSpace(uintptr_t numCodeBytes,U32 codeAlignment,uintptr_t numReadOnlyBytes,U32 readOnlyAlignment,uintptr_t numReadWriteBytes,U32 readWriteAlignment) override
		{
			//in llvm7+ it's signaling unwinding frames as being R/W on macos. This appears to be due to Mach-O not supporting read-only data sections?
			//Let's only clutch in this safety guard for Linux
#ifndef __APPLE__
			if(numReadWriteBytes)
				 Runtime::causeException(Exception::Cause::outOfMemory);
#endif
			// Calculate the number of pages to be used by each section.
			codeSection.numPages = shrAndRoundUp(numCodeBytes,Platform::getPageSizeLog2());
			readOnlySection.numPages = shrAndRoundUp(numReadOnlyBytes,Platform::getPageSizeLog2());
			readWriteSection.numPages = shrAndRoundUp(numReadWriteBytes,Platform::getPageSizeLog2());
			numAllocatedImagePages = codeSection.numPages + readOnlySection.numPages + readWriteSection.numPages;
			if(numAllocatedImagePages)
			{
				// Reserve enough contiguous pages for all sections.
				imageBaseAddress = Platform::allocateVirtualPages(numAllocatedImagePages);
				if(!imageBaseAddress || !Platform::commitVirtualPages(imageBaseAddress,numAllocatedImagePages)) { Errors::fatal("memory allocation for JIT code failed"); }
				codeSection.baseAddress = imageBaseAddress;
				readOnlySection.baseAddress = codeSection.baseAddress + (codeSection.numPages << Platform::getPageSizeLog2());
				readWriteSection.baseAddress = readOnlySection.baseAddress + (readOnlySection.numPages << Platform::getPageSizeLog2());
			}
		}
		virtual U8* allocateCodeSection(uintptr_t numBytes,U32 alignment,U32 sectionID,llvm::StringRef sectionName) override
		{
			return allocateBytes((Uptr)numBytes,alignment,codeSection);
		}
		virtual U8* allocateDataSection(uintptr_t numBytes,U32 alignment,U32 sectionID,llvm::StringRef SectionName,bool isReadOnly) override
		{
			return allocateBytes((Uptr)numBytes,alignment,isReadOnly ? readOnlySection : readWriteSection);
		}
		virtual bool finalizeMemory(std::string* ErrMsg = nullptr) override
		{
			WAVM_ASSERT_THROW(!isFinalized);
			isFinalized = true;
			// Set the requested final memory access for each section's pages.
			const Platform::MemoryAccess codeAccess = USE_WRITEABLE_JIT_CODE_PAGES ? Platform::MemoryAccess::ReadWriteExecute : Platform::MemoryAccess::Execute;
			if(codeSection.numPages && !Platform::setVirtualPageAccess(codeSection.baseAddress,codeSection.numPages,codeAccess)) { return false; }
			if(readOnlySection.numPages && !Platform::setVirtualPageAccess(readOnlySection.baseAddress,readOnlySection.numPages,Platform::MemoryAccess::ReadOnly)) { return false; }
			if(readWriteSection.numPages && !Platform::setVirtualPageAccess(readWriteSection.baseAddress,readWriteSection.numPages,Platform::MemoryAccess::ReadWrite)) { return false; }
			return true;
		}

		U8* getImageBaseAddress() const { return imageBaseAddress; }

	private:
		struct Section
		{
			U8* baseAddress;
			Uptr numPages;
			Uptr numCommittedBytes;
		};
		
		U8* imageBaseAddress;
		Uptr numAllocatedImagePages;
		bool isFinalized;

		Section codeSection;
		Section readOnlySection;
		Section readWriteSection;

		U8* allocateBytes(Uptr numBytes,Uptr alignment,Section& section)
		{
			WAVM_ASSERT_THROW(section.baseAddress);
			WAVM_ASSERT_THROW(!(alignment & (alignment - 1)));
			WAVM_ASSERT_THROW(!isFinalized);
			
			// Allocate the section at the lowest uncommitted byte of image memory.
			U8* allocationBaseAddress = section.baseAddress + align(section.numCommittedBytes,alignment);
			WAVM_ASSERT_THROW(!(reinterpret_cast<Uptr>(allocationBaseAddress) & (alignment-1)));
			section.numCommittedBytes = align(section.numCommittedBytes,alignment) + align(numBytes,alignment);

			// Check that enough space was reserved in the section.
			if(section.numCommittedBytes > (section.numPages << Platform::getPageSizeLog2())) { Errors::fatal("didn't reserve enough space in section"); }

			return allocationBaseAddress;
		}
		
		static Uptr align(Uptr size,Uptr alignment) { return (size + alignment - 1) & ~(alignment - 1); }
		static Uptr shrAndRoundUp(Uptr value,Uptr shift) { return (value + (Uptr(1)<<shift) - 1) >> shift; }

		UnitMemoryManager(const UnitMemoryManager&) = delete;
		void operator=(const UnitMemoryManager&) = delete;
	};

	// A unit of JIT compilation.
	// Encapsulates the LLVM JIT compilation pipeline but allows subclasses to define how the resulting code is used.
	struct JITUnit
	{
		JITUnit(bool inShouldLogMetrics = true)
		: shouldLogMetrics(inShouldLogMetrics)
		{
			objectLayer = llvm::make_unique<llvm::orc::LegacyRTDyldObjectLinkingLayer>(ES,[](llvm::orc::VModuleKey K) {
									return llvm::orc::LegacyRTDyldObjectLinkingLayer::Resources{
										std::make_shared<UnitMemoryManager>(), std::make_shared<llvm::orc::NullResolver>()
									};
							  },
							  [](llvm::orc::VModuleKey, const llvm::object::ObjectFile &Obj, const llvm::RuntimeDyld::LoadedObjectInfo &o) {
									//nothing to do
							  },
							  [this](llvm::orc::VModuleKey, const llvm::object::ObjectFile &Obj, const llvm::RuntimeDyld::LoadedObjectInfo &o) {
									for(auto symbolSizePair : llvm::object::computeSymbolSizes(Obj)) {
										auto symbol = symbolSizePair.first;
										auto name = symbol.getName();
										auto address = symbol.getAddress();
										if(symbol.getType() && symbol.getType().get() == llvm::object::SymbolRef::ST_Function && name && address) {
											Uptr loadedAddress = Uptr(*address);
											auto symbolSection = symbol.getSection();
											if(symbolSection)
												loadedAddress += (Uptr)o.getSectionLoadAddress(*symbolSection.get());
											//printf(">>> %s 0x%lX\n", symbolSizePair.first.getName()->data(), loadedAddress);
											std::map<U32,U32> offsetToOpIndexMap;
											notifySymbolLoaded(name->data(), loadedAddress, 0, std::move(offsetToOpIndexMap));
											//disassembleFunction((U8*)loadedAddress, symbolSizePair.second);
										}
									}
							  }
							  );
			objectLayer->setProcessAllSections(true);
			compileLayer = llvm::make_unique<CompileLayer>(*objectLayer,llvm::orc::SimpleCompiler(*targetMachine));
		}

		void compile(llvm::Module* llvmModule);

		virtual void notifySymbolLoaded(const char* name,Uptr baseAddress,Uptr numBytes,std::map<U32,U32>&& offsetToOpIndexMap) = 0;

	private:
		typedef llvm::orc::LegacyIRCompileLayer<llvm::orc::LegacyRTDyldObjectLinkingLayer, llvm::orc::SimpleCompiler> CompileLayer;

		llvm::orc::ExecutionSession ES;
		std::unique_ptr<llvm::orc::LegacyRTDyldObjectLinkingLayer> objectLayer;
		std::unique_ptr<CompileLayer> compileLayer;
		bool shouldLogMetrics;
	};

	// The JIT compilation unit for a WebAssembly module instance.
	struct JITModule : JITUnit, JITModuleBase
	{
		ModuleInstance* moduleInstance;

		std::vector<JITSymbol*> functionDefSymbols;

		JITModule(ModuleInstance* inModuleInstance): moduleInstance(inModuleInstance) {}
		~JITModule() override
		{
		}

		void notifySymbolLoaded(const char* name,Uptr baseAddress,Uptr numBytes,std::map<U32,U32>&& offsetToOpIndexMap) override
		{
			// Save the address range this function was loaded at for future address->symbol lookups.
			Uptr functionDefIndex;
			if(getFunctionIndexFromExternalName(name,functionDefIndex))
			{
				WAVM_ASSERT_THROW(moduleInstance);
				WAVM_ASSERT_THROW(functionDefIndex < moduleInstance->functionDefs.size());
				FunctionInstance* functionInstance = moduleInstance->functionDefs[functionDefIndex];
				functionInstance->nativeFunction = reinterpret_cast<void*>(baseAddress);
			}
		}
	};

	// The JIT compilation unit for a single invoke thunk.
	struct JITInvokeThunkUnit : JITUnit
	{
		const FunctionType* functionType;

		JITSymbol* symbol;

		JITInvokeThunkUnit(const FunctionType* inFunctionType): JITUnit(false), functionType(inFunctionType), symbol(nullptr) {}

		void notifySymbolLoaded(const char* name,Uptr baseAddress,Uptr numBytes,std::map<U32,U32>&& offsetToOpIndexMap) override
		{
#ifndef __APPLE__
			const char thunkPrefix[] = "invokeThunk";
#else
			const char thunkPrefix[] = "_invokeThunk";
#endif
			WAVM_ASSERT_THROW(!strcmp(name,thunkPrefix));
			symbol = new JITSymbol(functionType,baseAddress,numBytes,std::move(offsetToOpIndexMap));
		}
	};

	static Uptr printedModuleId = 0;

	void printModule(const llvm::Module* llvmModule,const char* filename)
	{
		std::error_code errorCode;
		std::string augmentedFilename = std::string(filename) + std::to_string(printedModuleId++) + ".ll";
		llvm::raw_fd_ostream dumpFileStream(augmentedFilename,errorCode,llvm::sys::fs::OpenFlags::F_Text);
		llvmModule->print(dumpFileStream,nullptr);
		Log::printf(Log::Category::debug,"Dumped LLVM module to: %s\n",augmentedFilename.c_str());
	}

	void JITUnit::compile(llvm::Module* llvmModule)
	{
		// Get a target machine object for this host, and set the module to use its data layout.
		llvmModule->setDataLayout(targetMachine->createDataLayout());

		// Verify the module.
		if(DUMP_UNOPTIMIZED_MODULE) { printModule(llvmModule,"llvmDump"); }
		if(VERIFY_MODULE)
		{
			std::string verifyOutputString;
			llvm::raw_string_ostream verifyOutputStream(verifyOutputString);
			if(llvm::verifyModule(*llvmModule,&verifyOutputStream))
			{ Errors::fatalf("LLVM verification errors:\n%s\n",verifyOutputString.c_str()); }
			Log::printf(Log::Category::debug,"Verified LLVM module\n");
		}

		// Run some optimization on the module's functions.
		Timing::Timer optimizationTimer;

		auto fpm = new llvm::legacy::FunctionPassManager(llvmModule);
		fpm->add(llvm::createPromoteMemoryToRegisterPass());
		fpm->add(llvm::createInstructionCombiningPass());
		fpm->add(llvm::createCFGSimplificationPass());
		fpm->add(llvm::createJumpThreadingPass());
		fpm->add(llvm::createConstantPropagationPass());
		fpm->doInitialization();

		for(auto functionIt = llvmModule->begin();functionIt != llvmModule->end();++functionIt)
		{ fpm->run(*functionIt); }
		delete fpm;

		if(DUMP_OPTIMIZED_MODULE) { printModule(llvmModule,"llvmOptimizedDump"); }

		// Pass the module to the JIT compiler.
		Timing::Timer machineCodeTimer;
		llvm::orc::VModuleKey K = ES.allocateVModule();
		std::unique_ptr<llvm::Module> mod(llvmModule);
		WAVM_ASSERT_THROW(!compileLayer->addModule(K, std::move(mod)));
		WAVM_ASSERT_THROW(!compileLayer->emitAndFinalize(K));
	}

	void instantiateModule(const IR::Module& module,ModuleInstance* moduleInstance)
	{
		// Emit LLVM IR for the module.
		auto llvmModule = emitModule(module,moduleInstance);

		// Construct the JIT compilation pipeline for this module.
		auto jitModule = new JITModule(moduleInstance);
		moduleInstance->jitModule = jitModule;

		// Compile the module.
		jitModule->compile(llvmModule);
	}

	std::string getExternalFunctionName(ModuleInstance* moduleInstance,Uptr functionDefIndex)
	{
		WAVM_ASSERT_THROW(functionDefIndex < moduleInstance->functionDefs.size());
		return "wasmFunc" + std::to_string(functionDefIndex)
			+ "_" + moduleInstance->functionDefs[functionDefIndex]->debugName;
	}

	bool getFunctionIndexFromExternalName(const char* externalName,Uptr& outFunctionDefIndex)
	{
#ifndef __APPLE__
		const char wasmFuncPrefix[] = "wasmFunc";
#else
		const char wasmFuncPrefix[] = "_wasmFunc";
#endif
		const Uptr numPrefixChars = sizeof(wasmFuncPrefix) - 1;
		if(!strncmp(externalName,wasmFuncPrefix,numPrefixChars))
		{
			char* numberEnd = nullptr;
			U64 functionDefIndex64 = std::strtoull(externalName + numPrefixChars,&numberEnd,10);
			if(functionDefIndex64 > UINTPTR_MAX) { return false; }
			outFunctionDefIndex = Uptr(functionDefIndex64);
			return true;
		}
		else { return false; }
	}

	bool describeInstructionPointer(Uptr ip,std::string& outDescription)
	{
		JITSymbol* symbol;
		{
			Platform::Lock addressToSymbolMapLock(addressToSymbolMapMutex);
			auto symbolIt = addressToSymbolMap.upper_bound(ip);
			if(symbolIt == addressToSymbolMap.end()) { return false; }
			symbol = symbolIt->second;
		}
		if(ip < symbol->baseAddress || ip >= symbol->baseAddress + symbol->numBytes) { return false; }

		switch(symbol->type)
		{
		case JITSymbol::Type::functionInstance:
			outDescription = symbol->functionInstance->debugName;
			if(!outDescription.size()) { outDescription = "<unnamed function>"; }
			break;
		case JITSymbol::Type::invokeThunk:
			outDescription = "<invoke thunk : " + asString(symbol->invokeThunkType) + ">";
			break;
		default: Errors::unreachable();
		};
		
		// Find the highest entry in the offsetToOpIndexMap whose offset is <= the symbol-relative IP.
		U32 ipOffset = (U32)(ip - symbol->baseAddress);
		Iptr opIndex = -1;
		for(auto offsetMapIt : symbol->offsetToOpIndexMap)
		{
			if(offsetMapIt.first <= ipOffset) { opIndex = offsetMapIt.second; }
			else { break; }
		}
		if(opIndex >= 0) { outDescription += " (op " + std::to_string(opIndex) + ")"; }
		return true;
	}

	InvokeFunctionPointer getInvokeThunk(const FunctionType* functionType)
	{
		// Reuse cached invoke thunks for the same function type.
		auto mapIt = invokeThunkTypeToSymbolMap.find(functionType);
		if(mapIt != invokeThunkTypeToSymbolMap.end()) { return reinterpret_cast<InvokeFunctionPointer>(mapIt->second->baseAddress); }

		auto llvmModule = new llvm::Module("",context);
		auto llvmFunctionType = llvm::FunctionType::get(
			llvmVoidType,
			{asLLVMType(functionType)->getPointerTo(),llvmI64Type->getPointerTo()},
			false);
		auto llvmFunction = llvm::Function::Create(llvmFunctionType,llvm::Function::ExternalLinkage,"invokeThunk",llvmModule);
		auto argIt = llvmFunction->args().begin();
		llvm::Value* functionPointer = &*argIt++;
		llvm::Value* argBaseAddress = &*argIt;
		auto entryBlock = llvm::BasicBlock::Create(context,"entry",llvmFunction);
		llvm::IRBuilder<> irBuilder(entryBlock);

		// Load the function's arguments from an array of 64-bit values at an address provided by the caller.
		std::vector<llvm::Value*> structArgLoads;
		for(Uptr parameterIndex = 0;parameterIndex < functionType->parameters.size();++parameterIndex)
		{
			structArgLoads.push_back(irBuilder.CreateLoad(
				irBuilder.CreatePointerCast(
					irBuilder.CreateInBoundsGEP(argBaseAddress,{emitLiteral((Uptr)parameterIndex)}),
					asLLVMType(functionType->parameters[parameterIndex])->getPointerTo()
					)
				));
		}

		// Call the llvm function with the actual implementation.
		auto returnValue = irBuilder.CreateCall(functionPointer,structArgLoads);

		// If the function has a return value, write it to the end of the argument array.
		if(functionType->ret != ResultType::none)
		{
			auto llvmResultType = asLLVMType(functionType->ret);
			irBuilder.CreateStore(
				returnValue,
				irBuilder.CreatePointerCast(
					irBuilder.CreateInBoundsGEP(argBaseAddress,{emitLiteral((Uptr)functionType->parameters.size())}),
					llvmResultType->getPointerTo()
					)
				);
		}

		irBuilder.CreateRetVoid();

		// Compile the invoke thunk.
		auto jitUnit = new JITInvokeThunkUnit(functionType);
		jitUnit->compile(llvmModule);

		WAVM_ASSERT_THROW(jitUnit->symbol);
		invokeThunkTypeToSymbolMap[functionType] = jitUnit->symbol;

		{
			Platform::Lock addressToSymbolMapLock(addressToSymbolMapMutex);
			addressToSymbolMap[jitUnit->symbol->baseAddress + jitUnit->symbol->numBytes] = jitUnit->symbol;
		}

		return reinterpret_cast<InvokeFunctionPointer>(jitUnit->symbol->baseAddress);
	}

	void init()
	{
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
		llvm::InitializeNativeTargetDisassembler();
		llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

		auto targetTriple = llvm::sys::getProcessTriple();

		targetMachine = llvm::EngineBuilder().selectTarget(
			llvm::Triple(targetTriple),"","",llvm::SmallVector<std::string,0>()
			);

		llvmI8Type = llvm::Type::getInt8Ty(context);
		llvmI16Type = llvm::Type::getInt16Ty(context);
		llvmI32Type = llvm::Type::getInt32Ty(context);
		llvmI64Type = llvm::Type::getInt64Ty(context);
		llvmF32Type = llvm::Type::getFloatTy(context);
		llvmF64Type = llvm::Type::getDoubleTy(context);
		llvmVoidType = llvm::Type::getVoidTy(context);
		llvmBoolType = llvm::Type::getInt1Ty(context);
		llvmI8PtrType = llvmI8Type->getPointerTo();

		llvmResultTypes[(Uptr)ResultType::none] = llvm::Type::getVoidTy(context);
		llvmResultTypes[(Uptr)ResultType::i32] = llvmI32Type;
		llvmResultTypes[(Uptr)ResultType::i64] = llvmI64Type;
		llvmResultTypes[(Uptr)ResultType::f32] = llvmF32Type;
		llvmResultTypes[(Uptr)ResultType::f64] = llvmF64Type;

		// Create zero constants of each type.
		typedZeroConstants[(Uptr)ValueType::any] = nullptr;
		typedZeroConstants[(Uptr)ValueType::i32] = emitLiteral((U32)0);
		typedZeroConstants[(Uptr)ValueType::i64] = emitLiteral((U64)0);
		typedZeroConstants[(Uptr)ValueType::f32] = emitLiteral((F32)0.0f);
		typedZeroConstants[(Uptr)ValueType::f64] = emitLiteral((F64)0.0);
	}
}
