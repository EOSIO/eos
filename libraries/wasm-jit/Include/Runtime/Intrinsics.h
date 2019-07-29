#pragma once

#include "Inline/BasicTypes.h"
#include "IR/IR.h"
#include "Runtime.h"

#include <map>

namespace Intrinsics
{
	// An intrinsic function.
	struct Function
	{
		Runtime::FunctionInstance* function;

		RUNTIME_API Function(const char* inName,const IR::FunctionType* type,void* nativeFunction, size_t offset);
		RUNTIME_API ~Function();

      void* const getNativeFunc() const;
      size_t getOffset() const;

	private:
		const char* name;
	};
	
	// The base class of Intrinsic globals.
	struct Global
	{
		Runtime::GlobalInstance* global;

		RUNTIME_API Global(const char* inName,IR::GlobalType inType);
		RUNTIME_API ~Global();
		
		RUNTIME_API void reset();

	protected:
		void* value;
	private:
		const char* name;
		IR::GlobalType globalType;
	};
	
	// A partially specialized template for Intrinsic globals:
	// Provides access via implicit coercion to a value, and for mutable globals an assignment operator.
	template<IR::ValueType type,bool isMutable>
	struct GenericGlobal : Global
	{
		typedef typename IR::ValueTypeInfo<type>::Value Value;

		GenericGlobal(const char* inName,Value inValue)
		: Global(inName,IR::GlobalType(type,isMutable)) { *(Value*)value = inValue; }

		operator Value() const { return *(Value*)value; }
		void operator=(Value newValue) { *(Value*)value = newValue; }
	};
	template<IR::ValueType type>
	struct GenericGlobal<type,false> : Global
	{
		typedef typename IR::ValueTypeInfo<type>::Value Value;

		GenericGlobal(const char* inName,Value inValue)
		: Global(inName,IR::GlobalType(type,false)) { *(Value*)value = inValue; }

		operator Value() const { return *(Value*)value; }
		
		void reset(Value inValue)
		{
			Global::reset();
			*(Value*)value = inValue;
		}
	};

	// Intrinsic memories and tables
	struct Memory
	{
		RUNTIME_API Memory(const char* inName,const IR::MemoryType& inType);
		RUNTIME_API ~Memory();

		operator Runtime::MemoryInstance*() const { return memory; }

	private:
		const char* name;
		Runtime::MemoryInstance* const memory;
	};

	struct Table
	{
		RUNTIME_API Table(const char* inName,const IR::TableType& inType);
		RUNTIME_API ~Table();
		
		operator Runtime::TableInstance*() const { return table; }

	private:
		const char* name;
		Runtime::TableInstance* const table;
	};

	// Finds an intrinsic object by name and type.
	RUNTIME_API Runtime::ObjectInstance* find(const std::string& name,const IR::ObjectType& type);

	// Returns an array of all intrinsic runtime Objects; used as roots for garbage collection.
	RUNTIME_API std::vector<Runtime::ObjectInstance*> getAllIntrinsicObjects();

   ///XXX
   const std::map<std::string,Intrinsics::Function*>& getIntrinsicFunctions();
}

namespace NativeTypes
{
	typedef I32 i32;
	typedef I64 i64;
	typedef F32 f32;
	typedef F64 f64;
	typedef void none;
	#if ENABLE_SIMD_PROTOTYPE
	typedef IR::V128 v128;
	typedef IR::V128 b8x16;
	typedef IR::V128 b16x8;
	typedef IR::V128 b32x4;
	typedef IR::V128 b64x2;
	#endif
};

// Macros for defining intrinsic functions of various arities.
#define DEFINE_INTRINSIC_FUNCTION0(module,Offset,cName,name,returnType) \
	NativeTypes::returnType cName##returnType(); \
	static Intrinsics::Function cName##returnType##Function(#module "." #name,IR::FunctionType::get(IR::ResultType::returnType),(void*)&cName##returnType,Offset); \
	NativeTypes::returnType cName##returnType()

#define DEFINE_INTRINSIC_FUNCTION1(module,cName,name,returnType,arg0Type,arg0Name) \
	NativeTypes::returnType cName##returnType##arg0Type(NativeTypes::arg0Type); \
	static Intrinsics::Function cName##returnType##arg0Type##Function(#module "." #name,IR::FunctionType::get(IR::ResultType::returnType,{IR::ValueType::arg0Type}),(void*)&cName##returnType##arg0Type); \
	NativeTypes::returnType cName##returnType##arg0Type(NativeTypes::arg0Type arg0Name)

#define DEFINE_INTRINSIC_FUNCTION2(module,Offset,cName,name,returnType,arg0Type,arg0Name,arg1Type,arg1Name) \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type(NativeTypes::arg0Type,NativeTypes::arg1Type); \
	static Intrinsics::Function cName##returnType##arg0Type##arg1Type##Function(#module "." #name,IR::FunctionType::get(IR::ResultType::returnType,{IR::ValueType::arg0Type,IR::ValueType::arg1Type}),(void*)&cName##returnType##arg0Type##arg1Type,Offset); \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type(NativeTypes::arg0Type arg0Name,NativeTypes::arg1Type arg1Name)

