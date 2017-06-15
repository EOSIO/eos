#include <eos/chain/wasm_interface.hpp>
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "Runtime/Runtime.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"
#include "IR/Module.h"
#include "IR/Operators.h"
#include "IR/Validate.h"
#include <eos/chain/key_value_object.hpp>

namespace eos { namespace chain {
   using namespace IR;
   using namespace Runtime;
  
   wasm_interface::wasm_interface() {
   }

DEFINE_INTRINSIC_FUNCTION4(env,store,store,none,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen ) {
   FC_ASSERT( keylen > 0 );
   FC_ASSERT( valuelen >= 0 );

   auto& wasm  = wasm_interface::get();

   FC_ASSERT( wasm.current_apply_context, "no apply context found" );

   auto& db    = wasm.current_apply_context->mutable_db;
   auto& scope = wasm.current_apply_context->scope;
   auto  mem   = wasm.current_memory;
   char* key   = &memoryRef<char>( mem, keyptr   );
   char* value = &memoryRef<char>( mem, valueptr );
   string keystr( key, key+keylen);


   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(scope, keystr) );
   if( obj ) {
      db.modify( *obj, [&]( auto& o ) {
         o.value.resize( valuelen );
         memcpy( o.value.data(), value, valuelen );
      });
   } else {
      db.create<key_value_object>( [&](auto& o) {
         o.scope = scope;
         o.key.insert( 0, key, keylen );
         o.value.insert( 0, value, valuelen );
      });
   }
} 

DEFINE_INTRINSIC_FUNCTION4(env,load,load,i32,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen ) {
   FC_ASSERT( keylen > 0 );
   FC_ASSERT( valuelen >= 0 );

   auto& wasm  = wasm_interface::get();

   FC_ASSERT( wasm.current_apply_context, "no apply context found" );

   auto& db    = wasm.current_apply_context->mutable_db;
   auto& scope = wasm.current_apply_context->scope;
   auto  mem   = wasm.current_memory;
   char* key   = &memoryRef<char>( mem, keyptr   );
   char* value = &memoryRef<char>( mem, valueptr );
   string keystr( key, key+keylen);

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(scope, keystr) );
   if( obj == nullptr ) return -1;
	 auto copylen =  std::min<size_t>(obj->value.size(),valuelen);
   if( copylen ) {
			memcpy( value, obj->value.data(), copylen );
   }
	 return copylen;
}

DEFINE_INTRINSIC_FUNCTION2(env,assert,assert,none,i32,test,i32,msg) {
  std::string message = &Runtime::memoryRef<char>( wasm_interface::get().current_memory, msg );
  if( !test ) edump((message));
  FC_ASSERT( test, "assertion failed: ${s}", ("s",message) );
} 

DEFINE_INTRINSIC_FUNCTION1(env,malloc,malloc,i32,i32,size) { 
   FC_ASSERT( size > 0 );
   int32_t& end = Runtime::memoryRef<int32_t>( Runtime::getDefaultMemory(wasm_interface::get().current_module), 0);
   int32_t old_end = end;
   end += 8*((size+7)/8);
   FC_ASSERT( end > old_end );
   return old_end;
}

DEFINE_INTRINSIC_FUNCTION1(env,printi,printi,none,i32,val) {
  idump((val));
}

DEFINE_INTRINSIC_FUNCTION2(env,print,print,none,i32,charptr,i32,size) {
  FC_ASSERT( size > 0 );
  char* str = &Runtime::memoryRef<char>( Runtime::getDefaultMemory(wasm_interface::get().current_module), charptr);
  char* end = &Runtime::memoryRef<char>( Runtime::getDefaultMemory(wasm_interface::get().current_module), charptr+size);
  edump((charptr)(size));
	wlog( std::string( str, end ) );
}

DEFINE_INTRINSIC_FUNCTION1(env,free,free,none,i32,ptr) { 
}

