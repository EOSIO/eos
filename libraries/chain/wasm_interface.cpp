#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <eosio/chain/wasm_interface_private.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <fc/exception/exception.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/io/raw.hpp>
#include <fc/utf8.hpp>

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
      { try {
         // Try to resolve an intrinsic first.
         if(IntrinsicResolver::singleton.resolve(mod_name,export_name,type, out)) {
            return true;
         }

         FC_ASSERT( !"unresolvable", "${module}.${export}", ("module",mod_name)("export",export_name) );
         return false;
      } FC_CAPTURE_AND_RETHROW( (mod_name)(export_name) ) }
   };

   /**
    *  Implementation class for the wasm cache
    *  it is responsible for compiling and storing instances of wasm code for use
    *
    */
   struct wasm_cache_impl {
      wasm_cache_impl()
      {
         check_wasm_opcode_dispositions();
         Runtime::init();
      }

      /**
       * this must wait for all work to be done otherwise it may destroy memory
       * referenced by other threads
       *
       * Expectations on wasm_cache dictate that all available code has been
       * returned before this can be destroyed
       */
      ~wasm_cache_impl() {
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
         code_info( vector<char>&& mem_image )
         :mem_image(std::forward<vector<char>>(mem_image))
         {}

         // a clean image of the memory used to sanitize things on checkin
         size_t mem_start           = 0;
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
      auto with_lock(std::mutex &l, F f) {
         std::lock_guard<std::mutex> lock(l);
         return f();
      };

      /**
       * Fetch the tracking struct given a code_id if it exists
       *
       * @param code_id
       * @return
       */
      optional_info_ref fetch_info(const digest_type& code_id) {
         return with_lock(_cache_lock, [&,this](){
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
         return with_lock(_cache_lock, [&,this](){
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
         with_lock(_compile_lock, [&,this](){
            // check to see if someone returned what we need before making a new one
            auto pending_result = try_fetch_entry(code_id);
            std::exception_ptr pending_error;

            if (!pending_result) {
               // time to compile a brand new (maybe first) copy of this code
               Module* module = new Module();
               ModuleInstance* instance = nullptr;
               vector<char> mem_image;

               try {
                  Serialization::MemoryInputStream stream((const U8 *) wasm_binary, wasm_binary_size);
                  WASM::serializeWithInjection(stream, *module);
                  validate_eosio_wasm_constraints(*module);

                  root_resolver resolver;
                  LinkResult link_result = linkModule(*module, resolver);
                  instance = instantiateModule(*module, std::move(link_result.resolvedImports));
                  FC_ASSERT(instance != nullptr);

                  //populate the module's data segments in to a vector so the initial state can be
                  // restored on each invocation
                  //Be Warned, this may need to be revisited when module imports make sense. The
                  // code won't handle data segments that initalize an imported memory which I think
                  // is valid.
                  for(const DataSegment& data_segment : module->dataSegments) {
                     FC_ASSERT(data_segment.baseOffset.type == InitializerExpression::Type::i32_const);
                     FC_ASSERT(module->memories.defs.size());
                     const U32  base_offset = data_segment.baseOffset.i32;
                     const Uptr memory_size = (module->memories.defs[0].type.size.min << IR::numBytesPerPageLog2);
                     if(base_offset >= memory_size || base_offset + data_segment.data.size() > memory_size)
                        FC_THROW_EXCEPTION(wasm_execution_error, "WASM data segment outside of valid memory range");
                     if(base_offset + data_segment.data.size() > mem_image.size())
                        mem_image.resize(base_offset + data_segment.data.size(), 0x00);
                     memcpy(mem_image.data() + base_offset, data_segment.data.data(), data_segment.data.size());
                  }
               } catch (...) {
                  pending_error = std::current_exception();
               }

               if (pending_error == nullptr) {
                  // grab the lock and put this in the cache as unavailble
                  with_lock(_cache_lock, [&,this]() {
                     // find or create a new entry
                     auto iter = _cache.emplace(code_id, code_info(std::move(mem_image))).first;

                     iter->second.instances.emplace_back(std::make_unique<wasm_cache::entry>(instance, module));
                     pending_result = optional_entry_ref(*iter->second.instances.back().get());
                  });
               }
            }

           if (pending_error != nullptr) {
              error = pending_error;
           } else {
              result = pending_result;
           }

         });


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
        auto& info = (*fetch_info(code_id)).get();
        // under a lock, put this entry back in the available instances side of the instances vector
        with_lock(_cache_lock, [&,this](){
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
      }

      //initialize the memory for a cache entry
      wasm_cache::entry& prepare_wasm_instance(wasm_cache::entry& wasm_cache_entry, const digest_type& code_id) {
         resetGlobalInstances(wasm_cache_entry.instance);
         MemoryInstance* memory_instance = getDefaultMemory(wasm_cache_entry.instance);
         if(memory_instance) {
            resetMemory(memory_instance, wasm_cache_entry.module->memories.defs[0].type);

            const code_info& info = (*fetch_info(code_id)).get();
            char* memstart = &memoryRef<char>(getDefaultMemory(wasm_cache_entry.instance), 0);
            memcpy(memstart, info.mem_image.data(), info.mem_image.size());
         }
         return wasm_cache_entry;
      }

      // mapping of digest to an entry for the code
      map<digest_type, code_info> _cache;
      std::mutex _cache_lock;

      // compilation lock
      std::mutex _compile_lock;
   };

   wasm_cache::wasm_cache()
      :_my( new wasm_cache_impl() ) {
   }

   wasm_cache::~wasm_cache() = default;

   wasm_cache::entry &wasm_cache::checkout( const digest_type& code_id, const char* wasm_binary, size_t wasm_binary_size ) {
      // see if there is an available entry in the cache
      auto result = _my->try_fetch_entry(code_id);
      if (result) {
         wasm_cache::entry& wasm_cache_entry = (*result).get();
         return _my->prepare_wasm_instance(wasm_cache_entry, code_id);
      }
      return _my->prepare_wasm_instance(_my->fetch_entry(code_id, wasm_binary, wasm_binary_size), code_id);
   }


   void wasm_cache::checkin(const digest_type& code_id, entry& code ) {
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
      vector<Value> args = {Value(uint64_t(context.act.account)),
                            Value(uint64_t(context.act.name))};
      my->call("apply", args, code, context);
   }

   void wasm_interface::error( wasm_cache::entry& code, apply_context& context ) {
      vector<Value> args = { /* */ };
      my->call("error", args, code, context);
   }

#if defined(assert)
   #undef assert
#endif

class context_aware_api {
   public:
      context_aware_api(wasm_interface& wasm, bool context_free = false )
      :context(intrinsics_accessor::get_context(wasm).context),
       code(intrinsics_accessor::get_context(wasm).code),
       sbrk_bytes(intrinsics_accessor::get_context(wasm).sbrk_bytes)
      {
         if( context.context_free )
            FC_ASSERT( context_free, "only context free api's can be used in this context" );
         context.used_context_free_api |= !context_free;
      }

      // For native contract compilation only
      context_aware_api(apply_context& ac, uint32_t& sb, wasm_cache::entry& c)
      :context(ac),
       code(c),
       sbrk_bytes(sb)
      {
      }

   protected:
      apply_context&     context;
      wasm_cache::entry& code;
      uint32_t&          sbrk_bytes;
};

class context_free_api : public context_aware_api {
   public:
      context_free_api( wasm_interface& wasm )
      :context_aware_api(wasm, true) { 
         /* the context_free_data is not available during normal application because it is prunable */
         FC_ASSERT( context.context_free, "this API may only be called from context_free apply" );
      }

      int get_context_free_data( uint32_t index, array_ptr<char> buffer, size_t buffer_size )const {
         return context.get_context_free_data( index, buffer, buffer_size );
      }
};
class privileged_api : public context_aware_api {
   public:
      privileged_api( wasm_interface& wasm )
      :context_aware_api(wasm)
      {
         FC_ASSERT( context.privileged, "${code} does not have permission to call this API", ("code",context.receiver) );
      }

      // For native contract compilation only
      privileged_api( apply_context& ac, uint32_t& sb, wasm_cache::entry& c )
      :context_aware_api(ac, sb, c)
      {
         FC_ASSERT( context.privileged, "${code} does not have permission to call this API", ("code",context.receiver) );
      }


      /**
       *  This should schedule the feature to be activated once the
       *  block that includes this call is irreversible. It should
       *  fail if the feature is already pending.
       *
       *  Feature name should be base32 encoded name. 
       */
      void activate_feature( int64_t feature_name ) {
         FC_ASSERT( !"Unsupported Hardfork Detected" );
      }

      /**
       * This should return true if a feature is active and irreversible, false if not.
       *
       * Irreversiblity by fork-database is not consensus safe, therefore, this defines
       * irreversiblity only by block headers not by BFT short-cut.
       */
      int is_feature_active( int64_t feature_name ) {
         return false;
      }

      void set_resource_limits( account_name account, 
                                int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight,
                                int64_t cpu_usec_per_period ) {
         auto& buo = context.db.get<bandwidth_usage_object,by_owner>( account );
         FC_ASSERT( buo.db_usage <= ram_bytes, "attempt to free too much space" );

         auto& gdp = context.controller.get_dynamic_global_properties();
         context.mutable_db.modify( gdp, [&]( auto& p ) {
           p.total_net_weight -= buo.net_weight;
           p.total_net_weight += net_weight;
           p.total_cpu_weight -= buo.cpu_weight;
           p.total_cpu_weight += cpu_weight;
           p.total_db_reserved -= buo.db_reserved_capacity;
           p.total_db_reserved += ram_bytes;
         });

         context.mutable_db.modify( buo, [&]( auto& o ){
            o.net_weight = net_weight;
            o.cpu_weight = cpu_weight;
            o.db_reserved_capacity = ram_bytes;
         });
      }


      void get_resource_limits( account_name account, 
                                uint64_t& ram_bytes, uint64_t& net_weight, uint64_t cpu_weight ) {
      }
                                               
      void set_active_producers( array_ptr<char> packed_producer_schedule, size_t datalen) {
         datastream<const char*> ds( packed_producer_schedule, datalen );
         producer_schedule_type psch;
         fc::raw::unpack(ds, psch);
         context.mutable_db.modify( context.controller.get_global_properties(), 
            [&]( auto& gprops ) {
                 gprops.new_active_producers = psch;
         });
      }

      bool is_privileged( account_name n )const {
         return context.db.get<account_object, by_name>( n ).privileged;
      }
      bool is_frozen( account_name n )const {
         return context.db.get<account_object, by_name>( n ).frozen;
      }
      void set_privileged( account_name n, bool is_priv ) {
         const auto& a = context.db.get<account_object, by_name>( n );
         context.mutable_db.modify( a, [&]( auto& ma ){
            ma.privileged = is_priv;
         });
      }

      void freeze_account( account_name n , bool should_freeze ) {
         const auto& a = context.db.get<account_object, by_name>( n );
         context.mutable_db.modify( a, [&]( auto& ma ){
            ma.frozen = should_freeze;
         });
      }

      /// TODO: add inline/deferred with support for arbitrary permissions rather than code/current auth
};

class checktime_api : public context_aware_api {
public:
   using context_aware_api::context_aware_api;

   void checktime(uint32_t instruction_count) {
      context.checktime(instruction_count);
   }
};

class producer_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      int get_active_producers(array_ptr<chain::account_name> producers, size_t datalen) {
         auto active_producers = context.get_active_producers();
         size_t len = active_producers.size();
         size_t cpy_len = std::min(datalen, len);
         memcpy(producers, active_producers.data(), cpy_len * sizeof(chain::account_name) );
         return len;
      }
};

class crypto_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      /**
       * This method can be optimized out during replay as it has
       * no possible side effects other than "passing". 
       */
      void assert_recover_key( fc::sha256& digest, 
                        array_ptr<char> sig, size_t siglen,
                        array_ptr<char> pub, size_t publen ) {
         fc::crypto::signature s;
         fc::crypto::public_key p;
         datastream<const char*> ds( sig, siglen );
         datastream<const char*> pubds( pub, publen );

         fc::raw::unpack(ds, s);
         fc::raw::unpack(pubds, p);

         auto check = fc::crypto::public_key( s, digest, false );
         FC_ASSERT( check == p, "Error expected key different than recovered key" );
      }

      int recover_key( fc::sha256& digest, 
                        array_ptr<char> sig, size_t siglen,
                        array_ptr<char> pub, size_t publen ) {
         fc::crypto::signature s;
         datastream<const char*> ds( sig, siglen );
         datastream<char*> pubds( pub, publen );

         fc::raw::unpack(ds, s);
         fc::raw::pack( pubds, fc::crypto::public_key( s, digest, false ) );
         return pubds.tellp();
      }

      void assert_sha256(array_ptr<char> data, size_t datalen, const fc::sha256& hash_val) {
         auto result = fc::sha256::hash( data, datalen );
         FC_ASSERT( result == hash_val, "hash miss match" );
      }

      void assert_sha1(array_ptr<char> data, size_t datalen, const fc::sha1& hash_val) {
         auto result = fc::sha1::hash( data, datalen );
         FC_ASSERT( result == hash_val, "hash miss match" );
      }

      void assert_sha512(array_ptr<char> data, size_t datalen, const fc::sha512& hash_val) {
         auto result = fc::sha512::hash( data, datalen );
         FC_ASSERT( result == hash_val, "hash miss match" );
      }

      void assert_ripemd160(array_ptr<char> data, size_t datalen, const fc::ripemd160& hash_val) {
         auto result = fc::ripemd160::hash( data, datalen );
         FC_ASSERT( result == hash_val, "hash miss match" );
      }


      void sha1(array_ptr<char> data, size_t datalen, fc::sha1& hash_val) {
         hash_val = fc::sha1::hash( data, datalen );
      }

      void sha256(array_ptr<char> data, size_t datalen, fc::sha256& hash_val) {
         hash_val = fc::sha256::hash( data, datalen );
      }

      void sha512(array_ptr<char> data, size_t datalen, fc::sha512& hash_val) {
         hash_val = fc::sha512::hash( data, datalen );
      }

      void ripemd160(array_ptr<char> data, size_t datalen, fc::ripemd160& hash_val) {
         hash_val = fc::ripemd160::hash( data, datalen );
      }
};

class string_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      void assert_is_utf8(array_ptr<const char> str, size_t datalen, null_terminated_ptr msg) {
         const bool test = fc::is_utf8(std::string( str, datalen ));

         FC_ASSERT( test, "assertion failed: ${s}", ("s",msg.value) );
      }
};

class system_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      void abort() {
         edump(("abort() called"));
         FC_ASSERT( false, "abort() called");
      }

      void eosio_assert(bool condition, null_terminated_ptr str) {
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

      name current_receiver() {
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

class database_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      int db_store_i64( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, array_ptr<const char> buffer, size_t buffer_size ) {
         return context.db_store_i64( scope, table, payer, id, buffer, buffer_size );
      }
      void db_update_i64( int itr, uint64_t payer, array_ptr<const char> buffer, size_t buffer_size ) {
         context.db_update_i64( itr, payer, buffer, buffer_size );
      }
      void db_remove_i64( int itr ) {
         context.db_remove_i64( itr );
      }
      int db_get_i64( int itr, array_ptr<char> buffer, size_t buffer_size ) {
         return context.db_get_i64( itr, buffer, buffer_size );
      }
      int db_next_i64( int itr, uint64_t& primary ) {
         return context.db_next_i64(itr, primary);
      }
      int db_previous_i64( int itr, uint64_t& primary ) {
         return context.db_previous_i64(itr, primary);
      }
      int db_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) { 
         return context.db_find_i64( code, scope, table, id ); 
      }
      int db_lowerbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) { 
         return context.db_lowerbound_i64( code, scope, table, id ); 
      }
      int db_upperbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) { 
         return context.db_upperbound_i64( code, scope, table, id ); 
      }

      int db_idx64_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const uint64_t& secondary ) {
         return context.idx64.store( scope, table, payer, id, secondary );
      }
      void db_idx64_update( int iterator, uint64_t payer, const uint64_t& secondary ) {
         return context.idx64.update( iterator, payer, secondary );
      }
      void db_idx64_remove( int iterator ) {
         return context.idx64.remove( iterator );
      }
      int db_idx64_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const uint64_t& secondary, uint64_t& primary ) {
         return context.idx64.find_secondary(code, scope, table, secondary, primary);
      }
      int db_idx64_find_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary, uint64_t primary ) {
         return context.idx64.find_primary(code, scope, table, secondary, primary);
      }
      int db_idx64_lowerbound( uint64_t code, uint64_t scope, uint64_t table,  uint64_t& secondary, uint64_t& primary ) {
         return context.idx64.lowerbound_secondary(code, scope, table, secondary, primary);
      }
      int db_idx64_upperbound( uint64_t code, uint64_t scope, uint64_t table,  uint64_t& secondary, uint64_t& primary ) {
         return context.idx64.upperbound_secondary(code, scope, table, secondary, primary);
      }
      int db_idx64_next( int iterator, uint64_t& primary  ) {
         return context.idx64.next_secondary(iterator, primary);
      }
      int db_idx64_previous( int iterator, uint64_t& primary ) {
         return context.idx64.previous_secondary(iterator, primary);
      }

      int db_idx128_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const uint128_t& secondary ) {
         return context.idx128.store( scope, table, payer, id, secondary );
      }
      void db_idx128_update( int iterator, uint64_t payer, const uint128_t& secondary ) {
         return context.idx128.update( iterator, payer, secondary );
      }
      void db_idx128_remove( int iterator ) {
         return context.idx128.remove( iterator );
      }
      int db_idx128_find_primary( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t primary ) {
         return context.idx128.find_primary( code, scope, table, secondary, primary );
      }
      int db_idx128_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const uint128_t& secondary, uint64_t& primary ) {
         return context.idx128.find_secondary(code, scope, table, secondary, primary);
      }
      int db_idx128_lowerbound( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t& primary ) {
         return context.idx128.lowerbound_secondary(code, scope, table, secondary, primary);
      }
      int db_idx128_upperbound( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t& primary ) {
         return context.idx128.upperbound_secondary(code, scope, table, secondary, primary);
      }
      int db_idx128_next( int iterator, uint64_t& primary ) {
         return context.idx128.next_secondary(iterator, primary);
      }
      int db_idx128_previous( int iterator, uint64_t& primary ) {
         return context.idx128.previous_secondary(iterator, primary);
      }

   /*
      int db_idx128_next( int iterator, uint64_t& primary ) {
      }
      int db_idx128_prev( int iterator, uint64_t& primary ) {
      }
      int db_idx128_find_secondary( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t& primary ) {
      }
      int db_idx128_lowerbound( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t& primary ) {
      }
      int db_idx128_upperbound( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t& primary ) {
      }
      */
};



