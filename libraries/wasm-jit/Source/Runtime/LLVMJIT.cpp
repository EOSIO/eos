#include "LLVMJIT.h"
#include "Inline/BasicTypes.h"
#include "Inline/Timing.h"
#include "Logging/Logging.h"
#include "RuntimePrivate.h"
#include "IR/Validate.h"

#include <memory>

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
      WAVM_ASSERT_THROW(numInstructionBytes > 0);
      WAVM_ASSERT_THROW(numInstructionBytes <= numBytesRemaining);
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

	// Allocates memory for the LLVM object loader.
	struct UnitMemoryManager : llvm::RTDyldMemoryManager
	{
		UnitMemoryManager() {}
		virtual ~UnitMemoryManager() override
		{}

		void registerEHFrames(U8* addr, U64 loadAddr,uintptr_t numBytes) override {}
		void deregisterEHFrames() override {}
		
		virtual bool needsToReserveAllocationSpace() override { return true; }
		virtual void reserveAllocationSpace(uintptr_t numCodeBytes,U32 codeAlignment,uintptr_t numReadOnlyBytes,U32 readOnlyAlignment,uintptr_t numReadWriteBytes,U32 readWriteAlignment) override {
         code.reset(new std::vector<uint8_t>(numCodeBytes + numReadOnlyBytes + numReadWriteBytes)); //XXX c++17 up
         ptr = code->data();
      }
		virtual U8* allocateCodeSection(uintptr_t numBytes,U32 alignment,U32 sectionID,llvm::StringRef sectionName) override
		{
         return get_next_code_ptr(numBytes, alignment);
		}
		virtual U8* allocateDataSection(uintptr_t numBytes,U32 alignment,U32 sectionID,llvm::StringRef SectionName,bool isReadOnly) override
		{
         //printf("DATA sz:%lu a:%u sec:%s\n", numBytes, alignment, SectionName.data());
         if(SectionName == ".eh_frame") {
            dumpster.resize(numBytes);
            return dumpster.data();
         }
         WAVM_ASSERT_THROW(isReadOnly);

         return get_next_code_ptr(numBytes, alignment);
		}

      virtual bool finalizeMemory(std::string* ErrMsg = nullptr) override {
         code->resize(ptr - code->data());
         return true;
      }

      std::unique_ptr<std::vector<uint8_t>> code;
      uint8_t* ptr;

      std::vector<uint8_t> dumpster;

      U8* get_next_code_ptr(uintptr_t numBytes, U32 alignment) {
         //XXX we should probably assert if alignment is > 16 or std align because the std::vector is aligned that way
         uintptr_t p = (uintptr_t)ptr;
         p += alignment - 1LL;
         p &= ~(alignment - 1LL);
         uint8_t* this_section = (uint8_t*)p;
         ptr = this_section + numBytes;

         return this_section;
      }

		UnitMemoryManager(const UnitMemoryManager&) = delete;
		void operator=(const UnitMemoryManager&) = delete;
	};

	// The JIT compilation unit for a WebAssembly module instance.
	struct JITModule : JITModuleBase
	{
		ModuleInstance* moduleInstance;

		JITModule(ModuleInstance* inModuleInstance): moduleInstance(inModuleInstance) {
			objectLayer = llvm::make_unique<llvm::orc::LegacyRTDyldObjectLinkingLayer>(ES,[this](llvm::orc::VModuleKey K) {
                           return llvm::orc::LegacyRTDyldObjectLinkingLayer::Resources{
                              unitmemorymanager, std::make_shared<llvm::orc::NullResolver>()
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
                                 Uptr functionDefIndex;
			                        if(getFunctionIndexFromExternalName(name->data(),functionDefIndex))
                                    function_to_offsets[functionDefIndex] = loadedAddress-(uintptr_t)unitmemorymanager->code->data();
                                 ///disassembleFunction((U8*)loadedAddress, symbolSizePair.second);
                              }
                           }
                       }
                       );
			objectLayer->setProcessAllSections(true);
			compileLayer = llvm::make_unique<CompileLayer>(*objectLayer,llvm::orc::SimpleCompiler(*targetMachine));
		}

		void compile(llvm::Module* llvmModule);

      std::shared_ptr<UnitMemoryManager> unitmemorymanager = std::make_shared<UnitMemoryManager>();

		~JITModule() override
		{
		}
	private:
		typedef llvm::orc::LegacyIRCompileLayer<llvm::orc::LegacyRTDyldObjectLinkingLayer, llvm::orc::SimpleCompiler> CompileLayer;

      llvm::orc::ExecutionSession ES;
		std::unique_ptr<llvm::orc::LegacyRTDyldObjectLinkingLayer> objectLayer;
		std::unique_ptr<CompileLayer> compileLayer;
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

	void JITModule::compile(llvm::Module* llvmModule)
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

      llvm::orc::VModuleKey K = ES.allocateVModule();
      std::unique_ptr<llvm::Module> xxx(llvmModule);
		auto zzz = compileLayer->addModule(K, std::move(xxx));
		auto xxxx = compileLayer->emitAndFinalize(K);

      final_pic_code = std::move(*unitmemorymanager->code);
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
		const char wasmFuncPrefix[] = "wasmFunc";
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

	void init()
	{
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
		llvm::InitializeNativeTargetDisassembler();
		llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

		auto targetTriple = llvm::sys::getProcessTriple();

		targetMachine = llvm::EngineBuilder().setRelocationModel(llvm::Reloc::PIC_).setCodeModel(llvm::CodeModel::Small).selectTarget(
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