#define DEFINE_INTRINSIC_FUNCTION3(module,cName,name,returnType,arg0Type,arg0Name,arg1Type,arg1Name,arg2Type,arg2Name) \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type(NativeTypes::arg0Type,NativeTypes::arg1Type,NativeTypes::arg2Type); \
	static Intrinsics::Function cName##returnType##arg0Type##arg1Type##arg2Type##Function(#module "." #name,IR::FunctionType::get(IR::ResultType::returnType,{IR::ValueType::arg0Type,IR::ValueType::arg1Type,IR::ValueType::arg2Type}),(void*)&cName##returnType##arg0Type##arg1Type##arg2Type); \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type(NativeTypes::arg0Type arg0Name,NativeTypes::arg1Type arg1Name,NativeTypes::arg2Type arg2Name)

#define DEFINE_INTRINSIC_FUNCTION4(module,cName,name,returnType,arg0Type,arg0Name,arg1Type,arg1Name,arg2Type,arg2Name,arg3Type,arg3Name) \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type(NativeTypes::arg0Type,NativeTypes::arg1Type,NativeTypes::arg2Type,NativeTypes::arg3Type); \
	static Intrinsics::Function cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##Function(#module "." #name,IR::FunctionType::get(IR::ResultType::returnType,{IR::ValueType::arg0Type,IR::ValueType::arg1Type,IR::ValueType::arg2Type,IR::ValueType::arg3Type}),(void*)&cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type); \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type(NativeTypes::arg0Type arg0Name,NativeTypes::arg1Type arg1Name,NativeTypes::arg2Type arg2Name,NativeTypes::arg3Type arg3Name)

#define DEFINE_INTRINSIC_FUNCTION5(module,cName,name,returnType,arg0Type,arg0Name,arg1Type,arg1Name,arg2Type,arg2Name,arg3Type,arg3Name,arg4Type,arg4Name) \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type(NativeTypes::arg0Type,NativeTypes::arg1Type,NativeTypes::arg2Type,NativeTypes::arg3Type,NativeTypes::arg4Type); \
	static Intrinsics::Function cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type##Function(#module "." #name,IR::FunctionType::get(IR::ResultType::returnType,{IR::ValueType::arg0Type,IR::ValueType::arg1Type,IR::ValueType::arg2Type,IR::ValueType::arg3Type,IR::ValueType::arg4Type}),(void*)&cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type); \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type(NativeTypes::arg0Type arg0Name,NativeTypes::arg1Type arg1Name,NativeTypes::arg2Type arg2Name,NativeTypes::arg3Type arg3Name,NativeTypes::arg4Type arg4Name)

#define DEFINE_INTRINSIC_FUNCTION6(module,cName,name,returnType,arg0Type,arg0Name,arg1Type,arg1Name,arg2Type,arg2Name,arg3Type,arg3Name,arg4Type,arg4Name,arg5Type,arg5Name) \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type##arg5Type(NativeTypes::arg0Type,NativeTypes::arg1Type,NativeTypes::arg2Type,NativeTypes::arg3Type,NativeTypes::arg4Type,NativeTypes::arg5Type); \
	static Intrinsics::Function cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type##arg5Type##Function(#module "." #name,IR::FunctionType::get(IR::ResultType::returnType,{IR::ValueType::arg0Type,IR::ValueType::arg1Type,IR::ValueType::arg2Type,IR::ValueType::arg3Type,IR::ValueType::arg4Type,IR::ValueType::arg5Type}),(void*)&cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type##arg5Type); \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type##arg5Type(NativeTypes::arg0Type arg0Name,NativeTypes::arg1Type arg1Name,NativeTypes::arg2Type arg2Name,NativeTypes::arg3Type arg3Name,NativeTypes::arg4Type arg4Name,NativeTypes::arg5Type arg5Name )

#define DEFINE_INTRINSIC_FUNCTION7(module,cName,name,returnType,arg0Type,arg0Name,arg1Type,arg1Name,arg2Type,arg2Name,arg3Type,arg3Name,arg4Type,arg4Name,arg5Type,arg5Name,arg6Type,arg6Name) \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type##arg5Type##arg6Type(NativeTypes::arg0Type,NativeTypes::arg1Type,NativeTypes::arg2Type,NativeTypes::arg3Type,NativeTypes::arg4Type,NativeTypes::arg5Type,NativeTypes::arg6Type); \
	static Intrinsics::Function cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type##arg5Type##arg6Type##Function(#module "." #name,IR::FunctionType::get(IR::ResultType::returnType,{IR::ValueType::arg0Type,IR::ValueType::arg1Type,IR::ValueType::arg2Type,IR::ValueType::arg3Type,IR::ValueType::arg4Type,IR::ValueType::arg5Type,IR::ValueType::arg6Type}),(void*)&cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type##arg5Type##arg6Type); \
	NativeTypes::returnType cName##returnType##arg0Type##arg1Type##arg2Type##arg3Type##arg4Type##arg5Type##arg6Type(NativeTypes::arg0Type arg0Name,NativeTypes::arg1Type arg1Name,NativeTypes::arg2Type arg2Name,NativeTypes::arg3Type arg3Name,NativeTypes::arg4Type arg4Name,NativeTypes::arg5Type arg5Name,NativeTypes::arg6Type arg6Name )

// Macros for defining intrinsic globals, memories, and tables.
#define DEFINE_INTRINSIC_GLOBAL(module,cName,name,valueType,isMutable,initializer) \
	static Intrinsics::GenericGlobal<IR::ValueType::valueType,isMutable> \
		cName(#module "." #name,initializer);

#define DEFINE_INTRINSIC_MEMORY(module,cName,name,type) static Intrinsics::Memory cName(#module "." #name,type);
#define DEFINE_INTRINSIC_TABLE(module,cName,name,type) static Intrinsics::Table cName(#module "." #name,type);
