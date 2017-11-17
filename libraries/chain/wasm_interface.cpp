#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <eosio/chain/wasm_interface_private.hpp>
#include <fc/exception/exception.hpp>

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
#include "IR/Types.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <mutex>
#include <thread>
#include <condition_variable>


using namespace IR;
using namespace Runtime;
using boost::asio::io_service;

namespace eosio { namespace chain {

   FunctionInstance *resolve_intrinsic(const string& name);

   struct root_resolver : Runtime::Resolver
   {
      bool resolve(const string& mod_name,
                   const string& export_name,
                   ObjectType type,
                   ObjectInstance*& out) override
      {
         if (mod_name == "env" && type.kind == ObjectKind::function ) {
            try {
               auto* intrinsic = resolve_intrinsic(export_name);
               if (intrinsic != nullptr) {
                  out = asObject(intrinsic);
                  return true;
               }
            } FC_RETHROW_EXCEPTIONS(error, "unresolvable symbol ${module}.${export}", ("module",mod_name)("export",export_name));
         }
         return false;
      }
   };

   /**
    *  Implementation class for the wasm cache
    *  it is responsible for compiling and storing instances of wasm code for use
    *
    */
   struct wasm_cache_impl {
      wasm_cache_impl()
      :_ios()
      ,_work(_ios)
      {
         _utility_thread = std::thread([](io_service* ios){
            ios->run();
         }, &_ios);
      }

      /**
       * this must wait for all work to be done otherwise it may destroy memory
       * referenced by other threads
       *
       * Expectations on wasm_cache dictate that all available code has been
       * returned before this can be destroyed
       */
      ~wasm_cache_impl() {
         _work.reset();
         _ios.stop();
      }

      /**
       * internal tracking structure which deduplicates memory images
       * and tracks available vs in-use entries.
       *
       * The instances array has two sections, "available" instances
       * are in the front of the vector and anything at an index of
       * available_instances or greater is considered "in use"
       *
       * instances are stored as pointers so that their positions
       * in the array can be moved without invaliding references to
       * the instance handed out to other threads
       */
      struct code_info {
         // a clean image of the memory used to sanitize things on checkin
         size_t mem_start           = 0;
         size_t mem_end             = 1<<16;
         vector<char> mem_image;

         // all existing instances of this code
         vector<unique_ptr<wasm_cache::entry>> instances;
         size_t available_instances = 0;
      };

      /**
       * Convenience method for running code with the _cache_lock and releaseint that lock
       * when the code completes
       *
       * @param f - lambda to execute
       * @return - varies depending on the signature of the lambda
       */
      template<typename F>
      auto with_lock(F f) {
         std::lock_guard<std::mutex> lock(_cache_lock);
         return f();
      };

      /**
       * Fetch the tracking struct given a code_id if it exists
       *
       * @param code_id
       * @return
       */
      optional<code_info&> fetch_info(const digest_type& code_id) {
         return with_lock([this, &](){
            auto iter = _cache.find(code_id);
            if (iter != _cache.end()) {
               return optional<code_info&>(iter->second);
            }

            return optional<code_info&>();
         });
      }

      /**
       * Opportunistically fetch an available instance of the code;
       * @param code_id - the id of the code to fetch
       * @return - reference to the entry when one is available
       */
      optional<wasm_cache::entry&> try_fetch_entry(const digest_type& code_id) {
         return with_lock([this, &](){
            auto iter = _cache.find(code_id);
            if (iter != _cache.end() && iter->second.available_instances > 0) {
               return optional<wasm_cache::entry&>(iter->second.instances.at(iter->second.available_instances--));
            }

            return optional<wasm_cache::entry&>();
         });
      }

