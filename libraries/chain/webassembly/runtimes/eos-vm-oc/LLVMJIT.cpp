/*
The EOS VM Optimized Compiler was created in part based on WAVM
https://github.com/WebAssembly/wasm-jit-prototype
subject the following:

Copyright (c) 2016-2019, Andrew Scheidecker
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of WAVM nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "LLVMJIT.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/NullResolver.h"
#include "llvm/ExecutionEngine/Orc/Core.h"

#include "llvm/Analysis/Passes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/SymbolSize.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Utils.h"
#include <memory>

#include <fc/io/datastream.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/typename.hpp>

#include "llvm/Support/LEB128.h"

#if LLVM_VERSION_MAJOR == 7
namespace llvm { namespace orc {
	using LegacyRTDyldObjectLinkingLayer = RTDyldObjectLinkingLayer;
	template<typename A, typename B>
	using LegacyIRCompileLayer = IRCompileLayer<A, B>;
}}
#endif

#define DUMP_UNOPTIMIZED_MODULE 0
#define VERIFY_MODULE 0
#define DUMP_OPTIMIZED_MODULE 0
#define PRINT_DISASSEMBLY 0

#if PRINT_DISASSEMBLY
#include "llvm-c/Disassembler.h"
static void disassembleFunction(U8* bytes,Uptr numBytes)
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
#endif

namespace eosio { namespace chain { namespace eosvmoc {

namespace LLVMJIT
{
	llvm::TargetMachine* targetMachine = nullptr;

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
			code = std::make_unique<std::vector<uint8_t>>(numCodeBytes + numReadOnlyBytes + numReadWriteBytes);
			ptr = code->data();
		}
		virtual U8* allocateCodeSection(uintptr_t numBytes,U32 alignment,U32 sectionID,llvm::StringRef sectionName) override
		{
			return get_next_code_ptr(numBytes, alignment);
		}
		virtual U8* allocateDataSection(uintptr_t numBytes,U32 alignment,U32 sectionID,llvm::StringRef SectionName,bool isReadOnly) override
		{
			if(SectionName == ".eh_frame") {
				dumpster.resize(numBytes);
				return dumpster.data();
			}
			if(SectionName == ".stack_sizes") {
				return stack_sizes.emplace_back(numBytes).data();
			}
			WAVM_ASSERT_THROW(isReadOnly);
			if(SectionName == ".eosio_table") {
				U8* ptr = get_next_code_ptr(numBytes, alignment);
				table_offset = ptr-code->data();
				return ptr;
			}

			return get_next_code_ptr(numBytes, alignment);
		}

		virtual bool finalizeMemory(std::string* ErrMsg = nullptr) override {
			code->resize(ptr - code->data());
			return true;
		}

		std::unique_ptr<std::vector<uint8_t>> code;
		uint8_t* ptr;

		std::vector<uint8_t> dumpster;
		std::list<std::vector<uint8_t>> stack_sizes;
		uintptr_t table_offset;

		U8* get_next_code_ptr(uintptr_t numBytes, U32 alignment) {
			FC_ASSERT(alignment <= alignof(std::max_align_t), "alignment of section exceeds max_align_t");
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
	struct JITModule
	{
		JITModule() {
			objectLayer = std::make_unique<llvm::orc::LegacyRTDyldObjectLinkingLayer>(ES,[this](llvm::orc::VModuleKey K) {
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
											Uptr functionDefIndex;
											if(getFunctionIndexFromExternalName(name->data(),functionDefIndex))
												function_to_offsets[functionDefIndex] = loadedAddress-(uintptr_t)unitmemorymanager->code->data();
#if PRINT_DISASSEMBLY
											disassembleFunction((U8*)loadedAddress, symbolSizePair.second);
#endif
										}
									}
							  }
							  );
			objectLayer->setProcessAllSections(true);
			compileLayer = std::make_unique<CompileLayer>(*objectLayer,llvm::orc::SimpleCompiler(*targetMachine));
		}

		void compile(llvm::Module* llvmModule);

		std::shared_ptr<UnitMemoryManager> unitmemorymanager = std::make_shared<UnitMemoryManager>();

		std::map<unsigned, uintptr_t> function_to_offsets;
		std::vector<uint8_t> final_pic_code;

		~JITModule()
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
		///Log::printf(Log::Category::debug,"Dumped LLVM module to: %s\n",augmentedFilename.c_str());
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
			///Log::printf(Log::Category::debug,"Verified LLVM module\n");
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
		std::unique_ptr<llvm::Module> mod(llvmModule);
		WAVM_ASSERT_THROW(!compileLayer->addModule(K, std::move(mod)));
		WAVM_ASSERT_THROW(!compileLayer->emitAndFinalize(K));

		final_pic_code = std::move(*unitmemorymanager->code);
	}

	instantiated_code instantiateModule(const IR::Module& module)
	{
		static bool inited;
		if(!inited) {
			inited = true;
			llvm::InitializeNativeTarget();
			llvm::InitializeNativeTargetAsmPrinter();
			llvm::InitializeNativeTargetAsmParser();
			llvm::InitializeNativeTargetDisassembler();
			llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

			auto targetTriple = llvm::sys::getProcessTriple();

			llvm::TargetOptions to;
			to.EmitStackSizeSection = 1;

			targetMachine = llvm::EngineBuilder().setRelocationModel(llvm::Reloc::PIC_).setCodeModel(llvm::CodeModel::Small).setTargetOptions(to).selectTarget(
				llvm::Triple(targetTriple),"","",llvm::SmallVector<std::string,0>()
				);
		}

		// Emit LLVM IR for the module.
		auto llvmModule = emitModule(module);

		// Construct the JIT compilation pipeline for this module.
		auto jitModule = new JITModule();
		// Compile the module.
		jitModule->compile(llvmModule);

		unsigned num_functions_stack_size_found = 0;
		for(const auto& stacksizes : jitModule->unitmemorymanager->stack_sizes) {
			fc::datastream<const char*> ds(reinterpret_cast<const char*>(stacksizes.data()), stacksizes.size());
			while(ds.remaining()) {
				uint64_t funcaddr;
				fc::unsigned_int stack_size;
				fc::raw::unpack(ds, funcaddr);
				fc::raw::unpack(ds, stack_size);

				++num_functions_stack_size_found;
				if(stack_size > 16u*1024u)
					_exit(1);
			}
		}
		if(num_functions_stack_size_found != module.functions.defs.size())
			_exit(1);
		if(jitModule->final_pic_code.size() >= 16u*1024u*1024u)
			_exit(1);

		instantiated_code ret;
		ret.code = jitModule->final_pic_code;
		ret.function_offsets = jitModule->function_to_offsets;
		ret.table_offset = jitModule->unitmemorymanager->table_offset;
		return ret;
	}
}
}}}