template<typename ObjectType>
class db_api : public context_aware_api {
   using KeyType = typename ObjectType::key_type;
   static constexpr int KeyCount = ObjectType::number_of_keys;
   using KeyArrayType = KeyType[KeyCount];
   using ContextMethodType = int(apply_context::*)(const table_id_object&, const account_name&, const KeyType*, const char*, size_t);

   private:
      int call(ContextMethodType method, const scope_name& scope, const name& table, account_name bta, array_ptr<const char> data, size_t data_len) {
         const auto& t_id = context.find_or_create_table(context.receiver, scope, table);
         FC_ASSERT(data_len >= KeyCount * sizeof(KeyType), "Data is not long enough to contain keys");
         const KeyType* keys = reinterpret_cast<const KeyType *>((const char *)data);

         const char* record_data =  ((const char*)data) + sizeof(KeyArrayType);
         size_t record_len = data_len - sizeof(KeyArrayType);
         return (context.*(method))(t_id, bta, keys, record_data, record_len) + sizeof(KeyArrayType);
      }

   public:
      using context_aware_api::context_aware_api;
      typedef KeyArrayType& KeyParamType;

      int store(const scope_name& scope, const name& table, const account_name& bta, array_ptr<const char> data, size_t data_len) {
         auto res = call(&apply_context::store_record<ObjectType>, scope, table, bta, data, data_len);
         //ilog("STORE [${scope},${code},${table}] => ${res} :: ${HEX}", ("scope",scope)("code",context.receiver)("table",table)("res",res)("HEX", fc::to_hex(data, data_len)));
         return res;
      }

