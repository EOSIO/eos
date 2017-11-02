#include <eosio/blockchain/wasm_interface.hpp>

#include <eosio/blockchain/apply_context.hpp>

#include <boost/core/ignore_unused.hpp>

#include <Runtime/Runtime.h>
#include "IR/Module.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "Runtime/Runtime.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"
#include "IR/Module.h"
#include "IR/Operators.h"
#include "IR/Validate.h"


namespace eosio { namespace blockchain {

   using namespace IR;
   using namespace Runtime;

   struct module_state {
      Runtime::ModuleInstance* instance     = nullptr;
      IR::Module*              module       = nullptr;
      int                      mem_start    = 0;
      int                      mem_end      = 1<<16;
      vector<char>             init_memory;
      fc::sha256               code_version;
   };

   struct root_resolver : Runtime::Resolver
   {
      bool resolve(const string& modname,
                   const string& exportname,
                   ObjectType type,
                   ObjectInstance*& out) override
      {
          // Try to resolve an intrinsic first.
          if(IntrinsicResolver::singleton.resolve(modname,exportname,type,out)) { return true; }
          FC_ASSERT( !"unresolvable", "${module}.${export}", ("module",modname)("export",exportname) );
          return false;
      }
   };



   std::mutex                     global_load_mutex;
   struct wasm_interface_impl {
      map<digest_type, module_state> code_cache;
      module_state*                  current_state = nullptr;
   };

   wasm_interface::wasm_interface()
   :my( new wasm_interface_impl() ) {

   }

   wasm_interface& wasm_interface::get() {
      static bool init_once = [](){  Runtime::init(); return true; }();
      boost::ignore_unused(init_once);

      thread_local wasm_interface* single = nullptr;
      if( !single ) {
         single = new wasm_interface();
      }
      return *single;
   }


