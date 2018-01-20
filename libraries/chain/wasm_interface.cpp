#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <eosio/chain/wasm_interface_private.hpp>
#include <fc/exception/exception.hpp>

#include <Runtime/Runtime.h>
#include "IR/Module.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "IR/Operators.h"
#include "IR/Validate.h"
#include "IR/Types.h"
#include "Runtime/Runtime.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <mutex>
#include <thread>
#include <condition_variable>



using namespace IR;
using namespace Runtime;
using boost::asio::io_service;

#if 0
   // account.h/hpp expected account API balance interchange format
   // must match account.hpp account_balance definition
   PACKED_STRUCT(
   struct account_balance
   {
      /**
      * Name of the account who's balance this is
      */
      account_name account;

      /**
      * Balance for this account
      */
      asset eos_balance;

      /**
      * Staked balance for this account
      */
      asset staked_balance;

      /**
      * Unstaking balance for this account
      */
      asset unstaking_balance;

      /**
      * Time at which last unstaking occurred for this account
      */
      time last_unstaking_time;
   })
#endif

namespace eosio { namespace chain {
   using namespace contracts;

   /**
    * Integration with the WASM Linker to resolve our intrinsics
    */
   struct root_resolver : Runtime::Resolver
   {
      bool resolve(const string& mod_name,
                   const string& export_name,
                   ObjectType type,
                   ObjectInstance*& out) override
      {
         // Try to resolve an intrinsic first.
         if(IntrinsicResolver::singleton.resolve(mod_name,export_name,type, out)) {
            return true;
         }

         FC_ASSERT( !"unresolvable", "${module}.${export}", ("module",mod_name)("export",export_name) );
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
         Runtime::init();

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
         _utility_thread.join();
         freeUnreferencedObjects({});
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
         code_info( size_t mem_end, vector<char>&& mem_image )
         :mem_end(mem_end),mem_image(std::forward<vector<char>>(mem_image))
         {}

         // a clean image of the memory used to sanitize things on checkin
         size_t mem_start           = 0;
         size_t mem_end             = 1<<16;
         vector<char> mem_image;

         // all existing instances of this code
         vector<unique_ptr<wasm_cache::entry>> instances;
         size_t available_instances = 0;
      };

      using optional_info_ref = optional<std::reference_wrapper<code_info>>;
      using optional_entry_ref = optional<std::reference_wrapper<wasm_cache::entry>>;

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
      optional_info_ref fetch_info(const digest_type& code_id) {
         return with_lock([&,this](){
            auto iter = _cache.find(code_id);
            if (iter != _cache.end()) {
               return optional_info_ref(iter->second);
            }

            return optional_info_ref();
         });
      }

