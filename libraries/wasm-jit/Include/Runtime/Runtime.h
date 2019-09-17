#pragma once

#include "Inline/BasicTypes.h"
#include "TaggedValue.h"
#include "IR/Types.h"

#include <map>
#include <vector>

#ifndef RUNTIME_API
	#define RUNTIME_API DLL_IMPORT
#endif

// Declare IR::Module to avoid including the definition.
namespace IR { struct Module; }

/////////////////
struct instantiated_code {
   std::vector<uint8_t> code;
   std::map<unsigned, uintptr_t> function_offsets;
};
namespace LLVMJIT {
instantiated_code instantiateModule(const IR::Module& module);
}
/////////////////

namespace Runtime
{
	// Initializes the runtime. Should only be called once per process.
	RUNTIME_API void init();

	// Information about a runtime exception.
	struct Exception
	{
		enum class Cause : U8
		{
			unknown,
			accessViolation,
			stackOverflow,
			integerDivideByZeroOrIntegerOverflow,
			invalidFloatOperation,
			invokeSignatureMismatch,
			reachedUnreachable,
			indirectCallSignatureMismatch,
			undefinedTableElement,
			calledAbort,
			calledUnimplementedIntrinsic,
			outOfMemory,
			invalidSegmentOffset,
			misalignedAtomicMemoryAccess
		};

		Cause cause;
		std::vector<std::string> callStack;		
	};
	
	// Returns a string that describes the given exception cause.
	inline const char* describeExceptionCause(Exception::Cause cause)
	{
		switch(cause)
		{
		case Exception::Cause::accessViolation: return "access violation";
		case Exception::Cause::stackOverflow: return "stack overflow";
		case Exception::Cause::integerDivideByZeroOrIntegerOverflow: return "integer divide by zero or signed integer overflow";
		case Exception::Cause::invalidFloatOperation: return "invalid floating point operation";
		case Exception::Cause::invokeSignatureMismatch: return "invoke signature mismatch";
		case Exception::Cause::reachedUnreachable: return "reached unreachable code";
		case Exception::Cause::indirectCallSignatureMismatch: return "call_indirect to function with wrong signature";
		case Exception::Cause::undefinedTableElement: return "undefined function table element";
		case Exception::Cause::calledAbort: return "called abort";
		case Exception::Cause::calledUnimplementedIntrinsic: return "called unimplemented intrinsic";
		case Exception::Cause::outOfMemory: return "out of memory";
		case Exception::Cause::invalidSegmentOffset: return "invalid segment offset";
		case Exception::Cause::misalignedAtomicMemoryAccess: return "misaligned atomic memory access";
		default: return "unknown";
		}
	}

	// Causes a runtime exception.
	[[noreturn]] RUNTIME_API void causeException(Exception::Cause cause);

	// These are subclasses of Object, but are only defined within Runtime, so other modules must
	// use these forward declarations as opaque pointers.
	struct FunctionInstance;
	struct TableInstance;
	struct MemoryInstance;
	struct GlobalInstance;
	struct ModuleInstance;

	// A runtime object of any type.
	struct ObjectInstance
	{
		const IR::ObjectKind kind;

		ObjectInstance(IR::ObjectKind inKind): kind(inKind) {}
		virtual ~ObjectInstance() {}
	};
	
	// Tests whether an object is of the given type.
	RUNTIME_API bool isA(ObjectInstance* object,const IR::ObjectType& type);

	// Casts from object to subclasses, and vice versa.
	inline FunctionInstance* asFunction(ObjectInstance* object)		{ WAVM_ASSERT_THROW(object && object->kind == IR::ObjectKind::function); return (FunctionInstance*)object; }
	inline TableInstance* asTable(ObjectInstance* object)			{ WAVM_ASSERT_THROW(object && object->kind == IR::ObjectKind::table); return (TableInstance*)object; }
	inline MemoryInstance* asMemory(ObjectInstance* object)		{ WAVM_ASSERT_THROW(object && object->kind == IR::ObjectKind::memory); return (MemoryInstance*)object; }
	inline GlobalInstance* asGlobal(ObjectInstance* object)		{ WAVM_ASSERT_THROW(object && object->kind == IR::ObjectKind::global); return (GlobalInstance*)object; }
	inline ModuleInstance* asModule(ObjectInstance* object)		{ WAVM_ASSERT_THROW(object && object->kind == IR::ObjectKind::module); return (ModuleInstance*)object; }

	template<typename Instance> Instance* as(ObjectInstance* object);
	template<> inline FunctionInstance* as<FunctionInstance>(ObjectInstance* object) { return asFunction(object); }
	template<> inline TableInstance* as<TableInstance>(ObjectInstance* object) { return asTable(object); }
	template<> inline MemoryInstance* as<MemoryInstance>(ObjectInstance* object) { return asMemory(object); }
	template<> inline GlobalInstance* as<GlobalInstance>(ObjectInstance* object) { return asGlobal(object); }
	template<> inline ModuleInstance* as<ModuleInstance>(ObjectInstance* object) { return asModule(object); }

	inline ObjectInstance* asObject(FunctionInstance* function) { return (ObjectInstance*)function; }
	inline ObjectInstance* asObject(TableInstance* table) { return (ObjectInstance*)table; }
	inline ObjectInstance* asObject(MemoryInstance* memory) { return (ObjectInstance*)memory; }
	inline ObjectInstance* asObject(GlobalInstance* global) { return (ObjectInstance*)global; }
	inline ObjectInstance* asObject(ModuleInstance* module) { return (ObjectInstance*)module; }