   void wasm_interface::load( digest_type codeid, const char* code, size_t codesize ) 
   {
      std::unique_lock<std::mutex> lock(global_load_mutex);

      FC_ASSERT( codeid != digest_type() );
      auto& state = my->code_cache[codeid];
      if( state.code_version == codeid ) {
         my->current_state = &state;
         return; /// already cached
      }

      state.module = new IR::Module();

     try {
       Serialization::MemoryInputStream stream((const U8*)code,codesize);
       WASM::serializeWithInjection(stream,*state.module);

       root_resolver resolver;
       LinkResult link_result = linkModule( *state.module, resolver );
       state.instance         = instantiateModule( *state.module, move(link_result.resolvedImports) );
       FC_ASSERT( state.instance );

       auto current_memory = Runtime::getDefaultMemory(state.instance);
         
       char* memstart = &memoryRef<char>( current_memory, 0 );
       const auto allocated_memory = Runtime::getDefaultMemorySize(state.instance);
       for( uint64_t i = 0; i < allocated_memory; ++i )
       {
          if( memstart[i] ) {
             state.mem_end = i+1;
          }
       }

       state.init_memory.resize(state.mem_end);
       memcpy( state.init_memory.data(), memstart, state.mem_end ); 
       state.code_version = codeid;
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
     my->current_state = &state;
   } /// wasm_interface::load

   void wasm_interface::init( apply_context& context ) 
   { try {
      try {
         FunctionInstance* init = asFunctionNullable(getInstanceExport(my->current_state->instance,"init"));
         if( !init ) return; /// if not found then it is a no-op

         //checktimeStart = fc::time_point::now();

         const FunctionType* functype = getFunctionType(init);
         FC_ASSERT( functype->parameters.size() == 0 );

         std::vector<Value> args(0);

         Runtime::invokeFunction(init,args);

      } catch( const Runtime::Exception& e ) {
          edump((string(describeExceptionCause(e.cause))));
          edump((e.callStack));
          throw;
      }
   } FC_CAPTURE_AND_RETHROW() } /// wasm_interface::init

   void wasm_interface::apply( apply_context& context ) 
   { try {
      try {
         FunctionInstance* call = asFunctionNullable(getInstanceExport(my->current_state->instance,"apply") );
         if( !call ) {
            return;
         }
         //FC_ASSERT( apply, "no entry point found for ${call}", ("call", std::string(name))  );

         FC_ASSERT( getFunctionType(call)->parameters.size() == 2 );

  //       idump((current_validate_context->msg.code)(current_validate_context->msg.type)(current_validate_context->code));
         vector<Value> args = { Value(uint64_t(context.current_action->act.scope)),
                                Value(uint64_t(context.current_action->act.name)) };

         auto& state = *my->current_state;
         char* memstart = &memoryRef<char>( getDefaultMemory(my->current_state->instance), 0 );
         memset( memstart + state.mem_end, 0, ((1<<16) - state.mem_end) );
         memcpy( memstart, state.init_memory.data(), state.mem_end);

         //checktimeStart = fc::time_point::now();

         Runtime::invokeFunction(call,args);
      } catch( const Runtime::Exception& e ) {
          edump((std::string(describeExceptionCause(e.cause))));
          edump((e.callStack));
          throw;
      }
   } FC_CAPTURE_AND_RETHROW() } /// wasm_interface::apply

   Runtime::MemoryInstance* wasm_interface::memory()const {
      return Runtime::getDefaultMemory( my->current_state->instance );
   }
   uint32_t wasm_interface::memory_size()const {
      return Runtime::getDefaultMemorySize( my->current_state->instance );
   }

   DEFINE_INTRINSIC_FUNCTION2(env,assert,assert,none,i32,test,i32,msg) {
      elog( "assert" );
      /*
      const char* m = &Runtime::memoryRef<char>( wasm_interface::get().current_memory, msg );
     std::string message( m );
     if( !test ) edump((message));
     FC_ASSERT( test, "assertion failed: ${s}", ("s",message)("ptr",msg) );
     */
   }

   DEFINE_INTRINSIC_FUNCTION1(env,printi,printi,none,i64,val) {
     std::cerr << uint64_t(val);
   }
   DEFINE_INTRINSIC_FUNCTION1(env,printd,printd,none,i64,val) {
     //std::cerr << DOUBLE(*reinterpret_cast<double *>(&val));
   }

   DEFINE_INTRINSIC_FUNCTION1(env,printi128,printi128,none,i32,val) {
     auto& wasm  = wasm_interface::get();
     auto  mem   = wasm.memory();
     auto& value = memoryRef<unsigned __int128>( mem, val );
     /*
     fc::uint128_t v(value>>64, uint64_t(value) );
     std::cerr << fc::variant(v).get_string();
     */

   }
   DEFINE_INTRINSIC_FUNCTION1(env,printn,printn,none,i64,val) {
     std::cerr << name(val).to_string();
   }

   DEFINE_INTRINSIC_FUNCTION1(env,prints,prints,none,i32,charptr) {
     auto& wasm  = wasm_interface::get();
     auto  mem   = wasm.memory();

     const char* str = &memoryRef<const char>( mem, charptr );
     const auto allocated_memory = wasm.memory_size(); //Runtime::getDefaultMemorySize(state.instance);

     std::cerr << std::string( str, strnlen(str, allocated_memory-charptr) );
   }

DEFINE_INTRINSIC_FUNCTION2(env,readMessage,readMessage,i32,i32,destptr,i32,destsize) {
   FC_ASSERT( destsize > 0 );

   /*
   wasm_interface& wasm = wasm_interface::get();
   auto  mem   = wasm.current_memory;
   char* begin = memoryArrayPtr<char>( mem, destptr, uint32_t(destsize) );

   int minlen = std::min<int>(wasm.current_validate_context->msg.data.size(), destsize);

//   wdump((destsize)(wasm.current_validate_context->msg.data.size()));
   memcpy( begin, wasm.current_validate_context->msg.data.data(), minlen );
   */
   return 0;//minlen;
}



#define READ_RECORD(READFUNC, INDEX, SCOPE) \
   return 0;
   /*
   auto lambda = [&](apply_context* ctx, INDEX::value_type::key_type* keys, char *data, uint32_t datalen) -> int32_t { \
      auto res = ctx->READFUNC<INDEX, SCOPE>( Name(scope), Name(code), Name(table), keys, data, datalen); \
      if (res >= 0) res += INDEX::value_type::number_of_keys*sizeof(INDEX::value_type::key_type); \
      return res; \
   }; \
   return validate<decltype(lambda), INDEX::value_type::key_type, INDEX::value_type::number_of_keys>(valueptr, valuelen, lambda);
   */

#define DEFINE_RECORD_READ_FUNCTIONS(OBJTYPE, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_INTRINSIC_FUNCTION5(env,load_##FUNCPREFIX##OBJTYPE,load_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(load_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,front_##FUNCPREFIX##OBJTYPE,front_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(front_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,back_##FUNCPREFIX##OBJTYPE,back_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(back_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,next_##FUNCPREFIX##OBJTYPE,next_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(next_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,previous_##FUNCPREFIX##OBJTYPE,previous_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(previous_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,lower_bound_##FUNCPREFIX##OBJTYPE,lower_bound_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(lower_bound_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,upper_bound_##FUNCPREFIX##OBJTYPE,upper_bound_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(upper_bound_record, INDEX, SCOPE); \
   }

#define UPDATE_RECORD(UPDATEFUNC, INDEX, DATASIZE) \
   return 0;

