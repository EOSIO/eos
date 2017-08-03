#include <boost/function.hpp>
#include <eos/chain/wasm_interface.hpp>
#include <eos/chain/chain_controller.hpp>
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
#include <chrono>

namespace eos { namespace chain {
   using namespace IR;
   using namespace Runtime;

   wasm_interface::wasm_interface() {
   }

#ifdef NDEBUG
   const int CHECKTIME_LIMIT = 2000;
#else
   const int CHECKTIME_LIMIT = 12000;
#endif

DEFINE_INTRINSIC_FUNCTION0(env,checktime,checktime,none) {
   auto dur = wasm_interface::get().current_execution_time();
   if (dur > CHECKTIME_LIMIT) {
      wlog("checktime called ${d}", ("d", dur));
      throw checktime_exceeded();
   }
}

   int32_t load_i128i128_object( uint64_t scope, uint64_t code, uint64_t table, int32_t valueptr, int32_t valuelen, load_i128i128_fnc function ) {
      
      static const uint32_t keylen = 2*sizeof(uint128_t);
      
      FC_ASSERT( valuelen >= keylen, "insufficient data passed" );

      auto& wasm  = wasm_interface::get();
      FC_ASSERT( wasm.current_validate_context, "no validate context found" );

      char* value = memoryArrayPtr<char>( wasm.current_memory, valueptr, valuelen );
      uint128_t*  primary = reinterpret_cast<uint128_t*>(value);
      uint128_t*  secondary = primary + 1;
      
      valuelen -= keylen;
      value    += keylen;

      auto res = (wasm.current_validate_context->*function)( 
         Name(scope), Name(code), Name(table),
         primary, secondary, value, valuelen
      );

      if(res > 0) res += keylen;
      return res;
   }
DEFINE_INTRINSIC_FUNCTION3(env, assert_sha256,assert_sha256,none,i32,dataptr,i32,datalen,i32,hash) {
   FC_ASSERT( datalen > 0 );

   auto& wasm  = wasm_interface::get();
   auto  mem          = wasm.current_memory;

   char* data = memoryArrayPtr<char>( mem, dataptr, datalen );
   const auto& v = memoryRef<fc::sha256>( mem, hash );

   auto result = fc::sha256::hash( data, datalen );
   FC_ASSERT( result == v, "hash miss match" );
}

DEFINE_INTRINSIC_FUNCTION3(env,sha256,sha256,none,i32,dataptr,i32,datalen,i32,hash) {
   FC_ASSERT( datalen > 0 );

   auto& wasm  = wasm_interface::get();
   auto  mem          = wasm.current_memory;

   char* data = memoryArrayPtr<char>( mem, dataptr, datalen );
   auto& v = memoryRef<fc::sha256>( mem, hash );
   v  = fc::sha256::hash( data, datalen );
}

DEFINE_INTRINSIC_FUNCTION2(env,multeq_i128,multeq_i128,none,i32,self,i32,other) {
   auto& wasm  = wasm_interface::get();
   auto  mem   = wasm.current_memory;
   auto& v = memoryRef<unsigned __int128>( mem, self );
   const auto& o = memoryRef<const unsigned __int128>( mem, other );
   v *= o;
}


DEFINE_INTRINSIC_FUNCTION2(env,diveq_i128,diveq_i128,none,i32,self,i32,other) {
   auto& wasm  = wasm_interface::get();
   auto  mem          = wasm.current_memory;
   auto& v = memoryRef<unsigned __int128>( mem, self );
   const auto& o = memoryRef<const unsigned __int128>( mem, other );
   FC_ASSERT( o != 0, "divide by zero" );
   v /= o;
}

DEFINE_INTRINSIC_FUNCTION0(env,now,now,i32) {
   return wasm_interface::get().current_validate_context->controller.head_block_time().sec_since_epoch();
}

DEFINE_INTRINSIC_FUNCTION4(env,store_i128i128,store_i128i128,i32,i64,scope,i64,table,i32,valueptr,i32,valuelen) {
   
   static const uint32_t keylen = 2*sizeof(uint128_t);
   
   FC_ASSERT( valuelen >= keylen, "insufficient data passed" );

   auto& wasm  = wasm_interface::get();
   FC_ASSERT( wasm.current_apply_context, "no apply context found" );

   char* value = memoryArrayPtr<char>( wasm.current_memory, valueptr, valuelen );
   uint128_t*  primary = reinterpret_cast<uint128_t*>(value);
   uint128_t*  secondary = primary + 1;
   
   valuelen -= keylen;
   value    += keylen;

   //wdump((datalen)(valuelen)(result));
   return wasm.current_apply_context->store_i128i128( Name(scope), Name(table),
                                                      *primary, *secondary, value, valuelen );
}

struct i128_keys {
   uint128_t primary;
   uint128_t secondary;
};

DEFINE_INTRINSIC_FUNCTION3(env,remove_i128i128,remove_i128i128,i32,i64,scope,i64,table,i32,data) {
   auto& wasm  = wasm_interface::get();
   FC_ASSERT( wasm.current_apply_context, "not a valid apply context" );

   const i128_keys& keys = memoryRef<const i128_keys>( wasm.current_memory, data );
   return wasm_interface::get().current_apply_context->remove_i128i128( Name(scope), Name(table), keys.primary, keys.secondary );
}

DEFINE_INTRINSIC_FUNCTION5(env,load_primary_i128i128,load_primary_i128i128,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) {
   return load_i128i128_object(scope, code, table, valueptr, valuelen, &apply_context::load_primary_i128i128);
}

DEFINE_INTRINSIC_FUNCTION5(env,load_secondary_i128i128,load_secondary_i128i128,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) {
   return load_i128i128_object(scope, code, table, valueptr, valuelen, &apply_context::load_secondary_i128i128);
}

DEFINE_INTRINSIC_FUNCTION5(env,back_primary_i128i128,back_primary_i128i128,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) {
   return load_i128i128_object(scope, code, table, valueptr, valuelen, &apply_context::back_primary_i128i128);
}

DEFINE_INTRINSIC_FUNCTION5(env,front_primary_i128i128,front_primary_i128i128,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) {
   return load_i128i128_object(scope, code, table, valueptr, valuelen, &apply_context::front_primary_i128i128);
}

DEFINE_INTRINSIC_FUNCTION5(env,back_secondary_i128i128,back_secondary_i128i128,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) {
   return load_i128i128_object(scope, code, table, valueptr, valuelen, &apply_context::back_secondary_i128i128);
}

DEFINE_INTRINSIC_FUNCTION5(env,front_secondary_i128i128,front_secondary_i128i128,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) {
   return load_i128i128_object(scope, code, table, valueptr, valuelen, &apply_context::front_secondary_i128i128);
}

DEFINE_INTRINSIC_FUNCTION0(env,currentCode,currentCode,i64) {
   auto& wasm  = wasm_interface::get();
   return wasm.current_validate_context->code.value;
}

DEFINE_INTRINSIC_FUNCTION1(env,requireAuth,requireAuth,none,i64,account) {
   wasm_interface::get().current_validate_context->require_authorization( Name(account) );
}

DEFINE_INTRINSIC_FUNCTION1(env,requireNotice,requireNotice,none,i64,account) {
   wasm_interface::get().current_apply_context->require_recipient( account );
}

DEFINE_INTRINSIC_FUNCTION1(env,hasRecipient,hasRecipient,i32,i64,account) {
   return wasm_interface::get().current_apply_context->has_recipient( account );
}
DEFINE_INTRINSIC_FUNCTION1(env,requireScope,requireScope,none,i64,scope) {
   wasm_interface::get().current_validate_context->require_scope( scope );
}

DEFINE_INTRINSIC_FUNCTION4(env,store_i64,store_i64,i32,i64,scope,i64,table,i32,valueptr,i32,valuelen)
{
   static const uint32_t keylen = sizeof(uint64_t);

   FC_ASSERT( valuelen >= sizeof(uint64_t) );

   auto& wasm  = wasm_interface::get();
   FC_ASSERT( wasm.current_apply_context, "no apply context found" );

   auto  mem   = wasm.current_memory;
   char* value = memoryArrayPtr<char>( mem, valueptr, valuelen);
   uint64_t* key = reinterpret_cast<uint64_t*>(value);

   valuelen -= keylen;
   value    += keylen;

   //idump((Name(scope))(Name(code))(Name(table))(Name(key))(valuelen) );

   return wasm.current_apply_context->store_i64( scope, table, *key, value, valuelen );
}

DEFINE_INTRINSIC_FUNCTION5(env,load_i64,load_i64,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) 
{
   //idump((Name(scope))(Name(code))(Name(table))(Name(key))(valuelen) );
   static const uint32_t keylen = sizeof(uint64_t);
   
   FC_ASSERT( valuelen >= keylen );

   auto& wasm  = wasm_interface::get();
   FC_ASSERT( wasm.current_validate_context, "no validate context found" );

   auto  mem   = wasm.current_memory;
   char* value = memoryArrayPtr<char>( mem, valueptr, valuelen);
   uint64_t* key = reinterpret_cast<uint64_t*>(value);

   valuelen -= keylen;
   value    += keylen;

   auto res = wasm.current_validate_context->load_i64( scope, code, table, *key, value, valuelen );
   if(res > 0) res += keylen;
   return res;
}

DEFINE_INTRINSIC_FUNCTION3(env,remove_i64,remove_i64,i32,i64,scope,i64,table,i64,key) {
   auto& wasm  = wasm_interface::get();
   FC_ASSERT( wasm.current_apply_context, "no apply context found" );
   return wasm.current_apply_context->remove_i64( scope, table, key );
}

DEFINE_INTRINSIC_FUNCTION3(env,memcpy,memcpy,i32,i32,dstp,i32,srcp,i32,len) {
   auto& wasm          = wasm_interface::get();
   auto  mem           = wasm.current_memory;
   char* dst           = memoryArrayPtr<char>( mem, dstp, len);
   const char* src     = memoryArrayPtr<const char>( mem, srcp, len );
   FC_ASSERT( len > 0 );

   if( dst > src )
      FC_ASSERT( dst >= (src + len), "overlap of memory range is undefined", ("d",dstp)("s",srcp)("l",len) );
   else
      FC_ASSERT( src >= (dst + len), "overlap of memory range is undefined", ("d",dstp)("s",srcp)("l",len) );

   memcpy( dst, src, uint32_t(len) );
   return dstp;
}


DEFINE_INTRINSIC_FUNCTION2(env,send,send,i32,i32,trx_buffer, i32,trx_buffer_size ) {
   auto& wasm  = wasm_interface::get();
   auto  mem   = wasm.current_memory;
   const char* buffer = &memoryRef<const char>( mem, trx_buffer );

   FC_ASSERT( trx_buffer_size > 0 );
   FC_ASSERT( wasm.current_apply_context, "not in apply context" );

   fc::datastream<const char*> ds(buffer, trx_buffer_size );
   eos::chain::GeneratedTransaction gtrx;
   eos::chain::Transaction& trx = gtrx;
   fc::raw::unpack( ds, trx );

/**
 *  The code below this section provides sanity checks that the generated message is well formed
 *  before being accepted. These checks do not need to be applied during reindex.
 */
#warning TODO: reserve per-thread static memory for MAX TRX SIZE 
/** make sure that packing what we just unpacked produces expected output */
   auto test = fc::raw::pack( trx );
   FC_ASSERT( 0 == memcmp( buffer, test.data(), test.size() ) );

/** TODO: make sure that we can call validate() on the message and it passes, this is thread safe and
 *   ensures the type is properly registered and can be deserialized... one issue is that this could
 *   construct a RECURSIVE virtual machine state which means the wasm_interface state needs to be a STACK vs
 *   a per-thread global.
 **/

//   wasm.current_apply_context->generated.emplace_back( std::move(gtrx) );

   return 0;
}




DEFINE_INTRINSIC_FUNCTION2(env,readMessage,readMessage,i32,i32,destptr,i32,destsize) {
   FC_ASSERT( destsize > 0 );

   wasm_interface& wasm = wasm_interface::get();
   auto  mem   = wasm.current_memory;
   char* begin = memoryArrayPtr<char>( mem, destptr, uint32_t(destsize) );

   int minlen = std::min<int>(wasm.current_validate_context->msg.data.size(), destsize);

//   wdump((destsize)(wasm.current_validate_context->msg.data.size()));
   memcpy( begin, wasm.current_validate_context->msg.data.data(), minlen );
   return minlen;
}

DEFINE_INTRINSIC_FUNCTION2(env,assert,assert,none,i32,test,i32,msg) {
   const char* m = &Runtime::memoryRef<char>( wasm_interface::get().current_memory, msg );
  std::string message( m );
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

DEFINE_INTRINSIC_FUNCTION1(env,printi,printi,none,i64,val) {
  std::cerr << uint64_t(val);
}

DEFINE_INTRINSIC_FUNCTION1(env,printi128,printi128,none,i32,val) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;
  auto& value = memoryRef<unsigned __int128>( mem, val );
  fc::uint128_t v(value>>64, uint64_t(value) );
  std::cerr << fc::variant(v).get_string();
}
DEFINE_INTRINSIC_FUNCTION1(env,printn,printn,none,i64,val) {
  std::cerr << Name(val).toString();
}

DEFINE_INTRINSIC_FUNCTION1(env,prints,prints,none,i32,charptr) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  const char* str = &memoryRef<const char>( mem, charptr );