      /**
       * Opportunistically fetch an available instance of the code;
       * @param code_id - the id of the code to fetch
       * @return - reference to the entry when one is available
       */
      optional_entry_ref try_fetch_entry(const digest_type& code_id) {
         return with_lock([&,this](){
            auto iter = _cache.find(code_id);
            if (iter != _cache.end() && iter->second.available_instances > 0) {
               auto &ptr = iter->second.instances.at(--(iter->second.available_instances));
               return optional_entry_ref(*ptr);
            }

            return optional_entry_ref();
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
         optional_entry_ref result;
         std::exception_ptr error;

         // compilation is not thread safe, so we dispatch it to a io_service running on a single thread to
         // queue up and synchronize compilations
         _ios.post([&,this](){
            // check to see if someone returned what we need before making a new one
            auto pending_result = try_fetch_entry(code_id);
            std::exception_ptr pending_error;

            if (!pending_result) {
               // time to compile a brand new (maybe first) copy of this code
               Module* module = new Module();
               ModuleInstance* instance = nullptr;
               size_t mem_end = 0;
               vector<char> mem_image;

               try {
                  Serialization::MemoryInputStream stream((const U8 *) wasm_binary, wasm_binary_size);
                  #warning TODO: restore checktime injection?
                  WASM::serialize(stream, *module);

                  root_resolver resolver;
                  LinkResult link_result = linkModule(*module, resolver);
                  instance = instantiateModule(*module, std::move(link_result.resolvedImports));
                  FC_ASSERT(instance != nullptr);

                  MemoryInstance* current_memory = Runtime::getDefaultMemory(instance);

                  if(current_memory) {
                     char *mem_ptr = &memoryRef<char>(current_memory, 0);
                     const auto allocated_memory = Runtime::getDefaultMemorySize(instance);
                     for (uint64_t i = 0; i < allocated_memory; ++i) {
                        if (mem_ptr[i])
                           mem_end = i + 1;
                     }
                     mem_image.resize(mem_end);
                     memcpy(mem_image.data(), mem_ptr, mem_end);
                  }
                  
               } catch (...) {
                  pending_error = std::current_exception();
               }

               if (pending_error == nullptr) {
                  // grab the lock and put this in the cache as unavailble
                  with_lock([&,this]() {
                     // find or create a new entry
                     auto iter = _cache.emplace(code_id, code_info(mem_end, std::move(mem_image))).first;

                     iter->second.instances.emplace_back(std::make_unique<wasm_cache::entry>(instance, module));
                     pending_result = optional_entry_ref(*iter->second.instances.back().get());
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
               return (*result).get();
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
         _ios.post([&,code_id,this](){
            // sanitize by reseting the memory that may now be dirty
            auto& info = (*fetch_info(code_id)).get();
            if(getDefaultMemory(entry.instance)) {
               char* memstart = &memoryRef<char>( getDefaultMemory(entry.instance), 0 );
               memset( memstart + info.mem_end, 0, ((1<<16) - info.mem_end) );
               memcpy( memstart, info.mem_image.data(), info.mem_end);
            }
            resetGlobalInstances(entry.instance);

            // under a lock, put this entry back in the available instances side of the instances vector
            with_lock([&,this](){
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

   wasm_cache::~wasm_cache() = default;

   wasm_cache::entry &wasm_cache::checkout( const digest_type& code_id, const char* wasm_binary, size_t wasm_binary_size ) {
      // see if there is an available entry in the cache
      auto result = _my->try_fetch_entry(code_id);
      if (result) {
         return (*result).get();
      }
      return _my->fetch_entry(code_id, wasm_binary, wasm_binary_size);
   }


   void wasm_cache::checkin(const digest_type& code_id, entry& code ) {
      MemoryInstance* default_mem = Runtime::getDefaultMemory(code.instance);
      if(default_mem)
         Runtime::shrinkMemory(default_mem, Runtime::getMemoryNumPages(default_mem) - 1);
      _my->return_entry(code_id, code);
   }

   /**
    * RAII wrapper to make sure that the context is cleaned up on exception
    */
   struct scoped_context {
      template<typename ...Args>
      scoped_context(optional<wasm_context> &context, Args&... args)
      :context(context)
      {
         context = wasm_context{ args... };
      }

      ~scoped_context() {
         context.reset();
      }

      optional<wasm_context>& context;
   };

   void wasm_interface_impl::call(const string& entry_point, const vector<Value>& args, wasm_cache::entry& code, apply_context& context)
   try {
      FunctionInstance* call = asFunctionNullable(getInstanceExport(code.instance,entry_point) );
      if( !call ) {
         return;
      }

      FC_ASSERT( getFunctionType(call)->parameters.size() == args.size() );

      auto context_guard = scoped_context(current_context, code, context);
      runInstanceStartFunc(code.instance);
      Runtime::invokeFunction(call,args);
   } catch( const Runtime::Exception& e ) {
      FC_THROW_EXCEPTION(wasm_execution_error,
                         "cause: ${cause}\n${callstack}",
                         ("cause", string(describeExceptionCause(e.cause)))
                         ("callstack", e.callStack));
   } FC_CAPTURE_AND_RETHROW()

   wasm_interface::wasm_interface()
      :my( new wasm_interface_impl() ) {
   }

   wasm_interface& wasm_interface::get() {
      thread_local wasm_interface* single = nullptr;
      if( !single ) {
         single = new wasm_interface();
      }
      return *single;
   }

   void wasm_interface::apply( wasm_cache::entry& code, apply_context& context ) {
      if (context.act.account == config::system_account_name && context.act.name == N(setcode)) {
         my->call("init", {}, code, context);
      } else {
         vector<Value> args = {Value(uint64_t(context.act.account)),
                               Value(uint64_t(context.act.name))};
         my->call("apply", args, code, context);
      }
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



DEFINE_INTRINSIC_FUNCTION1(env,printi128,printi128,none,i32,val) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;
  auto& value = memoryRef<unsigned __int128>( mem, val );
  fc::uint128_t v(value>>64, uint64_t(value) );
  std::cerr << fc::variant(v).get_string();
}
DEFINE_INTRINSIC_FUNCTION1(env,printn,printn,none,i64,val) {
  std::cerr << name(val).to_string();
}

DEFINE_INTRINSIC_FUNCTION1(env,prints,prints,none,i32,charptr) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  const char* str = &memoryRef<const char>( mem, charptr );

  std::cerr << std::string( str, strnlen(str, wasm.current_state->mem_end-charptr) );
}

DEFINE_INTRINSIC_FUNCTION2(env,prints_l,prints_l,none,i32,charptr,i32,len) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  const char* str = &memoryRef<const char>( mem, charptr );

  std::cerr << std::string( str, len );
}

DEFINE_INTRINSIC_FUNCTION2(env,printhex,printhex,none,i32,data,i32,datalen) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  char* buff = memoryArrayPtr<char>(mem, data, datalen);
  std::cerr << fc::to_hex(buff, datalen);
}


DEFINE_INTRINSIC_FUNCTION1(env,free,free,none,i32,ptr) {
}

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
DEFINE_INTRINSIC_FUNCTION2(env,account_balance_get,account_balance_get,i32,i32,charptr,i32,len) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  const uint32_t account_balance_size = sizeof(account_balance);
  FC_ASSERT( len == account_balance_size, "passed in len ${len} is not equal to the size of an account_balance struct == ${real_len}", ("len",len)("real_len",account_balance_size) );

  account_balance& total_balance = memoryRef<account_balance>( mem, charptr );

  wasm.current_apply_context->require_scope(total_balance.account);

  auto& db = wasm.current_apply_context->db;
  auto* balance        = db.find< balance_object,by_owner_name >( total_balance.account );
  auto* staked_balance = db.find<staked_balance_object,by_owner_name>( total_balance.account );

  if (balance == nullptr || staked_balance == nullptr)
     return false;

  total_balance.eos_balance          = asset(balance->balance, EOS_SYMBOL);
  total_balance.staked_balance       = asset(staked_balance->staked_balance);
  total_balance.unstaking_balance    = asset(staked_balance->unstaking_balance);
  total_balance.last_unstaking_time  = staked_balance->last_unstaking_time;

  return true;
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

class context_aware_api {
   public:
      context_aware_api(wasm_interface& wasm)
      :context(intrinsics_accessor::get_context(wasm).context), code(intrinsics_accessor::get_context(wasm).code),
       sbrk_bytes(intrinsics_accessor::get_context(wasm).sbrk_bytes)
      {}

   protected:
      uint32_t&          sbrk_bytes;
      wasm_cache::entry& code;
      apply_context&     context;
};

class system_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      void assert(bool condition, null_terminated_ptr str) {
         std::string message( str );
         if( !condition ) edump((message));
         FC_ASSERT( condition, "assertion failed: ${s}", ("s",message));
      }

      fc::time_point_sec now() {
         return context.controller.head_block_time();
      }
};

class action_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      int read_action(array_ptr<char> memory, size_t size) {
         FC_ASSERT(size > 0);
         int minlen = std::min<size_t>(context.act.data.size(), size);
         memcpy((void *)memory, context.act.data.data(), minlen);
         return minlen;
      }

      int action_size() {
         return context.act.data.size();
      }

      const name& current_receiver() {
         return context.receiver;
      }

      fc::time_point_sec publication_time() {
         return context.trx_meta.published;
      }

      name current_sender() {
         if (context.trx_meta.sender) {
            return *context.trx_meta.sender;
         } else {
            return name();
         }
      }
};

class console_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      void prints(null_terminated_ptr str) {
         context.console_append<const char*>(str);
      }

      void prints_l(array_ptr<const char> str, size_t str_len ) {
         context.console_append(string(str, str_len));
      }

      void printi(uint64_t val) {
         context.console_append(val);
      }

      void printi128(const unsigned __int128& val) {
         fc::uint128_t v(val>>64, uint64_t(val) );
         context.console_append(fc::variant(v).get_string());
      }

      void printd( wasm_double val ) {
         context.console_append(val.str());
      }

      void printn(const name& value) {
         context.console_append(value.to_string());
      }

      void printhex(array_ptr<const char> data, size_t data_len ) {
         context.console_append(fc::to_hex(data, data_len));
      }
};

template<typename ObjectType>
class db_api : public context_aware_api {
   using KeyType = typename ObjectType::key_type;
   static constexpr int KeyCount = ObjectType::number_of_keys;
   using KeyArrayType = KeyType[KeyCount];
   using ContextMethodType = int(apply_context::*)(const table_id_object&, const KeyType*, const char*, size_t);

   private:
      int call(ContextMethodType method, const scope_name& scope, const name& table, array_ptr<const char> data, size_t data_len) {
         const auto& t_id = context.find_or_create_table(scope, context.receiver, table);
         FC_ASSERT(data_len >= KeyCount * sizeof(KeyType), "Data is not long enough to contain keys");
         const KeyType* keys = reinterpret_cast<const KeyType *>((const char *)data);

         const char* record_data =  ((const char*)data) + sizeof(KeyArrayType);
         size_t record_len = data_len - sizeof(KeyArrayType);
         return (context.*(method))(t_id, keys, record_data, record_len) + sizeof(KeyArrayType);
      }

   public:
      using context_aware_api::context_aware_api;

      int store(const scope_name& scope, const name& table, array_ptr<const char> data, size_t data_len) {
         auto res = call(&apply_context::store_record<ObjectType>, scope, table, data, data_len);
         //ilog("STORE [${scope},${code},${table}] => ${res} :: ${HEX}", ("scope",scope)("code",context.receiver)("table",table)("res",res)("HEX", fc::to_hex(data, data_len)));
         return res;

      }

      int update(const scope_name& scope, const name& table, array_ptr<const char> data, size_t data_len) {
         return call(&apply_context::update_record<ObjectType>, scope, table, data, data_len);
      }

      int remove(const scope_name& scope, const name& table, const KeyArrayType &keys) {
         const auto& t_id = context.find_or_create_table(scope, context.receiver, table);
         return context.remove_record<ObjectType>(t_id, keys);
      }
};

template<typename IndexType, typename Scope>
class db_index_api : public context_aware_api {
   using KeyType = typename IndexType::value_type::key_type;
   static constexpr int KeyCount = IndexType::value_type::number_of_keys;
   using KeyArrayType = KeyType[KeyCount];
   using ContextMethodType = int(apply_context::*)(const table_id_object&, KeyType*, char*, size_t);


   int call(ContextMethodType method, const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
      auto maybe_t_id = context.find_table(scope, context.receiver, table);
      if (maybe_t_id == nullptr) {
         return -1;
      }

      const auto& t_id = *maybe_t_id;
      FC_ASSERT(data_len >= KeyCount * sizeof(KeyType), "Data is not long enough to contain keys");
      KeyType* keys = reinterpret_cast<KeyType *>((char *)data);

      char* record_data =  ((char*)data) + sizeof(KeyArrayType);
      size_t record_len = data_len - sizeof(KeyArrayType);

      auto res = (context.*(method))(t_id, keys, record_data, record_len);
      if (res != -1) {
         res += sizeof(KeyArrayType);
      }
      return res;
   }

   public:
      using context_aware_api::context_aware_api;

      int load(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         auto res = call(&apply_context::load_record<IndexType, Scope>, scope, code, table, data, data_len);
         //ilog("LOAD [${scope},${code},${table}] => ${res} :: ${HEX}", ("scope",scope)("code",code)("table",table)("res",res)("HEX", fc::to_hex(data, data_len)));
         return res;
      }

      int front(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::front_record<IndexType, Scope>, scope, code, table, data, data_len);
      }

      int back(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::back_record<IndexType, Scope>, scope, code, table, data, data_len);
      }

      int next(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::next_record<IndexType, Scope>, scope, code, table, data, data_len);
      }

      int previous(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::previous_record<IndexType, Scope>, scope, code, table, data, data_len);
      }

      int lower_bound(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::lower_bound_record<IndexType, Scope>, scope, code, table, data, data_len);
      }

      int upper_bound(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::upper_bound_record<IndexType, Scope>, scope, code, table, data, data_len);
      }

};

class memory_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;
     
      char* memcpy( array_ptr<char> dest, array_ptr<const char> src, size_t length) {
         return (char *)::memcpy(dest, src, length);
      }

      int memcmp( array_ptr<const char> dest, array_ptr<const char> src, size_t length) {
         return ::memcmp(dest, src, length);
      }

      char* memset( array_ptr<char> dest, int value, size_t length ) {
         return (char *)::memset( dest, value, length );
      }

      uint32_t sbrk(int num_bytes) {
         // TODO: omitted checktime function from previous version of sbrk, may need to be put back in at some point
         constexpr uint32_t NBPPL2  = IR::numBytesPerPageLog2;
         constexpr uint32_t MAX_MEM = 1024 * 1024;

         MemoryInstance*  default_mem    = Runtime::getDefaultMemory(code.instance);
         if(!default_mem)
            throw eosio::chain::page_memory_error();

         const uint32_t         num_pages      = Runtime::getMemoryNumPages(default_mem);
         const uint32_t         min_bytes      = (num_pages << NBPPL2) > UINT32_MAX ? UINT32_MAX : num_pages << NBPPL2;
         const uint32_t         prev_num_bytes = sbrk_bytes; //_num_bytes;
         
         // round the absolute value of num_bytes to an alignment boundary
         num_bytes = (num_bytes + 7) & ~7;

         if ((num_bytes > 0) && (prev_num_bytes > (MAX_MEM - num_bytes)))  // test if allocating too much memory (overflowed)
            throw eosio::chain::page_memory_error();
         else if ((num_bytes < 0) && (prev_num_bytes < (min_bytes - num_bytes))) // test for underflow
            throw eosio::chain::page_memory_error(); 

         // update the number of bytes allocated, and compute the number of pages needed
         sbrk_bytes += num_bytes;
         const uint32_t num_desired_pages = (sbrk_bytes + IR::numBytesPerPage - 1) >> NBPPL2;

         // grow or shrink the memory to the desired number of pages
         if (num_desired_pages > num_pages)
            Runtime::growMemory(default_mem, num_desired_pages - num_pages);
         else if (num_desired_pages < num_pages)
            Runtime::shrinkMemory(default_mem, num_pages - num_desired_pages);

         return prev_num_bytes;
      }
};

class transaction_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      void send_inline( array_ptr<char> data, size_t data_len ) {
         // TODO: use global properties object for dynamic configuration of this default_max_gen_trx_size
         FC_ASSERT( data_len < config::default_max_inline_action_size, "inline action too big" );