   /*
   auto lambda = [&](apply_context* ctx, INDEX::value_type::key_type* keys, char *data, uint32_t datalen) -> int32_t { \
      return ctx->UPDATEFUNC<INDEX::value_type>( Name(scope), Name(ctx->code.value), Name(table), keys, data, datalen); \
   }; \
   return validate<decltype(lambda), INDEX::value_type::key_type, INDEX::value_type::number_of_keys>(valueptr, DATASIZE, lambda);
   */

#define DEFINE_RECORD_UPDATE_FUNCTIONS(OBJTYPE, INDEX) \
   DEFINE_INTRINSIC_FUNCTION4(env,store_##OBJTYPE,store_##OBJTYPE,i32,i64,scope,i64,table,i32,valueptr,i32,valuelen) { \
      UPDATE_RECORD(store_record, INDEX, valuelen); \
   } \
   DEFINE_INTRINSIC_FUNCTION4(env,update_##OBJTYPE,update_##OBJTYPE,i32,i64,scope,i64,table,i32,valueptr,i32,valuelen) { \
      UPDATE_RECORD(update_record, INDEX, valuelen); \
   } \
   DEFINE_INTRINSIC_FUNCTION3(env,remove_##OBJTYPE,remove_##OBJTYPE,i32,i64,scope,i64,table,i32,valueptr) { \
      UPDATE_RECORD(remove_record, INDEX, sizeof(typename INDEX::value_type::key_type)*INDEX::value_type::number_of_keys); \
   }

DEFINE_RECORD_READ_FUNCTIONS(i64,,key_value_index, by_scope_primary);
DEFINE_RECORD_UPDATE_FUNCTIONS(i64, key_value_index);

DEFINE_INTRINSIC_FUNCTION1(env,requireAuth,requireAuth,none,i64,account) {
   //wasm_interface::get().current_validate_context->require_authorization( Name(account) );
}

DEFINE_INTRINSIC_FUNCTION1(env,requireNotice,requireNotice,none,i64,account) {
   //wasm_interface::get().current_validate_context->require_authorization( Name(account) );
}
DEFINE_INTRINSIC_FUNCTION0(env,checktime,checktime,none) {
   /*
   auto dur = wasm_interface::get().current_execution_time();
   if (dur > CHECKTIME_LIMIT) {
      wlog("checktime called ${d}", ("d", dur));
      throw checktime_exceeded();
   }
   */
}


} } /// eosio::blockchain