      /**
       * Fetch a copy of the code, this is guaranteed to return an entry IF the code is compilable.
       * In order to do that in safe way this code may cause the calling thread to sleep while a new
       * version of the code is compiled and inserted into the cache
       *
       * @param code_id - the id of the code to fetch
       * @param wasm_binary - the binary for the wasm
       * @param wasm_binary_size - the size of the binary
       * @return reference to a usable cache entry
       */
      wasm_cache::entry& fetch_entry(const digest_type& code_id, const char* wasm_binary, size_t wasm_binary_size) {
         std::condition_variable condition;
         optional<wasm_cache::entry&> result;
         std::exception_ptr error;

         // compilation is not thread safe, so we dispatch it to a io_service running on a single thread to
         // queue up and synchronize compilations
         _ios.post([this,&](){
            // check to see if someone returned what we need before making a new one
            auto pending_result = try_fetch_entry(code_id);
            std::exception_ptr pending_error;

            if (!pending_result) {
               // time to compile a brand new (maybe first) copy of this code
               unique_ptr<Module> module = std::make_unique(Module());
               unique_ptr<ModuleInstance> instance;
               size_t mem_end;
               vector<char> mem_image;

               try {
                  Serialization::MemoryInputStream stream((const U8 *) wasm_binary, wasm_binary_size);
                  WASM::serializeWithInjection(stream, *module.get());

                  root_resolver resolver;
                  LinkResult link_result = linkModule(*module.get(), resolver);
                  instance = instantiateModule(*module.get(), move(link_result.resolvedImports));
                  FC_ASSERT(instance != nullptr);

                  auto current_memory = Runtime::getDefaultMemory(instance.get());

                  char *mem_ptr = &memoryRef<char>(current_memory, 0);
                  const auto allocated_memory = Runtime::getDefaultMemorySize(instance.get());
                  for (uint64_t i = 0; i < allocated_memory; ++i) {
                     if (mem_ptr[i]) {
                        mem_end = i + 1;
                     }
                  }

                  mem_image.resize(mem_end);
                  memcpy(mem_image.data(), mem_ptr, mem_end);
               } catch (...) {
                  pending_error = std::current_exception();
               }

               if (pending_error == nullptr) {
                  // grab the lock and put this in the cache as unavailble
                  with_lock([this, &]() {
                     // find or create a new entry
                     auto iter = _cache.emplace(code_id, code_info {
                        .mem_end = mem_end,
                        .mem_image = std::move(mem_image)
                     }).first;

                     iter->second.instances.emplace_back(std::make_unique(std::move(module), std::move(instance)));
                     pending_result = optional<wasm_cache::entry &>(*iter->second.instances.back());
                  });
               }
            }

            // publish result under lock
            with_lock([&](){
               if (pending_error != nullptr) {
                  error = pending_error;
               } else {
                  result = pending_result;
               }
            });

            condition.notify_all();
         });

         // wait for the other thread to compile a copy for us
         {
            std::unique_lock<std::mutex> lock(_cache_lock);
            condition.wait(lock, [&]{
               return error != nullptr || result.valid();
            });
         }

         try {
            if (error != nullptr) {
               std::rethrow_exception(error);
            } else {
               return *result;
            }
         } FC_RETHROW_EXCEPTIONS(error, "error compiling WASM for code with hash: ${code_id}", ("code_id", code_id));
      }

      /**
       * return an entry to the cache.  The entry is presumed to come back in a "dirty" state and must be
       * sanitized before returning to the "available" state.  This sanitization is done asynchronously so
       * as not to delay the current executing thread.
       *
       * @param code_id - the code Id associated with the instance
       * @param entry - the entry to return
       */
      void return_entry(const digest_type& code_id, wasm_cache::entry& entry) {
         _ios.post([this,&](){
            // sanitize by reseting the memory that may now be dirty
            auto& info = *fetch_info(code_id);
            char* memstart = &memoryRef<char>( getDefaultMemory(entry.instance.get()), 0 );
            memset( memstart + info.mem_end, 0, ((1<<16) - info.mem_end) );
            memcpy( memstart, info.mem_image.data(), info.mem_end);

            // under a lock, put this entry back in the available instances side of the instances vector
            with_lock([this,&](){
               // walk the vector and find this entry
               auto iter = info.instances.begin();
               while (iter->get() != &entry) {
                  ++iter;
               }

               FC_ASSERT(iter != info.instances.end(), "Checking in a WASM enty that was not created properly!");

               auto first_unavailable = (info.instances.begin() + info.available_instances);
               if (iter != first_unavailable) {
                  std::swap(iter, first_unavailable);
               }
               info.available_instances++;
            });
         });
      }