         action act;
         fc::raw::unpack<action>(data, data_len, act);
         context.execute_inline(std::move(act));
      }


      void send_deferred( uint32_t sender_id, const fc::time_point_sec& execute_after, array_ptr<char> data, size_t data_len ) {
         try {
            // TODO: use global properties object for dynamic configuration of this default_max_gen_trx_size
            FC_ASSERT(data_len < config::default_max_gen_trx_size, "generated transaction too big");

            deferred_transaction dtrx;
            fc::raw::unpack<transaction>(data, data_len, dtrx);
            dtrx.sender = context.receiver;
            dtrx.sender_id = sender_id;
            dtrx.execute_after = execute_after;
            context.execute_deferred(std::move(dtrx));
         } FC_CAPTURE_AND_RETHROW((fc::to_hex(data, data_len)));
      }

};

REGISTER_INTRINSICS(system_api,
   (assert,      void(int, int))
   (now,          int())
);

REGISTER_INTRINSICS(action_api,
   (read_action,            int(int, int)  )
   (action_size,            int()          )
   (current_receiver,   int64_t()          )
   (publication_time,   int32_t()          )
   (current_sender,     int64_t()          )
);

REGISTER_INTRINSICS(apply_context,
   (require_write_lock,    void(int64_t)   )
   (require_read_lock,     void(int64_t, int64_t)   )
   (require_recipient,     void(int64_t)   )
   (require_authorization, void(int64_t), "require_auth", void(apply_context::*)(const account_name&)const)
);

REGISTER_INTRINSICS(console_api,
   (prints,                void(int)       )
   (prints_l,              void(int, int)  )
   (printi,                void(int64_t)   )
   (printi128,             void(int)       )
   (printd,                void(int64_t)   )
   (printn,                void(int64_t)   )
   (printhex,              void(int, int)  )
);

REGISTER_INTRINSICS(transaction_api,
   (send_inline,           void(int, int)  )
   (send_deferred,         void(int, int, int, int)  )
);

REGISTER_INTRINSICS(memory_api,
   (memcpy,                 int(int, int, int)   )
   (memcmp,                 int(int, int, int)   )
   (memset,                 int(int, int, int)   )
   (sbrk,                   int(int)             )
);


#define DB_METHOD_SEQ(SUFFIX) \
   (store,        int32_t(int64_t, int64_t, int, int),            "store_"#SUFFIX )\
   (update,       int32_t(int64_t, int64_t, int, int),            "update_"#SUFFIX )\
   (remove,       int32_t(int64_t, int64_t, int),                 "remove_"#SUFFIX )

#define DB_INDEX_METHOD_SEQ(SUFFIX)\
   (load,         int32_t(int64_t, int64_t, int64_t, int, int),   "load_"#SUFFIX )\
   (front,        int32_t(int64_t, int64_t, int64_t, int, int),   "front_"#SUFFIX )\
   (back,         int32_t(int64_t, int64_t, int64_t, int, int),   "back_"#SUFFIX )\
   (next,         int32_t(int64_t, int64_t, int64_t, int, int),   "next_"#SUFFIX )\
   (previous,     int32_t(int64_t, int64_t, int64_t, int, int),   "previous_"#SUFFIX )\
   (lower_bound,  int32_t(int64_t, int64_t, int64_t, int, int),   "lower_bound_"#SUFFIX )\
   (upper_bound,  int32_t(int64_t, int64_t, int64_t, int, int),   "upper_bound_"#SUFFIX )\

using db_api_key_value_object                                 = db_api<key_value_object>;
using db_api_keystr_value_object                              = db_api<keystr_value_object>;
using db_api_key128x128_value_object                          = db_api<key128x128_value_object>;
using db_api_key64x64x64_value_object                         = db_api<key64x64x64_value_object>;
using db_index_api_key_value_index_by_scope_primary           = db_index_api<key_value_index,by_scope_primary>;
using db_index_api_keystr_value_index_by_scope_primary        = db_index_api<keystr_value_index,by_scope_primary>;
using db_index_api_key128x128_value_index_by_scope_primary    = db_index_api<key128x128_value_index,by_scope_primary>;
using db_index_api_key128x128_value_index_by_scope_secondary  = db_index_api<key128x128_value_index,by_scope_secondary>;
using db_index_api_key64x64x64_value_index_by_scope_primary   = db_index_api<key64x64x64_value_index,by_scope_primary>;
using db_index_api_key64x64x64_value_index_by_scope_secondary = db_index_api<key64x64x64_value_index,by_scope_secondary>;
using db_index_api_key64x64x64_value_index_by_scope_tertiary  = db_index_api<key64x64x64_value_index,by_scope_tertiary>;

REGISTER_INTRINSICS(db_api_key_value_object,         DB_METHOD_SEQ(i64));
REGISTER_INTRINSICS(db_api_keystr_value_object,      DB_METHOD_SEQ(str));
REGISTER_INTRINSICS(db_api_key128x128_value_object,  DB_METHOD_SEQ(i128i128));
REGISTER_INTRINSICS(db_api_key64x64x64_value_object, DB_METHOD_SEQ(i64i64i64));

REGISTER_INTRINSICS(db_index_api_key_value_index_by_scope_primary,           DB_INDEX_METHOD_SEQ(i64));
REGISTER_INTRINSICS(db_index_api_keystr_value_index_by_scope_primary,        DB_INDEX_METHOD_SEQ(str));
REGISTER_INTRINSICS(db_index_api_key128x128_value_index_by_scope_primary,    DB_INDEX_METHOD_SEQ(primary_i128i128));
REGISTER_INTRINSICS(db_index_api_key128x128_value_index_by_scope_secondary,  DB_INDEX_METHOD_SEQ(secondary_i128i128));
REGISTER_INTRINSICS(db_index_api_key64x64x64_value_index_by_scope_primary,   DB_INDEX_METHOD_SEQ(primary_i64i64i64));
REGISTER_INTRINSICS(db_index_api_key64x64x64_value_index_by_scope_secondary, DB_INDEX_METHOD_SEQ(secondary_i64i64i64));
REGISTER_INTRINSICS(db_index_api_key64x64x64_value_index_by_scope_tertiary,  DB_INDEX_METHOD_SEQ(tertiary_i64i64i64));


} } /// eosio::chain