DEFINE_INTRINSIC_FUNCTION1(env,toUpper,toUpper,none,i32,charptr) { 
   std::cerr << "TO UPPER CALLED\n";// << charptr << "\n";
//   std::cerr << "moduleInstance: " << moduleInstance << "\n";
  // /*U8* base = */Runtime::getMemoryBaseAddress( Runtime::getDefaultMemory(moduleInstance) );
   //std::cerr << "Base Address: " << (int64_t)base;
   //char* c = (char*)(base + charptr);
   char& c = Runtime::memoryRef<char>( Runtime::getDefaultMemory(wasm_interface::get().current_module), charptr );
//   std::cerr << "char: " << c <<"\n";
//   if( c > 'Z' ) c -= 32;
//   return 0;
}

   wasm_interface& wasm_interface::get() {
      static wasm_interface*  wasm = nullptr;
      if( !wasm ) 
      {
         wlog( "Runtime::init" );
	       Runtime::init();
         wasm = new wasm_interface();
      }
      return *wasm;
   }



   struct RootResolver : Runtime::Resolver
   {
      std::map<std::string,Resolver*> moduleNameToResolverMap;

     bool resolve(const std::string& moduleName,const std::string& exportName,ObjectType type,ObjectInstance*& outObject) override
     {
       // Try to resolve an intrinsic first.
       if(IntrinsicResolver::singleton.resolve(moduleName,exportName,type,outObject)) { return true; }
       FC_ASSERT( !"unresolvable", "${module}.${export}", ("module",moduleName)("export",exportName) );

       // Then look for a named module.
       auto namedResolverIt = moduleNameToResolverMap.find(moduleName);
       if(namedResolverIt != moduleNameToResolverMap.end())
       {
         return namedResolverIt->second->resolve(moduleName,exportName,type,outObject);
       }

       // Finally, stub in missing function imports.
       if(type.kind == ObjectKind::function)
       {
         // Generate a function body that just uses the unreachable op to fault if called.
         Serialization::ArrayOutputStream codeStream;
         OperatorEncoderStream encoder(codeStream);
         encoder.unreachable();
         encoder.end();

         // Generate a module for the stub function.
         Module stubModule;
         DisassemblyNames stubModuleNames;
         stubModule.types.push_back(asFunctionType(type));
         stubModule.functions.defs.push_back({{0},{},std::move(codeStream.getBytes()),{}});
         stubModule.exports.push_back({"importStub",ObjectKind::function,0});
         stubModuleNames.functions.push_back({std::string(moduleName) + "." + exportName,{}});
         IR::setDisassemblyNames(stubModule,stubModuleNames);
         IR::validateDefinitions(stubModule);

         // Instantiate the module and return the stub function instance.
         auto stubModuleInstance = instantiateModule(stubModule,{});
         outObject = getInstanceExport(stubModuleInstance,"importStub");
         //Log::printf(Log::Category::error,"Generated stub for missing function import %s.%s : %s\n",moduleName.c_str(),exportName.c_str(),asString(type).c_str());
         return true;
       }
       else if(type.kind == ObjectKind::memory)
       {
         outObject = asObject(Runtime::createMemory(asMemoryType(type)));
         //Log::printf(Log::Category::error,"Generated stub for missing memory import %s.%s : %s\n",moduleName.c_str(),exportName.c_str(),asString(type).c_str());
         return true;
       }
       else if(type.kind == ObjectKind::table)
       {
         outObject = asObject(Runtime::createTable(asTableType(type)));
         //Log::printf(Log::Category::error,"Generated stub for missing table import %s.%s : %s\n",moduleName.c_str(),exportName.c_str(),asString(type).c_str());
         return true;
       }
       else if(type.kind == ObjectKind::global)
       {
         outObject = asObject(Runtime::createGlobal(asGlobalType(type),Runtime::Value(asGlobalType(type).valueType,Runtime::UntaggedValue())));
         //Log::printf(Log::Category::error,"Generated stub for missing global import %s.%s : %s\n",moduleName.c_str(),exportName.c_str(),asString(type).c_str());
         return true;
       }

       return false;
     }
   };

   void wasm_interface::validate( message_validate_context& c ) {
      current_validate_context       = &c;
      current_precondition_context   = nullptr;
      current_apply_context          = nullptr;

   }

   void wasm_interface::precondition( precondition_validate_context& c ) {
      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = nullptr;
   }


   char* wasm_interface::vm_allocate( int bytes ) {
			FunctionInstance* alloc_function = asFunctionNullable(getInstanceExport(current_module,"alloc"));
	    const FunctionType* functionType = getFunctionType(alloc_function);
      FC_ASSERT( functionType->parameters.size() == 1 );
	    std::vector<Value> invokeArgs(1);
      invokeArgs[0] = U32(bytes);

      auto result = Runtime::invokeFunction(alloc_function,invokeArgs);

      return &memoryRef<char>( current_memory, result.i32 );
   }

   U32 wasm_interface::vm_pointer_to_offset( char* ptr ) {
      return U32(ptr - &memoryRef<char>(current_memory,0));
   }

   void  wasm_interface::vm_apply( const vector<char>& message ) 
   { try {
      try {
				 FunctionInstance* apply = asFunctionNullable(getInstanceExport(current_module,"apply"));
				 const FunctionType* functionType = getFunctionType(apply);
				 FC_ASSERT( functionType->parameters.size() == 2 );

				 auto buffer = vm_allocate( message.size() );
				 memcpy( buffer, message.data(), message.size() );

				 std::vector<Value> args(2);
				 args[0] = vm_pointer_to_offset(buffer);
				 args[1] = U32(message.size());

				 Runtime::invokeFunction(apply,args);
      } catch( const Runtime::Exception& e ) {
          edump((std::string(describeExceptionCause(e.cause))));
					edump((e.callStack));
					throw;
      }
   } FC_CAPTURE_AND_RETHROW( (message) ) }

   void wasm_interface::apply( apply_context& c ) {
    try {
      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = &c;

      vm_apply( current_validate_context->msg.data );

      current_validate_context     = nullptr;
      current_precondition_context = nullptr;
      current_apply_context        = nullptr;
   } FC_CAPTURE_AND_RETHROW() }





   void wasm_interface::load(const char* bytes, size_t len)
   { try {
     static vector<char> memory_backup;
     if( module ) {
       char* memstart = &memoryRef<char>( current_memory, 0 );
       memcpy( memstart, memory_backup.data(), memory_backup.size() );
       return;	
			 auto start = fc::time_point::now();

       RootResolver rootResolver;
       LinkResult linkResult = linkModule(*module,rootResolver);
       current_module = instantiateModule( *module, std::move(linkResult.resolvedImports) );
       FC_ASSERT( current_module );
			 current_memory = Runtime::getDefaultMemory(current_module);
       

			 auto end = fc::time_point::now();
			 idump((  1000000.0 / (end-start).count() ) );
       return;
		  //  Runtime::freeUnreferencedObjects({});
        delete module;
     }
     wlog( "new module" );
     module = new IR::Module();

     // Load the module from a binary WebAssembly file.
     try
     {
			 auto start = fc::time_point::now();
       Serialization::MemoryInputStream stream((const U8*)bytes,len);
       WASM::serialize(stream,*module);

       RootResolver rootResolver;
       LinkResult linkResult = linkModule(*module,rootResolver);
       current_module = instantiateModule( *module, std::move(linkResult.resolvedImports) );
       FC_ASSERT( current_module );
			 current_memory = Runtime::getDefaultMemory(current_module);
			 auto end = fc::time_point::now();

       char* memstart = &memoryRef<char>( current_memory, 0 );
       memory_backup.resize(1<<16);
       memcpy( memstart, memory_backup.data(), memory_backup.size() );
			idump((  1000000.0 / (end-start).count() ) );
     }
     catch(Serialization::FatalSerializationException exception)
     {
       std::cerr << "Error deserializing WebAssembly binary file:" << std::endl;
       std::cerr << exception.message << std::endl;
       throw;
     }
     catch(IR::ValidationException exception)
     {
       std::cerr << "Error validating WebAssembly binary file:" << std::endl;
       std::cerr << exception.message << std::endl;
       throw;
     }
     catch(std::bad_alloc)
     {
       std::cerr << "Memory allocation failed: input is likely malformed" << std::endl;
       throw;
     }
   } FC_CAPTURE_AND_RETHROW() }

} }