      int update(const scope_name& scope, const name& table, const account_name& bta, array_ptr<const char> data, size_t data_len) {
         return call(&apply_context::update_record<ObjectType>, scope, table, bta, data, data_len);
      }
      
      int remove(const scope_name& scope, const name& table, const KeyArrayType &keys) {
         const auto& t_id = context.find_or_create_table(context.receiver, scope, table);
         return context.remove_record<ObjectType>(t_id, keys);
      }
};

template<>
class db_api<keystr_value_object> : public context_aware_api {
   using KeyType = std::string;
   static constexpr int KeyCount = 1;
   using KeyArrayType = KeyType[KeyCount];
   using ContextMethodType = int(apply_context::*)(const table_id_object&, const KeyType*, const char*, size_t);

/* TODO something weird is going on here, will maybe fix before DB changes or this might get 
 * totally changed anyway
   private:
      int call(ContextMethodType method, const scope_name& scope, const name& table, account_name bta,
            null_terminated_ptr key, size_t key_len, array_ptr<const char> data, size_t data_len) {
         const auto& t_id = context.find_or_create_table(context.receiver, scope, table);
         const KeyType keys((const char*)key.value, key_len); 

         const char* record_data =  ((const char*)data); 
         size_t record_len = data_len; 
         return (context.*(method))(t_id, bta, &keys, record_data, record_len);
      }
*/
   public:
      using context_aware_api::context_aware_api;

      int store_str(const scope_name& scope, const name& table, const account_name& bta,
            null_terminated_ptr key, uint32_t key_len, array_ptr<const char> data, size_t data_len) {
         const auto& t_id = context.find_or_create_table(context.receiver, scope, table);
         const KeyType keys(key.value, key_len); 
         const char* record_data =  ((const char*)data); 
         size_t record_len = data_len; 
         return context.store_record<keystr_value_object>(t_id, bta, &keys, record_data, record_len);
         //return call(&apply_context::store_record<keystr_value_object>, scope, table, bta, key, key_len, data, data_len);
      }

      int update_str(const scope_name& scope,  const name& table, const account_name& bta, 
            null_terminated_ptr key, uint32_t key_len, array_ptr<const char> data, size_t data_len) {
         const auto& t_id = context.find_or_create_table(context.receiver, scope, table);
         const KeyType keys((const char*)key, key_len); 
         const char* record_data =  ((const char*)data); 
         size_t record_len = data_len; 
         return context.update_record<keystr_value_object>(t_id, bta, &keys, record_data, record_len);
         //return call(&apply_context::update_record<keystr_value_object>, scope, table, bta, key, key_len, data, data_len);
      }
      
      int remove_str(const scope_name& scope, const name& table, const array_ptr<const char> &key, uint32_t key_len) {
         const auto& t_id = context.find_or_create_table(scope, context.receiver, table);
         const KeyArrayType k = {std::string(key, key_len)};
         return context.remove_record<keystr_value_object>(t_id, k);
      }
};

template<typename IndexType, typename Scope>
class db_index_api : public context_aware_api {
   using KeyType = typename IndexType::value_type::key_type;
   static constexpr int KeyCount = IndexType::value_type::number_of_keys;
   using KeyArrayType = KeyType[KeyCount];
   using ContextMethodType = int(apply_context::*)(const table_id_object&, KeyType*, char*, size_t);


   int call(ContextMethodType method, const account_name& code, const scope_name& scope, const name& table, array_ptr<char> data, size_t data_len) {
      auto maybe_t_id = context.find_table(code, scope, table);
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
         return res;
      }

      int front(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         auto res = call(&apply_context::front_record<IndexType, Scope>, scope, code, table, data, data_len);
         return res;
      }

      int back(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         auto res = call(&apply_context::back_record<IndexType, Scope>, scope, code, table, data, data_len);
         return res;
      }

      int next(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         auto res = call(&apply_context::next_record<IndexType, Scope>, scope, code, table, data, data_len);
         return res;
      }

      int previous(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         auto res = call(&apply_context::previous_record<IndexType, Scope>, scope, code, table, data, data_len);
         return res;
      }

      int lower_bound(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         auto res = call(&apply_context::lower_bound_record<IndexType, Scope>, scope, code, table, data, data_len);
         return res;
      }

      int upper_bound(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> data, size_t data_len) {
         auto res = call(&apply_context::upper_bound_record<IndexType, Scope>, scope, code, table, data, data_len);
         return res;
      }

};

template<>
class db_index_api<keystr_value_index, by_scope_primary> : public context_aware_api {
   using KeyType = std::string;
   static constexpr int KeyCount = 1;
   using KeyArrayType = KeyType[KeyCount];
   using ContextMethodType = int(apply_context::*)(const table_id_object&, KeyType*, char*, size_t);


   int call(ContextMethodType method, const scope_name& scope, const account_name& code, const name& table, 
         array_ptr<char> &key, uint32_t key_len, array_ptr<char> data, size_t data_len) {
      auto maybe_t_id = context.find_table(scope, context.receiver, table);
      if (maybe_t_id == nullptr) {
         return 0;
      }

      const auto& t_id = *maybe_t_id;
      //FC_ASSERT(data_len >= KeyCount * sizeof(KeyType), "Data is not long enough to contain keys");
      KeyType keys((const char*)key, key_len); // = reinterpret_cast<KeyType *>((char *)data);

      char* record_data =  ((char*)data); // + sizeof(KeyArrayType);
      size_t record_len = data_len; // - sizeof(KeyArrayType);

      return (context.*(method))(t_id, &keys, record_data, record_len); // + sizeof(KeyArrayType);
   }

   public:
      using context_aware_api::context_aware_api;

