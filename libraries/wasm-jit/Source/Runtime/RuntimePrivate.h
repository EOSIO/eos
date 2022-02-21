#pragma once

#include "Inline/BasicTypes.h"
#include "Platform/Platform.h"
#include "Runtime.h"

#include <functional>
#include <map>
#include <atomic>

#define HAS_64BIT_ADDRESS_SPACE (sizeof(Uptr) == 8 && !PRETEND_32BIT_ADDRESS_SPACE)

namespace LLVMJIT
{
	using namespace Runtime;
	
	struct JITModuleBase
	{
		virtual ~JITModuleBase() {}
	};

	void init();
	void instantiateModule(const IR::Module& module,Runtime::ModuleInstance* moduleInstance);
	bool describeInstructionPointer(Uptr ip,std::string& outDescription);
	
	typedef void (*InvokeFunctionPointer)(void*,U64*);

	// Generates an invoke thunk for a specific function type.
	InvokeFunctionPointer getInvokeThunk(const IR::FunctionType* functionType);
}

namespace Runtime
{
	using namespace IR;
	
	// A private root for all runtime objects that handles garbage collection.
	struct GCObject : ObjectInstance
	{
		GCObject(ObjectKind inKind);
		~GCObject() override;
	};

	// An instance of a function: a function defined in an instantiated module, or an intrinsic function.
	struct FunctionInstance : GCObject
	{
		ModuleInstance* moduleInstance;
		const FunctionType* type;
		void* nativeFunction;
		std::string debugName;

		FunctionInstance(ModuleInstance* inModuleInstance,const FunctionType* inType,void* inNativeFunction = nullptr,const char* inDebugName = "<unidentified FunctionInstance>")
		: GCObject(ObjectKind::function), moduleInstance(inModuleInstance), type(inType), nativeFunction(inNativeFunction), debugName(inDebugName) {}
	};

	// An instance of a WebAssembly Table.
	struct TableInstance : GCObject
	{
		struct FunctionElement
		{
			const FunctionType* type;
			void* value;
		};

		TableType type;

		FunctionElement* baseAddress;
		Uptr endOffset;

		U8* reservedBaseAddress;
		Uptr reservedNumPlatformPages;

		// The Objects corresponding to the FunctionElements at baseAddress.
		std::vector<ObjectInstance*> elements;

		TableInstance(const TableType& inType): GCObject(ObjectKind::table), type(inType), baseAddress(nullptr), endOffset(0), reservedBaseAddress(nullptr), reservedNumPlatformPages(0) {}
		~TableInstance() override;
	};

	// An instance of a WebAssembly Memory.
	struct MemoryInstance : GCObject
	{
		MemoryType type;

		U8* baseAddress;
		std::atomic<Uptr> numPages;
		Uptr endOffset;

		U8* reservedBaseAddress;
		Uptr reservedNumPlatformPages;

		MemoryInstance(const MemoryType& inType): GCObject(ObjectKind::memory), type(inType), baseAddress(nullptr), numPages(0), endOffset(0), reservedBaseAddress(nullptr), reservedNumPlatformPages(0) {}
		~MemoryInstance() override;

      static MemoryInstance* theMemoryInstance;
	};

	// An instance of a WebAssembly global.
	struct GlobalInstance : GCObject
	{
		GlobalType type;
		UntaggedValue value;
		UntaggedValue initialValue;

		GlobalInstance(GlobalType inType,UntaggedValue inValue): GCObject(ObjectKind::global), type(inType), value(inValue), initialValue(value) {}
	};

	// An instance of a WebAssembly module.
	struct ModuleInstance : GCObject
	{
		std::map<std::string,ObjectInstance*> exportMap;

		std::vector<FunctionInstance*> functionDefs;

		std::vector<FunctionInstance*> functions;
		std::vector<TableInstance*> tables;
		std::vector<MemoryInstance*> memories;
		std::vector<GlobalInstance*> globals;

		MemoryInstance* defaultMemory;
		TableInstance* defaultTable;

		LLVMJIT::JITModuleBase* jitModule;

		Uptr startFunctionIndex = UINTPTR_MAX;

		ModuleInstance(
			std::vector<FunctionInstance*>&& inFunctionImports,
			std::vector<TableInstance*>&& inTableImports,
			std::vector<MemoryInstance*>&& inMemoryImports,
			std::vector<GlobalInstance*>&& inGlobalImports
			)
		: GCObject(ObjectKind::module)
		, functions(inFunctionImports)
		, tables(inTableImports)
		, memories(inMemoryImports)
		, globals(inGlobalImports)
		, defaultMemory(nullptr)
		, defaultTable(nullptr)
		, jitModule(nullptr)
		{}

		~ModuleInstance() override;
	};

	// Adds GC roots from WASM threads to the provided array.
	void getThreadGCRoots(std::vector<ObjectInstance*>& outGCRoots);
}