      // mapping of digest to an entry for the code
      map<digest_type, code_info> _cache;
      std::mutex _cache_lock;

      // compilation and cleanup thread
      std::thread _utility_thread;
      io_service _ios;
      optional<io_service::work> _work;
   };

   wasm_cache::wasm_cache()
      :_my( new wasm_cache_impl() ) {
   }

   std::mutex                     global_load_mutex;
   wasm_cache::entry &wasm_cache::checkout( const digest_type& code_id, const char* wasm_binary, size_t wasm_binary_size ) {
      // see if there is an avaialble entry in the cache
      optional<entry&> result = _my->try_fetch_entry(code_id);

      if (result) {
         return *result;
      }

      return _my->fetch_entry(code_id, wasm_binary, wasm_binary_size);
   }


   void wasm_cache::checkin(const digest_type& code_id, entry& code ) {
      _my->return_entry(code_id, code);
   }

   void wasm_interface_impl::call(const string& entry_point, const vector<Value>& args, wasm_cache::entry& code, apply_context &context)
   try {
      FunctionInstance* call = asFunctionNullable(getInstanceExport(code.instance.get(),entry_point) );
      if( !call ) {
         return;
      }

      FC_ASSERT( getFunctionType(call)->parameters.size() == 2 );

      current_context = wasm_context{ code, context };
      Runtime::invokeFunction(call,args);
      current_context.reset();

   } catch( const Runtime::Exception& e ) {
      FC_THROW_EXCEPTION(wasm_execution_error,
                         "cause: ${cause}\n${callstack}",
                         ("cause", string(describeExceptionCause(e.cause)))
                         ("callstack", e.callStack));
   } FC_CAPTURE_AND_RETHROW()

   wasm_interface::wasm_interface()
      :my( new wasm_interface_impl() ) {
   }

   void wasm_interface::init( wasm_cache::entry& code, apply_context& context ) {
      my->call("init", {}, code, context);
   }

   void wasm_interface::apply( wasm_cache::entry& code, apply_context& context ) {
      vector<Value> args = { Value(uint64_t(context.act.scope)),
                             Value(uint64_t(context.act.name)) };
      my->call("apply", args, code, context);
   }

   void wasm_interface::error( wasm_cache::entry& code, apply_context& context ) {
      vector<Value> args = { /* */ };
      my->call("error", args, code, context);
   }

#if 0
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
      /*
      auto& wasm  = wasm_interface::get();
      auto  mem   = wasm.memory();
      auto& value = memoryRef<unsigned __int128>( mem, val );
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
#endif

#if defined(assert)
#undef assert
#endif

class intrinsics {
   public:
      intrinsics(wasm_interface &wasm)
      :wasm(wasm)
      ,context(*intrinsics_accessor::get_module_state(wasm).context)
      {}

      int read_action(array_ptr<char> memory, size_t size) {
         FC_ASSERT(size > 0);
         int minlen = std::min<int>(context.act.data.size(), size);
         memcpy((void *)memory, context.act.data.data(), minlen);
         return minlen;
      }

      void assert(bool condition, char const* str) {
         std::string message( str );
         if( !condition ) edump((message));
         FC_ASSERT( condition, "assertion failed: ${s}", ("s",message));
      }

   private:
      wasm_interface& wasm;
      apply_context& context;
};

static map<string, unique_ptr<Intrinsics::Function>> intrinsic_registry;
static void lazy_register_intrinsics()
{
   if (intrinsic_registry.size() !=0 ) {
      return;
   }
   REGISTER_INTRINSICS((intrinsic_registry, intrinsics), (read_action)(assert));
};

FunctionInstance *resolve_intrinsic(const string& name) {
   lazy_register_intrinsics();
   auto iter = intrinsic_registry.find(name);
   if (iter != intrinsic_registry.end()) {
      return (iter->second)->function;
   }

   return nullptr;
}

} } /// eosio::chain