      int load_str(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> key, size_t key_len, array_ptr<char> data, size_t data_len) {
         auto res = call(&apply_context::load_record<keystr_value_index, by_scope_primary>, scope, code, table, key, key_len, data, data_len);
         return res;
      }

      int front_str(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> key, size_t key_len, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::front_record<keystr_value_index, by_scope_primary>, scope, code, table, key, key_len, data, data_len);
      }

      int back_str(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> key, size_t key_len, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::back_record<keystr_value_index, by_scope_primary>, scope, code, table, key, key_len, data, data_len);
      }

      int next_str(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> key, size_t key_len, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::next_record<keystr_value_index, by_scope_primary>, scope, code, table, key, key_len, data, data_len);
      }

      int previous_str(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> key, size_t key_len, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::previous_record<keystr_value_index, by_scope_primary>, scope, code, table, key, key_len, data, data_len);
      }

      int lower_bound_str(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> key, size_t key_len, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::lower_bound_record<keystr_value_index, by_scope_primary>, scope, code, table, key, key_len, data, data_len);
      }

      int upper_bound_str(const scope_name& scope, const account_name& code, const name& table, array_ptr<char> key, size_t key_len, array_ptr<char> data, size_t data_len) {
         return call(&apply_context::upper_bound_record<keystr_value_index, by_scope_primary>, scope, code, table, key, key_len, data, data_len);
      }
};

class memory_api : public context_aware_api {
   public:
      memory_api( wasm_interface& wasm )
      :context_aware_api(wasm,true){}
     
      char* memcpy( array_ptr<char> dest, array_ptr<const char> src, size_t length) {
         return (char *)::memcpy(dest, src, length);
      }

      char* memmove( array_ptr<char> dest, array_ptr<const char> src, size_t length) {
         return (char *)::memmove(dest, src, length);
      }

      int memcmp( array_ptr<const char> dest, array_ptr<const char> src, size_t length) {
         return ::memcmp(dest, src, length);
      }

      char* memset( array_ptr<char> dest, int value, size_t length ) {
         return (char *)::memset( dest, value, length );
      }

      int sbrk(int num_bytes) {
         // sbrk should only allow for memory to grow
         if (num_bytes < 0)
            throw eosio::chain::page_memory_error();
         // TODO: omitted checktime function from previous version of sbrk, may need to be put back in at some point
         constexpr uint32_t NBPPL2  = IR::numBytesPerPageLog2;
         constexpr uint32_t MAX_MEM = wasm_constraints::maximum_linear_memory;

         MemoryInstance*  default_mem    = Runtime::getDefaultMemory(code.instance);
         if(!default_mem)
            return -1;

         const uint32_t         num_pages      = Runtime::getMemoryNumPages(default_mem);
         const uint32_t         min_bytes      = (num_pages << NBPPL2) > UINT32_MAX ? UINT32_MAX : num_pages << NBPPL2;
         const uint32_t         prev_num_bytes = sbrk_bytes; //_num_bytes;
         
         // round the absolute value of num_bytes to an alignment boundary
         num_bytes = (num_bytes + 7) & ~7;

         if ((num_bytes > 0) && (prev_num_bytes > (MAX_MEM - num_bytes)))  // test if allocating too much memory (overflowed)
            return -1;
         else if ((num_bytes < 0) && (prev_num_bytes < (min_bytes - num_bytes))) // test for underflow
            throw eosio::chain::page_memory_error(); 

         const uint32_t num_desired_pages = (sbrk_bytes + num_bytes + IR::numBytesPerPage - 1) >> NBPPL2;

         // grow or shrink the memory to the desired number of pages
         if (num_desired_pages > num_pages) {
            if(Runtime::growMemory(default_mem, num_desired_pages - num_pages) == -1)
               return -1;
         }
         else if (num_desired_pages < num_pages) {
            if(Runtime::shrinkMemory(default_mem, num_pages - num_desired_pages) == -1)
               return -1;
         }

         sbrk_bytes += num_bytes;
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


class context_free_transaction_api : public context_aware_api {
   public:
      context_free_transaction_api( wasm_interface& wasm )
      :context_aware_api(wasm,true){}

      // For native contract compilation only
      context_free_transaction_api( apply_context& ac, uint32_t& sb, wasm_cache::entry& c )
      :context_aware_api(ac, sb, c) {}

      int read_transaction( array_ptr<char> data, size_t data_len ) {
         bytes trx = context.get_packed_transaction();
         if (data_len >= trx.size()) {
            memcpy(data, trx.data(), trx.size());
         }
         return trx.size();
      }

      int transaction_size() {
         return context.get_packed_transaction().size();
      }

      int expiration() {
        return context.trx_meta.trx().expiration.sec_since_epoch();
      }

      int tapos_block_num() {
        return context.trx_meta.trx().ref_block_num;
      }
      int tapos_block_prefix() {
        return context.trx_meta.trx().ref_block_prefix;
      }

      int get_action( uint32_t type, uint32_t index, array_ptr<char> buffer, size_t buffer_size )const {
         return context.get_action( type, index, buffer, buffer_size );
      }

};

class compiler_builtins : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;
      void __ashlti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
         fc::uint128_t i(high, low);
         i <<= shift;
         ret = (unsigned __int128)i;
      }

      void __ashrti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
         // retain the signedness
         ret = high;
         ret <<= 64;
         ret |= low;
         ret >>= shift;
      }

      void __lshlti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
         fc::uint128_t i(high, low);
         i <<= shift;
         ret = (unsigned __int128)i;
      }

      void __lshrti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
         fc::uint128_t i(high, low);
         i >>= shift;
         ret = (unsigned __int128)i;
      }
      
      void __divti3(__int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         __int128 lhs = ha;
         __int128 rhs = hb;
         
         lhs <<= 64;
         lhs |=  la;

         rhs <<= 64;
         rhs |=  lb;

         FC_ASSERT(rhs != 0, "divide by zero");    

         lhs /= rhs; 

         ret = lhs;
      } 

      void __udivti3(unsigned __int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         unsigned __int128 lhs = ha;
         unsigned __int128 rhs = hb;
         
         lhs <<= 64;
         lhs |=  la;

         rhs <<= 64;
         rhs |=  lb;

         FC_ASSERT(rhs != 0, "divide by zero");    

         lhs /= rhs; 
         ret = lhs;
      }

      void __multi3(__int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         __int128 lhs = ha;
         __int128 rhs = hb;

         lhs <<= 64;
         lhs |=  la;

         rhs <<= 64;
         rhs |=  lb;

         lhs *= rhs; 
         ret = lhs;
      } 

      void __modti3(__int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         __int128 lhs = ha;
         __int128 rhs = hb;

         lhs <<= 64;
         lhs |=  la;
   
         rhs <<= 64;
         rhs |=  lb;
         
         FC_ASSERT(rhs != 0, "divide by zero");

         lhs %= rhs;
         ret = lhs;
      }

      void __umodti3(unsigned __int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         unsigned __int128 lhs = ha;
         unsigned __int128 rhs = hb;

         lhs <<= 64;
         lhs |=  la;
   
         rhs <<= 64;
         rhs |=  lb;
         
         FC_ASSERT(rhs != 0, "divide by zero");

         lhs %= rhs;
         ret = lhs;
      }

      static constexpr uint32_t SHIFT_WIDTH = (sizeof(uint64_t)*8)-1;
};

