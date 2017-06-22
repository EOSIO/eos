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
#include <eos/chain/account_object.hpp>

namespace eos { namespace chain {
   using namespace IR;
   using namespace Runtime;

   wasm_interface::wasm_interface() {
   }

DEFINE_INTRINSIC_FUNCTION4(env,store,store,none,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen ) {
//   ilog( "store ${keylen}  ${vallen}", ("keylen",keylen)("vallen",valuelen) );
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

//   if( valuelen == 8 ) idump(( *((int64_t*)value)));


   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(scope, keystr) );
   if( obj ) {
      db.modify( *obj, [&]( auto& o ) {
         o.value.assign(value, valuelen);
      });
   } else {
      db.create<key_value_object>( [&](auto& o) {
         o.scope = scope;
         o.key.insert( 0, key, keylen );
         o.value.insert( 0, value, valuelen );
      });
   }
}

DEFINE_INTRINSIC_FUNCTION2(env,remove,remove,i32,i32,keyptr,i32,keylen) {
   FC_ASSERT( keylen > 0 );

   auto& wasm  = wasm_interface::get();

   FC_ASSERT( wasm.current_apply_context, "no apply context found" );

   auto& db    = wasm.current_apply_context->mutable_db;
   auto& scope = wasm.current_apply_context->scope;
   auto  mem   = wasm.current_memory;
   char* key   = &memoryRef<char>( mem, keyptr   );
   string keystr( key, key+keylen);

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(scope, keystr) );
   if( obj ) {
			db.remove( *obj );
      return true;
   }
   return false;
}

DEFINE_INTRINSIC_FUNCTION3(env,memcpy,memcpy,i32,i32,dstp,i32,srcp,i32,len) {
   auto& wasm          = wasm_interface::get();
   auto  mem           = wasm.current_memory;
   char* dst           = &memoryRef<char>( mem, dstp);
   const char* src     = &memoryRef<const char>( mem, srcp );
   char* dst_end       = &memoryRef<char>( mem, dstp+uint32_t(len));
   const char* src_end = &memoryRef<const char>( mem, srcp+uint32_t(len) );

#warning TODO: wasm memcpy has undefined behavior if memory ranges overlap
/*
   if( dst > src )
			FC_ASSERT( dst < src_end && src < dst_end, "overlap of memory range is undefined", ("d",dstp)("s",srcp)("l",len) );
   else
			FC_ASSERT( src < dst_end && dst < src_end, "overlap of memory range is undefined", ("d",dstp)("s",srcp)("l",len) );
*/
   memcpy( dst, src, uint32_t(len) );
   return dstp;
}


DEFINE_INTRINSIC_FUNCTION2(env,Varint_unpack,Varint_unpack,none,i32,streamptr,i32,valueptr) {
   auto& wasm  = wasm_interface::get();
   auto  mem   = wasm.current_memory;

   uint32_t* stream = &memoryRef<uint32_t>( mem, streamptr );
   const char* pos = &memoryRef<const char>( mem, stream[1] );
   const char* end = &memoryRef<const char>( mem, stream[2] );
   uint32_t& value = memoryRef<uint32_t>( mem, valueptr );

   fc::unsigned_int vi;
   fc::datastream<const char*> ds(pos,end-pos);
   fc::raw::unpack( ds, vi );
   value = vi.value;

   stream[1] += ds.pos() - pos;
}

DEFINE_INTRINSIC_FUNCTION2(env,AccountName_unpack,AccountName_unpack,none,i32,streamptr,i32,accountptr) {
   auto& wasm  = wasm_interface::get();
   auto  mem   = wasm.current_memory;


   uint32_t* stream = &memoryRef<uint32_t>( mem, streamptr );
   const char* pos = &memoryRef<const char>( mem, stream[1] );
   const char* end = &memoryRef<const char>( mem, stream[2] );
   AccountName* name = &memoryRef<AccountName>( mem, accountptr );
   memset( name, 0, 32 );

   fc::datastream<const char*> ds( pos, end - pos );
   fc::raw::unpack( ds, *name );

   stream[1] += ds.pos() - pos;
}



DEFINE_INTRINSIC_FUNCTION4(env,load,load,i32,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen ) {
   //ilog( "load" );
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
 //  idump((keystr));

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(scope, keystr) );
   if( obj == nullptr ) return -1;
	 auto copylen =  std::min<size_t>(obj->value.size(),valuelen);
  // idump((copylen)(valuelen)(obj->value.size()));
   if( copylen ) {
			memcpy( value, obj->value.data(), copylen );
//      if( copylen == 8 ) idump(( *((int64_t*)value)));
   }
	 return copylen;
}

DEFINE_INTRINSIC_FUNCTION2(env,readMessage,readMessage,i32,i32,destptr,i32,destsize) {
   FC_ASSERT( destsize > 0 );
   wasm_interface& wasm = wasm_interface::get();
   auto  mem   = wasm.current_memory;
   char* begin = &Runtime::memoryRef<char>( mem, destptr );
   Runtime::memoryRef<char>( mem, destptr + destsize );

   int minlen = std::min<int>(wasm.current_validate_context->msg.data.size(), destsize);
   memcpy( begin, wasm.current_validate_context->msg.data.data(), minlen );
   return minlen;
}