  std::cerr << std::string( str, strnlen(str, wasm.current_state->mem_end-charptr) );
}

DEFINE_INTRINSIC_FUNCTION1(env,free,free,none,i32,ptr) {
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

   int64_t wasm_interface::current_execution_time()
   {
      return (fc::time_point::now() - checktimeStart).count();
   }


   char* wasm_interface::vm_allocate( int bytes ) {
      FunctionInstance* alloc_function = asFunctionNullable(getInstanceExport(current_module,"alloc"));
      const FunctionType* functionType = getFunctionType(alloc_function);
      FC_ASSERT( functionType->parameters.size() == 1 );
      std::vector<Value> invokeArgs(1);
      invokeArgs[0] = U32(bytes);

      checktimeStart = fc::time_point::now();

      auto result = Runtime::invokeFunction(alloc_function,invokeArgs);

      return &memoryRef<char>( current_memory, result.i32 );
   }

   U32 wasm_interface::vm_pointer_to_offset( char* ptr ) {
      return U32(ptr - &memoryRef<char>(current_memory,0));
   }

   void  wasm_interface::vm_call( const char* name ) {
   try {
      try {
         /*
         name += "_" + std::string( current_validate_context->msg.code ) + "_";
         name += std::string( current_validate_context->msg.type );
         */
         /// TODO: cache this somehow
         FunctionInstance* call = asFunctionNullable(getInstanceExport(current_module,name) );
         if( !call ) {
            //wlog( "unable to find call ${name}", ("name",name));
            return;
         }
         //FC_ASSERT( apply, "no entry point found for ${call}", ("call", std::string(name))  );

         FC_ASSERT( getFunctionType(call)->parameters.size() == 2 );

  //       idump((current_validate_context->msg.code)(current_validate_context->msg.type)(current_validate_context->code));
         std::vector<Value> args = { Value(uint64_t(current_validate_context->msg.code)),
                                     Value(uint64_t(current_validate_context->msg.type)) };

         auto& state = *current_state;
         char* memstart = &memoryRef<char>( current_memory, 0 );
         memset( memstart + state.mem_end, 0, ((1<<16) - state.mem_end) );
         memcpy( memstart, state.init_memory.data(), state.mem_end);

         checktimeStart = fc::time_point::now();

         Runtime::invokeFunction(call,args);
      } catch( const Runtime::Exception& e ) {
          edump((std::string(describeExceptionCause(e.cause))));
          edump((e.callStack));
          throw;
      }
   } FC_CAPTURE_AND_RETHROW( (name)(current_validate_context->msg.type) ) }

   void  wasm_interface::vm_apply()        { vm_call("apply" );          }

   void  wasm_interface::vm_onInit()
   { try {
      try {
          wlog( "on_init" );
            FunctionInstance* apply = asFunctionNullable(getInstanceExport(current_module,"init"));
            if( !apply ) {
               elog( "no onInit method found" );
               return; /// if not found then it is a no-op
         }

         checktimeStart = fc::time_point::now();

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

   void wasm_interface::validate( apply_context& c ) {
      /*
      current_validate_context       = &c;
      current_precondition_context   = nullptr;
      current_apply_context          = nullptr;

      load( c.code, c.db );
      vm_validate();
      */
   }
   void wasm_interface::precondition( apply_context& c ) {
   try {

      /*
      current_validate_context       = &c;
      current_precondition_context   = &c;

      load( c.code, c.db );
      vm_precondition();
      */

   } FC_CAPTURE_AND_RETHROW() }


   void wasm_interface::apply( apply_context& c ) {
    try {
      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = &c;

      load( c.code, c.db );
      vm_apply();

   } FC_CAPTURE_AND_RETHROW() }

   void wasm_interface::init( apply_context& c ) {
    try {
      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = &c;

      load( c.code, c.db );
      vm_onInit();

   } FC_CAPTURE_AND_RETHROW() }



   void wasm_interface::load( const AccountName& name, const chainbase::database& db ) {
      const auto& recipient = db.get<account_object,by_name>( name );
  //    idump(("recipient")(Name(name))(recipient.code_version));

      auto& state = instances[name];
      if( state.code_version != recipient.code_version ) {
         if( state.instance ) {
            /// TODO: free existing instance and module
#warning TODO: free existing module if the code has been updated, currently leak memory
            state.instance     = nullptr;
            state.module       = nullptr;
            state.code_version = fc::sha256();
         }
         state.module = new IR::Module();

        try
        {
          wlog( "LOADING CODE" );
          auto start = fc::time_point::now();
          Serialization::MemoryInputStream stream((const U8*)recipient.code.data(),recipient.code.size());
          WASM::serializeWithInjection(stream,*state.module);

          RootResolver rootResolver;
          LinkResult linkResult = linkModule(*state.module,rootResolver);
          state.instance = instantiateModule( *state.module, std::move(linkResult.resolvedImports) );
          FC_ASSERT( state.instance );
          auto end = fc::time_point::now();
          idump(( (end-start).count()/1000000.0) );

          current_memory = Runtime::getDefaultMemory(state.instance);

          char* memstart = &memoryRef<char>( current_memory, 0 );
         // state.init_memory.resize(1<<16); /// TODO: actually get memory size
          for( uint32_t i = 0; i < 10000; ++i )
              if( memstart[i] ) {
                   state.mem_end = i+1;
                   //std::cerr << (char)memstart[i];
              }
          ilog( "INIT MEMORY: ${size}", ("size", state.mem_end) );

          state.init_memory.resize(state.mem_end);
          memcpy( state.init_memory.data(), memstart, state.mem_end ); //state.init_memory.size() );
          std::cerr <<"\n";
          state.code_version = recipient.code_version;
          idump((state.code_version));
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
      current_state  = &state;
   }

} }