class math_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;
      

      void diveq_i128(unsigned __int128* self, const unsigned __int128* other) {
         fc::uint128_t s(*self);
         const fc::uint128_t o(*other);
         FC_ASSERT( o != 0, "divide by zero" );
         
         s = s/o;
         *self = (unsigned __int128)s;
      }

      void multeq_i128(unsigned __int128* self, const unsigned __int128* other) {
         fc::uint128_t s(*self);
         const fc::uint128_t o(*other);
         s *= o;
         *self = (unsigned __int128)s;
      }

      uint64_t double_add(uint64_t a, uint64_t b) {
         using DOUBLE = boost::multiprecision::cpp_bin_float_50;
         DOUBLE c = DOUBLE(*reinterpret_cast<double *>(&a))
                  + DOUBLE(*reinterpret_cast<double *>(&b));
         double res = c.convert_to<double>();
         return *reinterpret_cast<uint64_t *>(&res);
      }

      uint64_t double_mult(uint64_t a, uint64_t b) {
         using DOUBLE = boost::multiprecision::cpp_bin_float_50;
         DOUBLE c = DOUBLE(*reinterpret_cast<double *>(&a))
                  * DOUBLE(*reinterpret_cast<double *>(&b));
         double res = c.convert_to<double>();
         return *reinterpret_cast<uint64_t *>(&res);
      }

      uint64_t double_div(uint64_t a, uint64_t b) {
         using DOUBLE = boost::multiprecision::cpp_bin_float_50;
         DOUBLE divisor = DOUBLE(*reinterpret_cast<double *>(&b));
         FC_ASSERT(divisor != 0, "divide by zero");
         DOUBLE c = DOUBLE(*reinterpret_cast<double *>(&a)) / divisor;
         double res = c.convert_to<double>();
         return *reinterpret_cast<uint64_t *>(&res);
      }

      uint32_t double_eq(uint64_t a, uint64_t b) {
         using DOUBLE = boost::multiprecision::cpp_bin_float_50;
         return DOUBLE(*reinterpret_cast<double *>(&a)) == DOUBLE(*reinterpret_cast<double *>(&b));
      }

      uint32_t double_lt(uint64_t a, uint64_t b) {
         using DOUBLE = boost::multiprecision::cpp_bin_float_50;
         return DOUBLE(*reinterpret_cast<double *>(&a)) < DOUBLE(*reinterpret_cast<double *>(&b));
      }

      uint32_t double_gt(uint64_t a, uint64_t b) {
         using DOUBLE = boost::multiprecision::cpp_bin_float_50;
         return DOUBLE(*reinterpret_cast<double *>(&a)) > DOUBLE(*reinterpret_cast<double *>(&b));
      }

      uint64_t double_to_i64(uint64_t n) {
         using DOUBLE = boost::multiprecision::cpp_bin_float_50;
         return DOUBLE(*reinterpret_cast<double *>(&n)).convert_to<int64_t>();
      }

      uint64_t i64_to_double(int64_t n) {
         using DOUBLE = boost::multiprecision::cpp_bin_float_50;
         double res = DOUBLE(n).convert_to<double>();
         return *reinterpret_cast<uint64_t *>(&res);
      }
};

REGISTER_INTRINSICS(math_api,
   (diveq_i128,    void(int, int)            )
   (multeq_i128,   void(int, int)            )
   (double_add,    int64_t(int64_t, int64_t) )
   (double_mult,   int64_t(int64_t, int64_t) )
   (double_div,    int64_t(int64_t, int64_t) )
   (double_eq,     int32_t(int64_t, int64_t) )
   (double_lt,     int32_t(int64_t, int64_t) )
   (double_gt,     int32_t(int64_t, int64_t) )
   (double_to_i64, int64_t(int64_t)          )
   (i64_to_double, int64_t(int64_t)          )
);

