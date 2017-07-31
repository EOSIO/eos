#include "LLVMJIT.h"
#include "Inline/BasicTypes.h"
#include "Inline/Timing.h"
#include "Logging/Logging.h"
#include "RuntimePrivate.h"

#ifdef _DEBUG
	// This needs to be 1 to allow debuggers such as Visual Studio to place breakpoints and step through the JITed code.
	#define USE_WRITEABLE_JIT_CODE_PAGES 1

	#define DUMP_UNOPTIMIZED_MODULE 1
	#define VERIFY_MODULE 1
	#define DUMP_OPTIMIZED_MODULE 1
#else
	#define USE_WRITEABLE_JIT_CODE_PAGES 0
	#define DUMP_UNOPTIMIZED_MODULE 0
	#define VERIFY_MODULE 0
	#define DUMP_OPTIMIZED_MODULE 0
#endif

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
	
	#if ENABLE_SIMD_PROTOTYPE
	llvm::Type* llvmI8x16Type;
	llvm::Type* llvmI16x8Type;
	llvm::Type* llvmI32x4Type;
	llvm::Type* llvmI64x2Type;
	llvm::Type* llvmF32x4Type;
	llvm::Type* llvmF64x2Type;
	llvm::Type* llvmB8x16Type;
	llvm::Type* llvmB16x8Type;
	llvm::Type* llvmB32x4Type;
	llvm::Type* llvmB64x2Type;
	#endif

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
		, isFinalized(false)
		, codeSection({0})
		, readOnlySection({0})
		, readWriteSection({0})
		, hasRegisteredEHFrames(false)
		{}
		virtual ~UnitMemoryManager() override
		{
			// Deregister the exception handling frame info.
			if(hasRegisteredEHFrames)
			{
				hasRegisteredEHFrames = false;
				deregisterEHFrames(ehFramesAddr,ehFramesLoadAddr,ehFramesNumBytes);
			}

			// Decommit the image pages, but leave them reserved to catch any references to them that might erroneously remain.
			Platform::decommitVirtualPages(imageBaseAddress,numAllocatedImagePages);
		}
		
		void registerEHFrames(U8* addr, U64 loadAddr,uintptr_t numBytes) override
		{
			llvm::RTDyldMemoryManager::registerEHFrames(addr,loadAddr,numBytes);
			hasRegisteredEHFrames = true;
			ehFramesAddr = addr;
			ehFramesLoadAddr = loadAddr;
			ehFramesNumBytes = numBytes;
		}
		void deregisterEHFrames(U8* addr, U64 loadAddr,uintptr_t numBytes) override
		{
			llvm::RTDyldMemoryManager::deregisterEHFrames(addr,loadAddr,numBytes);
		}
		
		virtual bool needsToReserveAllocationSpace() override { return true; }
		virtual void reserveAllocationSpace(uintptr_t numCodeBytes,U32 codeAlignment,uintptr_t numReadOnlyBytes,U32 readOnlyAlignment,uintptr_t numReadWriteBytes,U32 readWriteAlignment) override
		{
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
			assert(!isFinalized);
			isFinalized = true;
			// Set the requested final memory access for each section's pages.
			const Platform::MemoryAccess codeAccess = USE_WRITEABLE_JIT_CODE_PAGES ? Platform::MemoryAccess::ReadWriteExecute : Platform::MemoryAccess::Execute;
			if(codeSection.numPages && !Platform::setVirtualPageAccess(codeSection.baseAddress,codeSection.numPages,codeAccess)) { return false; }
			if(readOnlySection.numPages && !Platform::setVirtualPageAccess(readOnlySection.baseAddress,readOnlySection.numPages,Platform::MemoryAccess::ReadOnly)) { return false; }
			if(readWriteSection.numPages && !Platform::setVirtualPageAccess(readWriteSection.baseAddress,readWriteSection.numPages,Platform::MemoryAccess::ReadWrite)) { return false; }
			return true;
		}
		virtual void invalidateInstructionCache()
		{
			// Invalidate the instruction cache for the whole image.
			llvm::sys::Memory::InvalidateInstructionCache(imageBaseAddress,numAllocatedImagePages << Platform::getPageSizeLog2());
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

		bool hasRegisteredEHFrames;
		U8* ehFramesAddr;
		U64 ehFramesLoadAddr;
		Uptr ehFramesNumBytes;

		U8* allocateBytes(Uptr numBytes,Uptr alignment,Section& section)
		{
			assert(section.baseAddress);
			assert(!(alignment & (alignment - 1)));
			assert(!isFinalized);
			
			// Allocate the section at the lowest uncommitted byte of image memory.
			U8* allocationBaseAddress = section.baseAddress + align(section.numCommittedBytes,alignment);
			assert(!(reinterpret_cast<Uptr>(allocationBaseAddress) & (alignment-1)));
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
		#ifdef _WIN32
			, pdataCopy(nullptr)
		#endif
		{
			objectLayer = llvm::make_unique<ObjectLayer>(NotifyLoadedFunctor(this),NotifyFinalizedFunctor(this));
			objectLayer->setProcessAllSections(true);
			compileLayer = llvm::make_unique<CompileLayer>(*objectLayer,llvm::orc::SimpleCompiler(*targetMachine));
		}
		~JITUnit()
		{
			compileLayer->removeModuleSet(handle);
			#ifdef _WIN32
				if(pdataCopy) { Platform::deregisterSEHUnwindInfo(reinterpret_cast<Uptr>(pdataCopy)); }
			#endif
		}

		void compile(llvm::Module* llvmModule);

		virtual void notifySymbolLoaded(const char* name,Uptr baseAddress,Uptr numBytes,std::map<U32,U32>&& offsetToOpIndexMap) = 0;

	private:
		
		// Functor that receives notifications when an object produced by the JIT is loaded.
		struct NotifyLoadedFunctor
		{
			JITUnit* jitUnit;
			NotifyLoadedFunctor(JITUnit* inJITUnit): jitUnit(inJITUnit) {}
			void operator()(
				const llvm::orc::ObjectLinkingLayerBase::ObjSetHandleT& objectSetHandle,
				const std::vector<std::unique_ptr<llvm::object::OwningBinary<llvm::object::ObjectFile>>>& objectSet,
				const std::vector<std::unique_ptr<llvm::RuntimeDyld::LoadedObjectInfo>>& loadedObjects
				);
		};
		
		// Functor that receives notifications when an object produced by the JIT is finalized.
		struct NotifyFinalizedFunctor
		{
			JITUnit* jitUnit;
			NotifyFinalizedFunctor(JITUnit* inJITUnit): jitUnit(inJITUnit) {}
			void operator()(const llvm::orc::ObjectLinkingLayerBase::ObjSetHandleT& objectSetHandle);
		};
		typedef llvm::orc::ObjectLinkingLayer<NotifyLoadedFunctor> ObjectLayer;
		typedef llvm::orc::IRCompileLayer<ObjectLayer> CompileLayer;

		UnitMemoryManager memoryManager;
		std::unique_ptr<ObjectLayer> objectLayer;
		std::unique_ptr<CompileLayer> compileLayer;
		CompileLayer::ModuleSetHandleT handle;
		bool shouldLogMetrics;

		struct LoadedObject
		{
			llvm::object::ObjectFile* object;
			llvm::RuntimeDyld::LoadedObjectInfo* loadedObject;
		};

		std::vector<LoadedObject> loadedObjects;

		#ifdef _WIN32
			U8* pdataCopy;
		#endif
	};

	// The JIT compilation unit for a WebAssembly module instance.
	struct JITModule : JITUnit, JITModuleBase
	{
		ModuleInstance* moduleInstance;

		std::vector<JITSymbol*> functionDefSymbols;

		JITModule(ModuleInstance* inModuleInstance): moduleInstance(inModuleInstance) {}
		~JITModule() override
		{
			// Delete the module's symbols, and remove them from the global address-to-symbol map.
			Platform::Lock addressToSymbolMapLock(addressToSymbolMapMutex);
			for(auto symbol : functionDefSymbols)
			{
				addressToSymbolMap.erase(addressToSymbolMap.find(symbol->baseAddress + symbol->numBytes));
				delete symbol;
			}
		}

		void notifySymbolLoaded(const char* name,Uptr baseAddress,Uptr numBytes,std::map<U32,U32>&& offsetToOpIndexMap) override
		{
			// Save the address range this function was loaded at for future address->symbol lookups.
			Uptr functionDefIndex;
			if(getFunctionIndexFromExternalName(name,functionDefIndex))
			{
				assert(moduleInstance);
				assert(functionDefIndex < moduleInstance->functionDefs.size());
				FunctionInstance* functionInstance = moduleInstance->functionDefs[functionDefIndex];
				auto symbol = new JITSymbol(functionInstance,baseAddress,numBytes,std::move(offsetToOpIndexMap));
				functionDefSymbols.push_back(symbol);
				functionInstance->nativeFunction = reinterpret_cast<void*>(baseAddress);

				{
					Platform::Lock addressToSymbolMapLock(addressToSymbolMapMutex);
					addressToSymbolMap[baseAddress + numBytes] = symbol;
				}
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
			assert(!strcmp(name,"invokeThunk"));
			symbol = new JITSymbol(functionType,baseAddress,numBytes,std::move(offsetToOpIndexMap));
		}
	};
	
	// Used to override LLVM's default behavior of looking up unresolved symbols in DLL exports.
	struct NullResolver : llvm::JITSymbolResolver
	{
		static NullResolver singleton;
		virtual llvm::JITSymbol findSymbol(const std::string& name) override;
		virtual llvm::JITSymbol findSymbolInLogicalDylib(const std::string& name) override;
	};
	
	NullResolver NullResolver::singleton;
	llvm::JITSymbol NullResolver::findSymbol(const std::string& name)
	{
		// Allow __chkstk through: the LLVM X86 code generator adds calls to it when allocating more than 4KB of stack space.
		if(name == "__chkstk")
		{
			void *addr = llvm::sys::DynamicLibrary::SearchForAddressOfSymbol(name);
			if (addr) { return llvm::JITSymbol(reinterpret_cast<Uptr>(addr),llvm::JITSymbolFlags::None); }
		}

		Log::printf(Log::Category::error,"LLVM generated code referenced external symbol: %s\n",name.c_str());
		Errors::unreachable();
	}
	llvm::JITSymbol NullResolver::findSymbolInLogicalDylib(const std::string& name) { return llvm::JITSymbol(nullptr); }

	void JITUnit::NotifyLoadedFunctor::operator()(
		const llvm::orc::ObjectLinkingLayerBase::ObjSetHandleT& objectSetHandle,
		const std::vector<std::unique_ptr<llvm::object::OwningBinary<llvm::object::ObjectFile>>>& objectSet,
		const std::vector<std::unique_ptr<llvm::RuntimeDyld::LoadedObjectInfo>>& loadedObjects
		)
	{
		assert(objectSet.size() == loadedObjects.size());
		for(Uptr objectIndex = 0;objectIndex < loadedObjects.size();++objectIndex)
		{
			llvm::object::ObjectFile* object = objectSet[objectIndex].get()->getBinary();
			llvm::RuntimeDyld::LoadedObjectInfo* loadedObject = loadedObjects[objectIndex].get();
			
			// Make a copy of the loaded object info for use by the finalizer.
			jitUnit->loadedObjects.push_back({object,loadedObject});

			#ifdef _WIN32
				// On Windows, look for .pdata and .xdata sections containing information about how to unwind the stack.
				// This needs to be done before the below emitAndFinalize call, which will incorrectly apply relocations to the unwind info.
				
				// Find the pdata section.
				llvm::object::SectionRef pdataSection;
				for(auto section : object->sections())
				{
					llvm::StringRef sectionName;
					if(!section.getName(sectionName))
					{
						if(sectionName == ".pdata") { pdataSection = section; break; }
					}
				}

				// Pass the pdata section to the platform to register unwind info.
				if(pdataSection.getObject())
				{
					const Uptr imageBaseAddress = reinterpret_cast<Uptr>(jitUnit->memoryManager.getImageBaseAddress());
					const Uptr pdataSectionLoadAddress = (Uptr)loadedObject->getSectionLoadAddress(pdataSection);
					
					// The LLVM COFF dynamic loader doesn't handle the image-relative relocations used by the pdata section,
					// and overwrites those values with o: https://github.com/llvm-mirror/llvm/blob/e84d8c12d5157a926db15976389f703809c49aa5/lib/ExecutionEngine/RuntimeDyld/Targets/RuntimeDyldCOFFX86_64.h#L96
					// This works around that by making a copy of the pdata section and doing the pdata relocations manually.
					jitUnit->pdataCopy = new U8[pdataSection.getSize()];
					memcpy(jitUnit->pdataCopy,reinterpret_cast<U8*>(pdataSectionLoadAddress),pdataSection.getSize());

					for(auto pdataRelocIt : pdataSection.relocations())
					{
						// Only handle type 3 (IMAGE_REL_AMD64_ADDR32NB).
						if(pdataRelocIt.getType() != 3) { Errors::unreachable(); }

						const auto symbol = pdataRelocIt.getSymbol();
						const U64 symbolAddress = symbol->getAddress().get();
						const llvm::object::section_iterator symbolSection = symbol->getSection().get();
						U32* valueToRelocate = (U32*)(jitUnit->pdataCopy + pdataRelocIt.getOffset());
						const U64 relocatedValue64 =
							+ (symbolAddress - symbolSection->getAddress())
							+ loadedObject->getSectionLoadAddress(*symbolSection)
							+ *valueToRelocate
							- imageBaseAddress;
						if(relocatedValue64 > UINT32_MAX) { Errors::unreachable(); }
						*valueToRelocate = (U32)relocatedValue64;
					}

					Platform::registerSEHUnwindInfo(imageBaseAddress,reinterpret_cast<Uptr>(jitUnit->pdataCopy),pdataSection.getSize());
				}
			#endif
		}

	}

	void JITUnit::NotifyFinalizedFunctor::operator()(const llvm::orc::ObjectLinkingLayerBase::ObjSetHandleT& objectSetHandle)
	{
		for(Uptr objectIndex = 0;objectIndex < jitUnit->loadedObjects.size();++objectIndex)
		{
			llvm::object::ObjectFile* object = jitUnit->loadedObjects[objectIndex].object;
			llvm::RuntimeDyld::LoadedObjectInfo* loadedObject = jitUnit->loadedObjects[objectIndex].loadedObject;

			// Create a DWARF context to interpret the debug information in this compilation unit.
			auto dwarfContext = llvm::make_unique<llvm::DWARFContextInMemory>(*object,loadedObject);

			// Iterate over the functions in the loaded object.
			for(auto symbolSizePair : llvm::object::computeSymbolSizes(*object))
			{
				auto symbol = symbolSizePair.first;
				auto name = symbol.getName();
				auto address = symbol.getAddress();
				if(	symbol.getType() && symbol.getType().get() == llvm::object::SymbolRef::ST_Function
				&&	name
				&&	address)
				{
					// Compute the address the functions was loaded at.
					Uptr loadedAddress = *address;
					auto symbolSection = symbol.getSection();
					if(symbolSection)
					{
						loadedAddress += (Uptr)loadedObject->getSectionLoadAddress(*symbolSection.get());
					}

					// Get the DWARF line info for this symbol, which maps machine code addresses to WebAssembly op indices.
					llvm::DILineInfoTable lineInfoTable = dwarfContext->getLineInfoForAddressRange(loadedAddress,symbolSizePair.second);
					std::map<U32,U32> offsetToOpIndexMap;
					for(auto lineInfo : lineInfoTable) { offsetToOpIndexMap.emplace(U32(lineInfo.first - loadedAddress),lineInfo.second.Line); }

					// Notify the JIT unit that the symbol was loaded.
					jitUnit->notifySymbolLoaded(name->data(),loadedAddress,symbolSizePair.second,std::move(offsetToOpIndexMap));
				}
			}
		}

		jitUnit->loadedObjects.clear();
	}

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
		
		if(shouldLogMetrics)
		{
			Timing::logRatePerSecond("Optimized LLVM module",optimizationTimer,(F64)llvmModule->size(),"functions");
		}

		if(DUMP_OPTIMIZED_MODULE) { printModule(llvmModule,"llvmOptimizedDump"); }

		// Pass the module to the JIT compiler.
		Timing::Timer machineCodeTimer;
		handle = compileLayer->addModuleSet(
			std::vector<llvm::Module*>{llvmModule},
			&memoryManager,
			&NullResolver::singleton);
		compileLayer->emitAndFinalize(handle);

		if(shouldLogMetrics)
		{
			Timing::logRatePerSecond("Generated machine code",machineCodeTimer,(F64)llvmModule->size(),"functions");
		}
		
		delete llvmModule;
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
		assert(functionDefIndex < moduleInstance->functionDefs.size());
		return "wasmFunc" + std::to_string(functionDefIndex)
			+ "_" + moduleInstance->functionDefs[functionDefIndex]->debugName;
	}

	bool getFunctionIndexFromExternalName(const char* externalName,Uptr& outFunctionDefIndex)
	{
		if(!strncmp(externalName,"wasmFunc",8))
		{
			char* numberEnd = nullptr;
			outFunctionDefIndex = std::strtoull(externalName + 8,&numberEnd,10);
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

		assert(jitUnit->symbol);
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
		llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

		auto targetTriple = llvm::sys::getProcessTriple();
		#ifdef __APPLE__
			// Didn't figure out exactly why, but this works around a problem with the MacOS dynamic loader. Without it,
			// our symbols can't be found in the JITed object file.
			targetTriple += "-elf";
		#endif
		targetMachine = llvm::EngineBuilder().selectTarget(llvm::Triple(targetTriple),"","",llvm::SmallVector<std::string,0>());

		llvmI8Type = llvm::Type::getInt8Ty(context);
		llvmI16Type = llvm::Type::getInt16Ty(context);
		llvmI32Type = llvm::Type::getInt32Ty(context);
		llvmI64Type = llvm::Type::getInt64Ty(context);
		llvmF32Type = llvm::Type::getFloatTy(context);
		llvmF64Type = llvm::Type::getDoubleTy(context);
		llvmVoidType = llvm::Type::getVoidTy(context);
		llvmBoolType = llvm::Type::getInt1Ty(context);
		llvmI8PtrType = llvmI8Type->getPointerTo();
		
		#if ENABLE_SIMD_PROTOTYPE
		llvmI8x16Type = llvm::VectorType::get(llvmI8Type,16);
		llvmI16x8Type = llvm::VectorType::get(llvmI16Type,8);
		llvmI32x4Type = llvm::VectorType::get(llvmI32Type,4);
		llvmI64x2Type = llvm::VectorType::get(llvmI64Type,2);
		llvmF32x4Type = llvm::VectorType::get(llvmF32Type,4);
		llvmF64x2Type = llvm::VectorType::get(llvmF64Type,2);
		llvmB8x16Type = llvm::VectorType::get(llvmBoolType,16);
		llvmB16x8Type = llvm::VectorType::get(llvmBoolType,8);
		llvmB32x4Type = llvm::VectorType::get(llvmBoolType,4);
		llvmB64x2Type = llvm::VectorType::get(llvmBoolType,2);
		#endif

		llvmResultTypes[(Uptr)ResultType::none] = llvm::Type::getVoidTy(context);
		llvmResultTypes[(Uptr)ResultType::i32] = llvmI32Type;
		llvmResultTypes[(Uptr)ResultType::i64] = llvmI64Type;
		llvmResultTypes[(Uptr)ResultType::f32] = llvmF32Type;
		llvmResultTypes[(Uptr)ResultType::f64] = llvmF64Type;

		#if ENABLE_SIMD_PROTOTYPE
		llvmResultTypes[(Uptr)ResultType::v128] = llvmI64x2Type;
		#endif

		// Create zero constants of each type.
		typedZeroConstants[(Uptr)ValueType::any] = nullptr;
		typedZeroConstants[(Uptr)ValueType::i32] = emitLiteral((U32)0);
		typedZeroConstants[(Uptr)ValueType::i64] = emitLiteral((U64)0);
		typedZeroConstants[(Uptr)ValueType::f32] = emitLiteral((F32)0.0f);
		typedZeroConstants[(Uptr)ValueType::f64] = emitLiteral((F64)0.0);

		#if ENABLE_SIMD_PROTOTYPE
		typedZeroConstants[(Uptr)ValueType::v128] = llvm::ConstantVector::get({typedZeroConstants[(Uptr)ValueType::i64],typedZeroConstants[(Uptr)ValueType::i64]});
		#endif
	}
}