DEFINE_INTRINSIC_FUNCTION2(env,assert,assert,none,i32,test,i32,msg) {
  std::string message = &Runtime::memoryRef<char>( wasm_interface::get().current_memory, msg );
  if( !test ) edump((message));
  FC_ASSERT( test, "assertion failed: ${s}", ("s",message)("ptr",msg) );
}

DEFINE_INTRINSIC_FUNCTION0(env,messageSize,messageSize,i32) {
   return wasm_interface::get().current_validate_context->msg.data.size();
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
DEFINE_INTRINSIC_FUNCTION1(env,printi64,printi64,none,i64,val) {
  idump((val));
}

DEFINE_INTRINSIC_FUNCTION2(env,print,print,none,i32,charptr,i32,size) {
  FC_ASSERT( size > 0 );
  const char* str = &Runtime::memoryRef<const char>( Runtime::getDefaultMemory(wasm_interface::get().current_module), charptr);
  const char* end = &Runtime::memoryRef<const char>( Runtime::getDefaultMemory(wasm_interface::get().current_module), charptr+size);
  edump((charptr)(size));
	wlog( std::string( str, size ) );
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

   void  wasm_interface::vm_apply()
   { try {
      try {
         std::string mangledapply("onApply_");
         mangledapply += std::string( current_validate_context->msg.type ) + "_";
         mangledapply += std::string( current_validate_context->msg.recipient );
//         idump((mangledapply));

				 FunctionInstance* apply = asFunctionNullable(getInstanceExport(current_module,mangledapply.c_str()));
		 		 if( !apply ) return; /// if not found then it is a no-op

				 const FunctionType* functionType = getFunctionType(apply);
				 FC_ASSERT( functionType->parameters.size() == 0 );

				 std::vector<Value> args(0);

				 Runtime::invokeFunction(apply,args);
      } catch( const Runtime::Exception& e ) {
          edump((std::string(describeExceptionCause(e.cause))));
					edump((e.callStack));
					throw;
      }
   } FC_CAPTURE_AND_RETHROW() }

   void  wasm_interface::vm_onInit()
   { try {
      try {
         // wlog( "onInit" );
				 FunctionInstance* apply = asFunctionNullable(getInstanceExport(current_module,"onInit"));
		 		 if( !apply ) {
           wlog( "no onInit method found" );
					 return; /// if not found then it is a no-op
         }

				 const FunctionType* functionType = getFunctionType(apply);
				 FC_ASSERT( functionType->parameters.size() == 0 );

				 std::vector<Value> args(0);

				 Runtime::invokeFunction(apply,args);
      } catch( const Runtime::Exception& e ) {
          edump((std::string(describeExceptionCause(e.cause))));
					edump((e.callStack));
					throw;
      }
   } FC_CAPTURE_AND_RETHROW() }

   void wasm_interface::apply( apply_context& c ) {
    try {
      load( c.scope, c.db );

      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = &c;

      vm_apply();

   } FC_CAPTURE_AND_RETHROW() }

   void wasm_interface::init( apply_context& c ) {
    try {
//      ilog( "WASM INTERFACE INIT" );
      load( c.scope, c.db );

      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = &c;

      vm_onInit();

   } FC_CAPTURE_AND_RETHROW() }


   void wasm_interface::load( const AccountName& name, const chainbase::database& db ) {
      const auto& recipient = db.get<account_object,by_name>( name );

      auto& state = instances[name];
      if( state.code_version != recipient.code_version ) {
         if( state.instance ) {
            /// TODO: free existing instance and module
#warning TODO: free existing module if the code has been updated, currently leak memory
            state.instance     = nullptr;
            state.module       = nullptr;
            state.code_version = -1;
         }
         state.module = new IR::Module();

        try
        {
          Serialization::MemoryInputStream stream((const U8*)recipient.code.data(),recipient.code.size());
          WASM::serialize(stream,*state.module);

          RootResolver rootResolver;
          LinkResult linkResult = linkModule(*state.module,rootResolver);
          state.instance = instantiateModule( *state.module, std::move(linkResult.resolvedImports) );
          FC_ASSERT( state.instance );
          current_memory = Runtime::getDefaultMemory(state.instance);

          char* memstart = &memoryRef<char>( current_memory, 0 );
          state.init_memory.resize(1<<16); /// TODO: actually get memory size
          memcpy( state.init_memory.data(), memstart, state.init_memory.size() );
          std::cerr <<"INIT MEMORY: \n";
          for( uint32_t i = 0; i < 10000; ++i )
              if( memstart[i] )
								 std::cerr << (char)memstart[i];
          std::cerr <<"\n";
          state.code_version = recipient.code_version;
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
      }

      current_module = state.instance;
      current_memory = getDefaultMemory( current_module );
      char* memstart = &memoryRef<char>( current_memory, 0 );
      memcpy( memstart, state.init_memory.data(), state.init_memory.size() );
   }



/*
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
   */

} }