REGISTER_INTRINSICS(compiler_builtins,
   (__ashlti3,     void(int, int64_t, int64_t, int)               )
   (__ashrti3,     void(int, int64_t, int64_t, int)               )
   (__lshlti3,     void(int, int64_t, int64_t, int)               )
   (__lshrti3,     void(int, int64_t, int64_t, int)               )
   (__divti3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__udivti3,     void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__modti3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__umodti3,     void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__multi3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
);

REGISTER_INTRINSICS(privileged_api,
   (activate_feature,          void(int64_t)                                 )
   (is_feature_active,         int(int64_t)                                  )
   (set_resource_limits,       void(int64_t,int64_t,int64_t,int64_t,int64_t) )
   (set_active_producers,      void(int,int)                                 )
   (is_privileged,             int(int64_t)                                  )
   (set_privileged,            void(int64_t, int)                            )
   (freeze_account,            void(int64_t, int)                            )
   (is_frozen,                 int(int64_t)                                  )
);

REGISTER_INTRINSICS(checktime_api,
   (checktime,      void(int))
);

REGISTER_INTRINSICS(producer_api,
   (get_active_producers,      int(int, int) )
);

REGISTER_INTRINSICS( database_api,
   (db_store_i64,        int(int64_t,int64_t,int64_t,int64_t,int,int))
   (db_update_i64,       void(int,int64_t,int,int))
   (db_remove_i64,       void(int))
   (db_get_i64,          int(int, int, int))
   (db_next_i64,         int(int, int))
   (db_previous_i64,     int(int, int))
   (db_find_i64,         int(int64_t,int64_t,int64_t,int64_t))
   (db_lowerbound_i64,   int(int64_t,int64_t,int64_t,int64_t))
   (db_upperbound_i64,   int(int64_t,int64_t,int64_t,int64_t))
                             
   (db_idx64_store,          int(int64_t,int64_t,int64_t,int64_t,int))
   (db_idx64_remove,         void(int))
   (db_idx64_update,         void(int,int64_t,int))
   (db_idx64_find_primary,   int(int64_t,int64_t,int64_t,int,int64_t))
   (db_idx64_find_secondary, int(int64_t,int64_t,int64_t,int,int))
   (db_idx64_lowerbound,     int(int64_t,int64_t,int64_t,int,int))
   (db_idx64_upperbound,     int(int64_t,int64_t,int64_t,int,int))
   (db_idx64_next,           int(int, int))
   (db_idx64_previous,       int(int, int))

   (db_idx128_store,          int(int64_t,int64_t,int64_t,int64_t,int))
   (db_idx128_remove,         void(int))
   (db_idx128_update,         void(int,int64_t,int))
   (db_idx128_find_primary,   int(int64_t,int64_t,int64_t,int,int64_t))
   (db_idx128_find_secondary, int(int64_t,int64_t,int64_t,int,int))
   (db_idx128_lowerbound,     int(int64_t,int64_t,int64_t,int,int))
   (db_idx128_upperbound,     int(int64_t,int64_t,int64_t,int,int))
   (db_idx128_next,           int(int, int))
   (db_idx128_previous,       int(int, int))
);

REGISTER_INTRINSICS(crypto_api,
   (assert_recover_key,     void(int, int, int, int, int) )
   (recover_key,            int(int, int, int, int, int)  )
   (assert_sha256,          void(int, int, int)           )
   (assert_sha1,            void(int, int, int)           )
   (assert_sha512,          void(int, int, int)           )
   (assert_ripemd160,       void(int, int, int)           )
   (sha1,                   void(int, int, int)           )
   (sha256,                 void(int, int, int)           )
   (sha512,                 void(int, int, int)           )
   (ripemd160,              void(int, int, int)           )
);

REGISTER_INTRINSICS(string_api,
   (assert_is_utf8,  void(int, int, int) )
);

REGISTER_INTRINSICS(system_api,
   (abort,        void())
   (eosio_assert, void(int, int))
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
   (require_write_lock,    void(int64_t)          )
   (require_read_lock,     void(int64_t, int64_t) )
   (require_recipient,     void(int64_t)          )
   (require_authorization, void(int64_t), "require_auth", void(apply_context::*)(const account_name&)const)
   (is_account,            int(int64_t)           )
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

REGISTER_INTRINSICS(context_free_transaction_api,
   (read_transaction,       int(int, int)            )
   (transaction_size,       int()                    )
   (expiration,             int()                    )
   (tapos_block_prefix,     int()                    )
   (tapos_block_num,        int()                    )
   (get_action,            int (int, int, int, int)  )
);

REGISTER_INTRINSICS(transaction_api,
   (send_inline,           void(int, int)            )
   (send_deferred,         void(int, int, int, int)  )
);

REGISTER_INTRINSICS(context_free_api,
   (get_context_free_data, int(int, int, int) )
)

REGISTER_INTRINSICS(memory_api,
   (memcpy,                 int(int, int, int)  )
   (memmove,                int(int, int, int)  )
   (memcmp,                 int(int, int, int)  )
   (memset,                 int(int, int, int)  )
   (sbrk,                   int(int)            )
);

#define DB_METHOD_SEQ(SUFFIX) \
   (store,        int32_t(int64_t, int64_t, int64_t, int, int),   "store_"#SUFFIX ) \
   (update,       int32_t(int64_t, int64_t, int64_t, int, int),   "update_"#SUFFIX ) \
   (remove,       int32_t(int64_t, int64_t, int),                 "remove_"#SUFFIX )

#define DB_INDEX_METHOD_SEQ(SUFFIX)\
   (load,         int32_t(int64_t, int64_t, int64_t, int, int),   "load_"#SUFFIX )\
   (front,        int32_t(int64_t, int64_t, int64_t, int, int),   "front_"#SUFFIX )\
   (back,         int32_t(int64_t, int64_t, int64_t, int, int),   "back_"#SUFFIX )\
   (previous,     int32_t(int64_t, int64_t, int64_t, int, int),   "previous_"#SUFFIX )\
   (lower_bound,  int32_t(int64_t, int64_t, int64_t, int, int),   "lower_bound_"#SUFFIX )\
   (upper_bound,  int32_t(int64_t, int64_t, int64_t, int, int),   "upper_bound_"#SUFFIX )\

using db_api_key_value_object                                 = db_api<key_value_object>;
using db_api_keystr_value_object                              = db_api<keystr_value_object>;
using db_api_key128x128_value_object                          = db_api<key128x128_value_object>;
using db_api_key64x64_value_object                            = db_api<key64x64_value_object>;
using db_api_key64x64x64_value_object                         = db_api<key64x64x64_value_object>;
using db_index_api_key_value_index_by_scope_primary           = db_index_api<key_value_index,by_scope_primary>;
using db_index_api_keystr_value_index_by_scope_primary        = db_index_api<keystr_value_index,by_scope_primary>;
using db_index_api_key128x128_value_index_by_scope_primary    = db_index_api<key128x128_value_index,by_scope_primary>;
using db_index_api_key128x128_value_index_by_scope_secondary  = db_index_api<key128x128_value_index,by_scope_secondary>;
using db_index_api_key64x64_value_index_by_scope_primary      = db_index_api<key64x64_value_index,by_scope_primary>;
using db_index_api_key64x64_value_index_by_scope_secondary    = db_index_api<key64x64_value_index,by_scope_secondary>;
using db_index_api_key64x64x64_value_index_by_scope_primary   = db_index_api<key64x64x64_value_index,by_scope_primary>;
using db_index_api_key64x64x64_value_index_by_scope_secondary = db_index_api<key64x64x64_value_index,by_scope_secondary>;
using db_index_api_key64x64x64_value_index_by_scope_tertiary  = db_index_api<key64x64x64_value_index,by_scope_tertiary>;

REGISTER_INTRINSICS(db_api_key_value_object,         DB_METHOD_SEQ(i64));
REGISTER_INTRINSICS(db_api_key128x128_value_object,  DB_METHOD_SEQ(i128i128));
REGISTER_INTRINSICS(db_api_key64x64_value_object,    DB_METHOD_SEQ(i64i64));
REGISTER_INTRINSICS(db_api_key64x64x64_value_object, DB_METHOD_SEQ(i64i64i64));
REGISTER_INTRINSICS(db_api_keystr_value_object,
   (store_str,                int32_t(int64_t, int64_t, int64_t, int, int, int, int)  )
   (update_str,               int32_t(int64_t, int64_t, int64_t, int, int, int, int)  )
   (remove_str,               int32_t(int64_t, int64_t, int, int)  ));

REGISTER_INTRINSICS(db_index_api_key_value_index_by_scope_primary,           DB_INDEX_METHOD_SEQ(i64));
REGISTER_INTRINSICS(db_index_api_keystr_value_index_by_scope_primary,
   (load_str,            int32_t(int64_t, int64_t, int64_t, int, int, int, int)  )
   (front_str,           int32_t(int64_t, int64_t, int64_t, int, int, int, int)  )
   (back_str,            int32_t(int64_t, int64_t, int64_t, int, int, int, int)  )
   (next_str,            int32_t(int64_t, int64_t, int64_t, int, int, int, int)  )
   (previous_str,        int32_t(int64_t, int64_t, int64_t, int, int, int, int)  )
   (lower_bound_str,     int32_t(int64_t, int64_t, int64_t, int, int, int, int)  )
   (upper_bound_str,     int32_t(int64_t, int64_t, int64_t, int, int, int, int)  ));

REGISTER_INTRINSICS(db_index_api_key128x128_value_index_by_scope_primary,    DB_INDEX_METHOD_SEQ(primary_i128i128));
REGISTER_INTRINSICS(db_index_api_key128x128_value_index_by_scope_secondary,  DB_INDEX_METHOD_SEQ(secondary_i128i128));
REGISTER_INTRINSICS(db_index_api_key64x64_value_index_by_scope_primary,      DB_INDEX_METHOD_SEQ(primary_i64i64));
REGISTER_INTRINSICS(db_index_api_key64x64_value_index_by_scope_secondary,    DB_INDEX_METHOD_SEQ(secondary_i64i64));
REGISTER_INTRINSICS(db_index_api_key64x64x64_value_index_by_scope_primary,   DB_INDEX_METHOD_SEQ(primary_i64i64i64));
REGISTER_INTRINSICS(db_index_api_key64x64x64_value_index_by_scope_secondary, DB_INDEX_METHOD_SEQ(secondary_i64i64i64));
REGISTER_INTRINSICS(db_index_api_key64x64x64_value_index_by_scope_tertiary,  DB_INDEX_METHOD_SEQ(tertiary_i64i64i64));

apply_context* native_context = 0;

extern "C" {

#define _REGISTER_NATIVE_INTRINSIC_EXPLICIT(API, INTRINSIC_NAME, RETURN, PARAM_LIST, ARGS, FUNC_NAME) \
RETURN INTRINSIC_NAME PARAM_LIST {      \
   uint32_t sb = 0;                     \
   wasm_cache::entry c(0,0);            \
   API api(*native_context, sb, c);     \
   return api.FUNC_NAME ARGS;           \
}

#define _REGISTER_NATIVE_INTRINSIC5(CLS, INTRINSIC_NAME, RETURN, PARAM_LIST, ARGS, FUNC_NAME)\
   _REGISTER_NATIVE_INTRINSIC_EXPLICIT(CLS, INTRINSIC_NAME, RETURN, PARAM_LIST, ARGS, FUNC_NAME )

#define _REGISTER_NATIVE_INTRINSIC4(CLS, INTRINSIC_NAME, RETURN, PARAM_LIST, ARGS)\
   _REGISTER_NATIVE_INTRINSIC_EXPLICIT(CLS, INTRINSIC_NAME, RETURN, PARAM_LIST, ARGS, INTRINSIC_NAME )

#define _REGISTER_NATIVE_INTRINSIC3(CLS, INTRINSIC_NAME, RETURN, NAME)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":" BOOST_PP_STRINGIZE(INTRINSIC_NAME) " without a full set of parameters");

#define _REGISTER_NATIVE_INTRINSIC2(CLS, INTRINSIC_NAME, RETURN)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":" BOOST_PP_STRINGIZE(INTRINSIC_NAME) " without a full set of parameters");

#define _REGISTER_NATIVE_INTRINSIC1(CLS, INTRINSIC_NAME)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":" BOOST_PP_STRINGIZE(INTRINSIC_NAME) " without a full set of parameters");

#define _REGISTER_NATIVE_INTRINSIC0(CLS, INTRINSIC_NAME)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":<unknown> without a method name, return value and signature");

#define _REGISTER_NATIVE_INTRINSIC(R, CLS, INFO)\
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(_REGISTER_NATIVE_INTRINSIC, _UNWRAP_SEQ INFO) _EXPAND_ARGS(CLS, INFO), BOOST_PP_EMPTY())

#define REGISTER_NATIVE_INTRINSICS(CLS, MEMBERS)\
   BOOST_PP_SEQ_FOR_EACH(_REGISTER_NATIVE_INTRINSIC, CLS, _WRAPPED_SEQ(MEMBERS))

REGISTER_NATIVE_INTRINSICS(action_api,
   (action_size, int, (), ())
   (read_action, int, (char* memory, size_t size), (array_ptr<char>(memory), size))
   (current_receiver, uint64_t, (), ())
   (current_sender, uint64_t, (), ())
   (publication_time, uint32_t, (), ().sec_since_epoch())
)

/*
// Note: No version of checktime found in eosiolib
REGISTER_INTRINSICS(checktime_api,
   (checktime,      void(int))
);
*/

REGISTER_NATIVE_INTRINSICS(compiler_builtins,
   (__ashlti3,     void, (void* res, int64_t lo, int64_t hi, uint32_t shift), (*(__int128*)res, lo, hi, shift))
   (__ashrti3,     void, (void* res, int64_t lo, int64_t hi, uint32_t shift), (*(__int128*)res, lo, hi, shift))
   (__lshlti3,     void, (void* res, int64_t lo, int64_t hi, uint32_t shift), (*(__int128*)res, lo, hi, shift))
   (__lshrti3,     void, (void* res, int64_t lo, int64_t hi, uint32_t shift), (*(__int128*)res, lo, hi, shift))
   // Note: Commented out because these appear to conflict with built-in compiler-defined functions. Intrinsics should probably be renamed.
   //(__divti3,      void, (void* res, int64_t la, int64_t ha, int64_t lb, int64_t hb), (*(__int128*)res, la, ha, lb, hb))
   //(__udivti3,     void, (void* res, int64_t la, int64_t ha, int64_t lb, int64_t hb), (*(unsigned __int128*)res, la, ha, lb, hb))
   //(__modti3,      void, (void* res, int64_t la, int64_t ha, int64_t lb, int64_t hb), (*(__int128*)res, la, ha, lb, hb))
   //(__umodti3,     void, (void* res, int64_t la, int64_t ha, int64_t lb, int64_t hb), (*(unsigned __int128*)res, la, ha, lb, hb))
   (__multi3,      void, (void* res, int64_t la, int64_t ha, int64_t lb, int64_t hb), (*(__int128*)res, la, ha, lb, hb))
)

REGISTER_NATIVE_INTRINSICS(console_api,
   (printn, void, (uint64_t val), (val))
   (prints, void, (const char* val), (null_terminated_ptr(val)))
   (printi, void, (uint64_t val), (val))
   (printi128, void, (const uint128_t* val), (*val))
   (printd, void, (uint64_t val), (wasm_double(val)))
   (printhex, void, (const char* data, uint32_t datalen), (array_ptr<const char>(data), datalen))
   (prints_l, void, (const char* cstr, uint32_t len), (array_ptr<const char>(cstr), len))
)

/*
// Note: No version of get_context_free_data found in eosiolib
REGISTER_INTRINSICS(context_free_api,
   (get_context_free_data, int(int, int, int) )
)
*/

REGISTER_NATIVE_INTRINSICS(context_free_transaction_api,
   (read_transaction,       size_t, (char* buffer, size_t size), (array_ptr<char>(buffer), size))
   (transaction_size,       size_t, (), ())
   (expiration,             int, (), ())
   (tapos_block_prefix,     int, (), ())
   (tapos_block_num,        int, (), ())
   // Note: No declaration of get_action found in eosiolib
   (get_action,             int, (uint32_t type, uint32_t index, char* buffer, size_t length), (type, index, array_ptr<char>(buffer), length))
)

REGISTER_NATIVE_INTRINSICS(crypto_api,
   (assert_recover_key,     void, (uint64_t digest, char* sig, size_t siglen, char* pub, size_t publen), \
                          (*(fc::sha256*)digest, array_ptr<char>(sig), siglen, array_ptr<char>(pub), publen))
   (recover_key,            int, (uint64_t digest, char* sig, size_t siglen, char* pub, size_t publen), \
                          (*(fc::sha256*)digest, array_ptr<char>(sig), siglen, array_ptr<char>(pub), publen))
   (assert_sha256,          void, (char* data, uint32_t length, void* hash), (array_ptr<char>(data), length, *(fc::sha256*)hash))
   (assert_sha1,            void, (char* data, uint32_t length, void* hash), (array_ptr<char>(data), length, *(fc::sha1*)hash))
   (assert_sha512,          void, (char* data, uint32_t length, void* hash), (array_ptr<char>(data), length, *(fc::sha512*)hash))
   (assert_ripemd160,       void, (char* data, uint32_t length, void* hash), (array_ptr<char>(data), length, *(fc::ripemd160*)hash))
   (sha1,                   void, (char* data, uint32_t length, void* hash), (array_ptr<char>(data), length, *(fc::sha1*)hash))
   (sha256,                 void, (char* data, uint32_t length, void* hash), (array_ptr<char>(data), length, *(fc::sha256*)hash))
   (sha512,                 void, (char* data, uint32_t length, void* hash), (array_ptr<char>(data), length, *(fc::sha512*)hash))
   (ripemd160,              void, (char* data, uint32_t length, void* hash), (array_ptr<char>(data), length, *(fc::ripemd160*)hash))
)

REGISTER_NATIVE_INTRINSICS(database_api,
   (db_store_i64,        int, (int64_t scope, int64_t table, int64_t payer, int64_t id, const char* data, uint32_t length),
                          (scope, table, payer, id, array_ptr<const char>(data), length))
   (db_update_i64,       void, (int32_t iter, int64_t payer, const char* data, uint32_t length), (iter, payer, array_ptr<const char>(data), length))
   (db_remove_i64,       void, (int32_t iter), (iter))
   (db_get_i64,          int, (int32_t iter, char* data, uint32_t length), (iter, array_ptr<char>(data), length))
   (db_next_i64,         int, (int32_t iter, uint64_t* primary), (iter, *primary))
   (db_previous_i64,     int, (int32_t iter, uint64_t* primary), (iter, *primary))
   (db_find_i64,         int, (int64_t code, int64_t scope, int64_t table, int64_t id), (code, scope, table, id))
   (db_lowerbound_i64,   int, (int64_t code, int64_t scope, int64_t table, int64_t id), (code, scope, table, id))
   (db_upperbound_i64,   int, (int64_t code, int64_t scope, int64_t table, int64_t id), (code, scope, table, id))

   (db_idx64_store,          int, (int64_t scope, int64_t table, int64_t payer, int64_t id, uint64_t* secondary), (scope, table, payer, id, *secondary))
   (db_idx64_remove,         void, (int32_t iter), (iter))
   (db_idx64_update,         void, (int32_t iter, int64_t payer, const uint64_t* secondary), (iter, payer, *secondary))
   (db_idx64_find_primary,   int, (int64_t code, int64_t scope, int64_t table, uint64_t* secondary, uint64_t primary), (code, scope, table, *secondary, primary))
   (db_idx64_find_secondary, int, (int64_t code, int64_t scope, int64_t table, uint64_t* secondary, uint64_t* primary), (code, scope, table, *secondary, *primary))
   (db_idx64_lowerbound,     int, (int64_t code, int64_t scope, int64_t table, uint64_t* secondary, uint64_t* primary), (code, scope, table, *secondary, *primary))
   (db_idx64_upperbound,     int, (int64_t code, int64_t scope, int64_t table, uint64_t* secondary, uint64_t* primary), (code, scope, table, *secondary, *primary))
   (db_idx64_next,           int, (int32_t iter, uint64_t* primary), (iter, *primary))
   (db_idx64_previous,       int, (int32_t iter, uint64_t* primary), (iter, *primary))

   (db_idx128_store,          int, (int64_t scope, int64_t table, int64_t payer, int64_t id, uint128_t* secondary), (scope, table, payer, id, *secondary))
   (db_idx128_remove,         void, (int32_t iter), (iter))
   (db_idx128_update,         void, (int32_t iter, int64_t payer, const uint128_t* secondary), (iter, payer, *secondary))
   (db_idx128_find_primary,   int, (account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t primary), (code, scope, table, *secondary, primary))
   (db_idx128_find_secondary, int, (account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t* primary), (code, scope, table, *secondary, *primary))
   (db_idx128_lowerbound,     int, (account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t* primary), (code, scope, table, *secondary, *primary))
   (db_idx128_upperbound,     int, (account_name code, account_name scope, table_name table, uint128_t* secondary, uint64_t* primary), (code, scope, table, *secondary, *primary))
   (db_idx128_next,           int, (int32_t iter, uint64_t* primary), (iter, *primary))
   (db_idx128_previous,       int, (int32_t iter, uint64_t* primary), (iter, *primary))
)

REGISTER_NATIVE_INTRINSICS(math_api,
   (diveq_i128,    void, (uint64_t val1, uint64_t val2), ((unsigned __int128*) val1, (unsigned __int128*) val2))
   (multeq_i128,   void, (uint64_t val1, uint64_t val2), ((unsigned __int128*) val1, (unsigned __int128*) val2))
   (double_add,    uint64_t, (uint64_t val1, uint64_t val2), (val1, val2))
   (double_mult,   uint64_t, (uint64_t val1, uint64_t val2), (val1, val2))
   (double_div,    uint64_t, (uint64_t val1, uint64_t val2), (val1, val2))
   (double_eq,     uint32_t, (uint64_t val1, uint64_t val2), (val1, val2))
   (double_lt,     uint32_t, (uint64_t val1, uint64_t val2), (val1, val2))
   (double_gt,     uint32_t, (uint64_t val1, uint64_t val2), (val1, val2))
   (double_to_i64, uint64_t, (uint64_t val), (val))
   (i64_to_double, uint64_t, (uint64_t val), (val))
)

/*
// Note: I don't think we need to implement these as we are OK using the native methods
REGISTER_NATIVE_INTRINSICS(memory_api,
   (memcpy,  int, (int, int, int), ())
   (memmove, int, (int, int, int), ())
   (memcmp,  int, (int, int, int), ())
   (memset,  int, (int, int, int), ())
   (sbrk,    int, (int), ())
)
*/

REGISTER_NATIVE_INTRINSICS(privileged_api,
   // Note: None of the following functions were found in eosiolib
   // (activate_feature,     void, (int64_t), ())
   // (is_feature_active,    int,  (int64_t), ())
   // (set_privileged,       void, (int64_t, int), ())
   // (freeze_account,       void, (int64_t, int), ())
   // (is_frozen,            int,  (int64_t), ())
   (set_resource_limits,  void, (uint64_t account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight, int64_t ignored), (account, ram_bytes, net_weight, cpu_weight, ignored))
   (set_active_producers, void, (char* producer_data, size_t producer_data_size), (array_ptr<char>(producer_data), producer_data_size))
   (is_privileged,        bool,  (uint64_t account), (account))
)

REGISTER_NATIVE_INTRINSICS(producer_api,
   (get_active_producers, uint32_t, (chain::account_name* producers, uint32_t datalen), (array_ptr<chain::account_name>(producers), datalen))
)

/*
// Note: Not found in eosiolib
REGISTER_INTRINSICS(string_api,
   (assert_is_utf8,  void(int, int, int) )
);
*/

REGISTER_NATIVE_INTRINSICS(system_api,
   (eosio_assert, void, (bool condition, null_terminated_ptr str), (condition, str))
   // Note: Abort is problematic, because if I define a global abort() function it conflicts with the system's abort()
   // (abort,        void, (), ())
   (now,          uint32_t, (), ().sec_since_epoch())
)

REGISTER_NATIVE_INTRINSICS(transaction_api,
   (send_inline,   void, (char* data, size_t data_len), (array_ptr<char>(data), data_len))
   (send_deferred, void, (uint32_t sender_id, fc::time_point_sec delay_until, char* data, size_t data_len), (sender_id, delay_until, array_ptr<char>(data), data_len))
)

#define REGISTER_NATIVE_APPLY_CONTEXT_INTRINSIC(INTRINSIC_NAME, RETURN, PARAM_LIST, ARGS, FUNC_NAME) \
RETURN INTRINSIC_NAME PARAM_LIST {         \
   return native_context->FUNC_NAME ARGS;  \
}

REGISTER_NATIVE_APPLY_CONTEXT_INTRINSIC(require_auth, void, (int64_t val), (val), require_authorization)
REGISTER_NATIVE_APPLY_CONTEXT_INTRINSIC(require_recipient, void, (int64_t val), (val), require_recipient)
REGISTER_NATIVE_APPLY_CONTEXT_INTRINSIC(require_write_lock, void, (int64_t val), (val), require_write_lock)
REGISTER_NATIVE_APPLY_CONTEXT_INTRINSIC(require_read_lock, void, (int64_t val1, int64_t val2), (val1, val2), require_read_lock)
REGISTER_NATIVE_APPLY_CONTEXT_INTRINSIC(is_account, int, (int64_t val), (val), is_account)

#define DB_NATIVE_METHOD_SEQ(SUFFIX, CLASS) \
   (store_##SUFFIX,  int32_t, (int64_t scope, int64_t table, int64_t bta, const char* data, uint32_t len), (scope, table, bta, array_ptr<const char>(data), len), store) \
   (update_##SUFFIX, int32_t, (int64_t scope, int64_t table, int64_t bta, const char* data, uint32_t len), (scope, table, bta, array_ptr<const char>(data), len), update) \
   (remove_##SUFFIX, int32_t, (int64_t scope, int64_t table, CLASS::KeyParamType keys), (scope, table, keys), remove)

#define DB_NATIVE_INDEX_METHOD_SEQ(SUFFIX) \
   (load_##SUFFIX,         int32_t, (int64_t code, int64_t scope, int64_t table, char* data, uint32_t len), (code, scope, table, array_ptr<char>(data), len), load) \
   (front_##SUFFIX,        int32_t, (int64_t code, int64_t scope, int64_t table, char* data, uint32_t len), (code, scope, table, array_ptr<char>(data), len), front) \
   (back_##SUFFIX,         int32_t, (int64_t code, int64_t scope, int64_t table, char* data, uint32_t len), (code, scope, table, array_ptr<char>(data), len), back) \
   (previous_##SUFFIX,     int32_t, (int64_t code, int64_t scope, int64_t table, char* data, uint32_t len), (code, scope, table, array_ptr<char>(data), len), previous) \
   (lower_bound_##SUFFIX,  int32_t, (int64_t code, int64_t scope, int64_t table, char* data, uint32_t len), (code, scope, table, array_ptr<char>(data), len), lower_bound) \
   (upper_bound_##SUFFIX,  int32_t, (int64_t code, int64_t scope, int64_t table, char* data, uint32_t len), (code, scope, table, array_ptr<char>(data), len), upper_bound)

REGISTER_NATIVE_INTRINSICS(db_api_key_value_object,         DB_NATIVE_METHOD_SEQ(i64, db_api_key_value_object))
REGISTER_NATIVE_INTRINSICS(db_api_key128x128_value_object,  DB_NATIVE_METHOD_SEQ(i128i128, db_api_key128x128_value_object))
REGISTER_NATIVE_INTRINSICS(db_api_key64x64_value_object,    DB_NATIVE_METHOD_SEQ(i64i64, db_api_key64x64_value_object))
REGISTER_NATIVE_INTRINSICS(db_api_key64x64x64_value_object, DB_NATIVE_METHOD_SEQ(i64i64i64, db_api_key64x64x64_value_object))

REGISTER_NATIVE_INTRINSICS(db_index_api_key_value_index_by_scope_primary,           DB_NATIVE_INDEX_METHOD_SEQ(i64))
REGISTER_NATIVE_INTRINSICS(db_index_api_key128x128_value_index_by_scope_primary,    DB_NATIVE_INDEX_METHOD_SEQ(primary_i128i128))
REGISTER_NATIVE_INTRINSICS(db_index_api_key128x128_value_index_by_scope_secondary,  DB_NATIVE_INDEX_METHOD_SEQ(secondary_i128i128))
REGISTER_NATIVE_INTRINSICS(db_index_api_key64x64_value_index_by_scope_primary,      DB_NATIVE_INDEX_METHOD_SEQ(primary_i64i64))
REGISTER_NATIVE_INTRINSICS(db_index_api_key64x64_value_index_by_scope_secondary,    DB_NATIVE_INDEX_METHOD_SEQ(secondary_i64i64))
REGISTER_NATIVE_INTRINSICS(db_index_api_key64x64x64_value_index_by_scope_primary,   DB_NATIVE_INDEX_METHOD_SEQ(primary_i64i64i64))
REGISTER_NATIVE_INTRINSICS(db_index_api_key64x64x64_value_index_by_scope_secondary, DB_NATIVE_INDEX_METHOD_SEQ(secondary_i64i64i64))
REGISTER_NATIVE_INTRINSICS(db_index_api_key64x64x64_value_index_by_scope_tertiary,  DB_NATIVE_INDEX_METHOD_SEQ(tertiary_i64i64i64))

REGISTER_NATIVE_INTRINSICS(db_api_keystr_value_object,
   (store_str,  int32_t, (int64_t scope, int64_t table, int64_t bta, char* key, uint32_t keylen, char* value, uint32_t len),
                (scope, table, bta, null_terminated_ptr(key), keylen, array_ptr<const char>(value), len))
   (update_str, int32_t, (int64_t scope, int64_t table, int64_t bta, char* key, uint32_t keylen, char* value, uint32_t len),
                (scope, table, bta, null_terminated_ptr(key), keylen, array_ptr<const char>(value), len))
   (remove_str, int32_t, (int64_t scope, int64_t table, char* key, uint32_t keylen), (scope, table, array_ptr<const char>(key), keylen))
)

// Note: front_str and back_str have fewer parameters in db.h
REGISTER_NATIVE_INTRINSICS(db_index_api_keystr_value_index_by_scope_primary,
   (load_str,        int32_t, (int64_t code, int64_t scope, int64_t table, char* key, uint32_t keylen, char* value, uint32_t len),
                     (code, scope, table, array_ptr<char>(key), keylen, array_ptr<char>(value), len))
   (front_str,       int32_t, (int64_t code, int64_t scope, int64_t table, char* key, uint32_t keylen, char* value, uint32_t len),
                     (code, scope, table, array_ptr<char>(key), keylen, array_ptr<char>(value), len))
   (back_str,        int32_t, (int64_t code, int64_t scope, int64_t table, char* key, uint32_t keylen, char* value, uint32_t len),
                     (code, scope, table, array_ptr<char>(key), keylen, array_ptr<char>(value), len))
   (next_str,        int32_t, (int64_t code, int64_t scope, int64_t table, char* key, uint32_t keylen, char* value, uint32_t len),
                     (code, scope, table, array_ptr<char>(key), keylen, array_ptr<char>(value), len))
   (previous_str,    int32_t, (int64_t code, int64_t scope, int64_t table, char* key, uint32_t keylen, char* value, uint32_t len),
                     (code, scope, table, array_ptr<char>(key), keylen, array_ptr<char>(value), len))
   (lower_bound_str, int32_t, (int64_t code, int64_t scope, int64_t table, char* key, uint32_t keylen, char* value, uint32_t len),
                     (code, scope, table, array_ptr<char>(key), keylen, array_ptr<char>(value), len))
   (upper_bound_str, int32_t, (int64_t code, int64_t scope, int64_t table, char* key, uint32_t keylen, char* value, uint32_t len),
                     (code, scope, table, array_ptr<char>(key), keylen, array_ptr<char>(value), len))
)

} // extern "C"


} } /// eosio::chain