	// Casts from object to subclass that checks that the object is the right kind and returns null if not.
	inline FunctionInstance* asFunctionNullable(ObjectInstance* object)	{ return object && object->kind == IR::ObjectKind::function ? (FunctionInstance*)object : nullptr; }
	inline TableInstance* asTableNullable(ObjectInstance* object)		{ return object && object->kind == IR::ObjectKind::table ? (TableInstance*)object : nullptr; }
	inline MemoryInstance* asMemoryNullable(ObjectInstance* object)	{ return object && object->kind == IR::ObjectKind::memory ? (MemoryInstance*)object : nullptr; }
	inline GlobalInstance* asGlobalNullable(ObjectInstance* object)		{ return object && object->kind == IR::ObjectKind::global ? (GlobalInstance*)object : nullptr; }
	inline ModuleInstance* asModuleNullable(ObjectInstance* object)	{ return object && object->kind == IR::ObjectKind::module ? (ModuleInstance*)object : nullptr; }
	
	// Frees unreferenced Objects, using the provided array of Objects as the root set.
	RUNTIME_API void freeUnreferencedObjects(std::vector<ObjectInstance*>&& rootObjectReferences);

	//
	// Functions
	//

	// Invokes a FunctionInstance with the given parameters, and returns the result.
	// Throws a Runtime::Exception if a trap occurs.
	RUNTIME_API Result invokeFunction(FunctionInstance* function,const std::vector<Value>& parameters);

	// Returns the type of a FunctionInstance.
	RUNTIME_API const IR::FunctionType* getFunctionType(FunctionInstance* function);

	//
	// Tables
	//

	// Creates a Table. May return null if the memory allocation fails.
	RUNTIME_API TableInstance* createTable(IR::TableType type);

	// Reads an element from the table. Assumes that index is in bounds.
	RUNTIME_API ObjectInstance* getTableElement(TableInstance* table,Uptr index);

	// Writes an element to the table. Assumes that index is in bounds, and returns a pointer to the previous value of the element.
	RUNTIME_API ObjectInstance* setTableElement(TableInstance* table,Uptr index,ObjectInstance* newValue);

	// Gets the current or maximum size of the table.
	RUNTIME_API Uptr getTableNumElements(TableInstance* table);
	RUNTIME_API Uptr getTableMaxElements(TableInstance* table);

	// Grows or shrinks the size of a table by numElements. Returns the previous size of the table.
	RUNTIME_API Iptr growTable(TableInstance* table,Uptr numElements);
	RUNTIME_API Iptr shrinkTable(TableInstance* table,Uptr numElements);

	//
	// Memories
	//

	// Creates a Memory. May return null if the memory allocation fails.
	RUNTIME_API MemoryInstance* createMemory(IR::MemoryType type);

	// Gets the base address of the memory's data.
	RUNTIME_API U8* getMemoryBaseAddress(MemoryInstance* memory);

	// Gets the current or maximum size of the memory in pages.
	RUNTIME_API Uptr getMemoryNumPages(MemoryInstance* memory);
	RUNTIME_API Uptr getMemoryMaxPages(MemoryInstance* memory);

	// Grows or shrinks the size of a memory by numPages. Returns the previous size of the memory.
	RUNTIME_API Iptr growMemory(MemoryInstance* memory,Uptr numPages);
	RUNTIME_API Iptr shrinkMemory(MemoryInstance* memory,Uptr numPages);

	// Validates that an offset range is wholly inside a Memory's virtual address range.
	RUNTIME_API U8* getValidatedMemoryOffsetRange(MemoryInstance* memory,Uptr offset,Uptr numBytes);
	
	// Validates an access to a single element of memory at the given offset, and returns a reference to it.
	template<typename Value> Value& memoryRef(MemoryInstance* memory,U32 offset)
	{ return *(Value*)getValidatedMemoryOffsetRange(memory,offset,sizeof(Value)); }

	// Validates an access to multiple elements of memory at the given offset, and returns a pointer to it.
	template<typename Value> Value* memoryArrayPtr(MemoryInstance* memory,U32 offset,U32 numElements)
	{ return (Value*)getValidatedMemoryOffsetRange(memory,offset,numElements * sizeof(Value)); }

	//
	// Globals
	//

	// Creates a GlobalInstance with the specified type and initial value.
	RUNTIME_API GlobalInstance* createGlobal(IR::GlobalType type,Value initialValue);

	// Reads the current value of a global.
	RUNTIME_API Value getGlobalValue(GlobalInstance* global);

	// Writes a new value to a global, and returns the previous value.
	RUNTIME_API Value setGlobalValue(GlobalInstance* global,Value newValue);

	//
	// Modules
	//

	struct ImportBindings
	{
		std::vector<FunctionInstance*> functions;
		std::vector<TableInstance*> tables;
		std::vector<MemoryInstance*> memories;
		std::vector<GlobalInstance*> globals;
	};

	// Gets the default table/memory for a ModuleInstance.
	RUNTIME_API MemoryInstance* getDefaultMemory(ModuleInstance* moduleInstance);
	RUNTIME_API uint64_t getDefaultMemorySize(ModuleInstance* moduleInstance);
	RUNTIME_API TableInstance* getDefaultTable(ModuleInstance* moduleInstance);

	RUNTIME_API void runInstanceStartFunc(ModuleInstance* moduleInstance);
	RUNTIME_API void resetGlobalInstances(ModuleInstance* moduleInstance);
	RUNTIME_API void resetMemory(MemoryInstance* memory, IR::MemoryType& newMemoryType);

	RUNTIME_API const std::vector<uint8_t>& getPICCode(ModuleInstance* moduleInstance);
	RUNTIME_API const std::map<unsigned, uintptr_t>& getFunctionOffsets(ModuleInstance* moduleInstance);

	// Gets an object exported by a ModuleInstance by name.
	RUNTIME_API ObjectInstance* getInstanceExport(ModuleInstance* moduleInstance,const std::string& name);
}
