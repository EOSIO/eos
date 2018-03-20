#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <eosio/chain/wasm_interface_private.hpp>
#include <eosio/chain/wasm_eosio_validation.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
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
#include <fstream>

#include <mutex>
#include <thread>
#include <condition_variable>

namespace eosio { namespace chain {
   using namespace contracts;
   using namespace webassembly;
   using namespace webassembly::common;


   /**
    *  Implementation class for the wasm cache
    *  it is responsible for compiling and storing instances of wasm code for use
    *
    */
   struct wasm_cache_impl {
      wasm_cache_impl()
      {
         // TODO clean this up
         //check_wasm_opcode_dispositions();
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
         Runtime::freeUnreferencedObjects({});
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
         explicit code_info(wavm::info&& wavm_info, binaryen::info&& binaryen_info)
         : wavm_info(std::forward<wavm::info>(wavm_info))
         , binaryen_info(std::forward<binaryen::info>(binaryen_info))
         {}


         wavm::info   wavm_info;
         binaryen::info binaryen_info;

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
      struct test_mutator {
         static std::vector<uint8_t> accept( wasm_ops::instr* inst ) {
            std::cout << "ACCEPTING\n";
            return {};
         }
      };
      struct test_mutator2 {
         static std::vector<uint8_t> accept( wasm_ops::instr* inst ) {
            std::cout << "ACCEPTING2\n";
            return {};
         }
      };

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

               fc::optional<wavm::entry> wavm;
               fc::optional<wavm::info> wavm_info;
               fc::optional<binaryen::entry> binaryen;
               fc::optional<binaryen::info> binaryen_info;

               try {

                  IR::Module* module = new IR::Module();

                  Serialization::MemoryInputStream stream((const U8 *) wasm_binary, wasm_binary_size);
                  WASM::serialize(stream, *module);

                  wasm_validations::wasm_binary_validation validator( *module );
                  wasm_injections::wasm_binary_injection injector( *module );

                  injector.inject();
                  validator.validate();
                  Serialization::ArrayOutputStream outstream;
                  WASM::serialize(outstream, *module);
                  std::vector<U8> bytes = outstream.getBytes();

                  wavm = wavm::entry::build((char*)bytes.data(), bytes.size());
                  wavm_info.emplace(*wavm);

                  binaryen = binaryen::entry::build((char*)bytes.data(), bytes.size());
                  binaryen_info.emplace(*binaryen);
               } catch (...) {
                  pending_error = std::current_exception();
               }

               if (pending_error == nullptr) {
                  // grab the lock and put this in the cache as unavailble
                  with_lock(_cache_lock, [&,this]() {
                     // find or create a new entry
                     auto iter = _cache.emplace(code_id, code_info(std::move(*wavm_info),std::move(*binaryen_info))).first;

                     iter->second.instances.emplace_back(std::make_unique<wasm_cache::entry>(std::move(*wavm), std::move(*binaryen)));
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
         // sanitize by reseting the memory that may now be dirty
         auto& info = (*fetch_info(code_id)).get();
         entry.wavm.reset(info.wavm_info);
         entry.binaryen.reset(info.binaryen_info);

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
         auto& info = (*fetch_info(code_id)).get();
         wasm_cache_entry.wavm.prepare(info.wavm_info);
         wasm_cache_entry.binaryen.prepare(info.binaryen_info);
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
      scoped_context(optional<wasm_context> &context, Args&&... args)
      :context(context)
      {
         context.emplace( std::forward<Args>(args)... );
      }

      ~scoped_context() {
         context.reset();
      }

      optional<wasm_context>& context;
   };

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

   void wasm_interface::apply( wasm_cache::entry& code, apply_context& context, vm_type vm ) {
      try {
         auto context_guard = scoped_context(my->current_context, code, context, vm);
         switch (vm) {
            case vm_type::wavm:
               code.wavm.call_apply(context);
               break;
            case vm_type::binaryen:
               code.binaryen.call_apply(context);
               break;
         }
      } catch ( const wasm_exit& ){}
   }

   void wasm_interface::error( wasm_cache::entry& code, apply_context& context, vm_type vm ) {
      try {
         auto context_guard = scoped_context(my->current_context, code, context, vm);
         switch (vm) {
            case vm_type::wavm:
               code.wavm.call_error(context);
               break;
            case vm_type::binaryen:
               code.binaryen.call_error(context);
               break;
         }
      } catch ( const wasm_exit& ){}
   }

   wasm_context& common::intrinsics_accessor::get_context(wasm_interface &wasm) {
      FC_ASSERT(wasm.my->current_context.valid());
      return *wasm.my->current_context;
   }

   const wavm::entry& wavm::entry::get(wasm_interface& wasm) {
      return common::intrinsics_accessor::get_context(wasm).code.wavm;
   }


   const binaryen::entry& binaryen::entry::get(wasm_interface& wasm) {
      return common::intrinsics_accessor::get_context(wasm).code.binaryen;
   }

#if defined(assert)
   #undef assert
#endif

class context_aware_api {
   public:
      context_aware_api(wasm_interface& wasm, bool context_free = false )
      :code(intrinsics_accessor::get_context(wasm).code),
       context(intrinsics_accessor::get_context(wasm).context)
      ,vm(intrinsics_accessor::get_context(wasm).vm)
      {
         if( context.context_free )
            FC_ASSERT( context_free, "only context free api's can be used in this context" );
         context.used_context_free_api |= !context_free;
      }

   protected:
      wasm_cache::entry&         code;
      apply_context&             context;
      wasm_interface::vm_type    vm;

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
                                uint64_t ram_bytes, int64_t net_weight, int64_t cpu_weight,
                                int64_t /*cpu_usec_per_period*/ ) {
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
   explicit checktime_api( wasm_interface& wasm )
   :context_aware_api(wasm,true){}

   void checktime(uint32_t instruction_count) {
      context.checktime(instruction_count);
   }
};

class softfloat_api : public context_aware_api {
   public:
      // TODO add traps on truncations for special cases (NaN or outside the range which rounds to an integer)
      using context_aware_api::context_aware_api;
      // float binops
      float32_t _eosio_f32_add( float32_t a, float32_t b ) { return f32_add( a, b ); }
      float32_t _eosio_f32_sub( float32_t a, float32_t b ) { return f32_sub( a, b ); }
      float32_t _eosio_f32_div( float32_t a, float32_t b ) { return f32_div( a, b ); }
      float32_t _eosio_f32_mul( float32_t a, float32_t b ) { return f32_mul( a, b ); }
      float32_t _eosio_f32_min( float32_t a, float32_t b ) { return f32_lt( a, b ) ? a : b; }
      float32_t _eosio_f32_max( float32_t a, float32_t b ) { return f32_lt( a, b ) ? b : a; }
      float32_t _eosio_f32_copysign( float32_t a, float32_t b ) {
         uint32_t sign_of_a = a.v >> 31;
         uint32_t sign_of_b = b.v >> 31;
         a.v &= ~(1 << 31);             // clear the sign bit
         a.v = a.v | (sign_of_b << 31); // add the sign of b
         return a;
      }
      // float unops
      float32_t _eosio_f32_abs( float32_t a ) { 
         a.v &= ~(1 << 31);  
         return a; 
      }
      float32_t _eosio_f32_neg( float32_t a ) { 
         uint32_t sign = a.v >> 31;
         a.v &= ~(1 << 31);  
         a.v |= (!sign << 31);
         return a; 
      }
      float32_t _eosio_f32_sqrt( float32_t a ) { return f32_sqrt( a ); }
      // ceil, floor, trunc and nearest are lifted from libc
      float32_t _eosio_f32_ceil( float32_t a ) {
         int e = (int)(a.v >> 23 & 0xff) - 0x7f;
         uint32_t m;
         if (e >= 23)
            return a;
         if (e >= 0) {
            m = 0x007fffff >> e;
            if ((a.v & m) == 0)
               return a;
            if (a.v >> 31 == 0)
               a.v += m;
            a.v &= ~m;
         } else {
            if (a.v >> 31)
               a.v = 0x80000000; // return -0.0f
            else if (a.v << 1)
               a.v = 0x3F800000; // return 1.0f
         }
         return a;
      }
      float32_t _eosio_f32_floor( float32_t a ) {
         int e = (int)(a.v >> 23 & 0xff) - 0x7f;
         uint32_t m;
         if (e >= 23)
            return a;
         if (e >= 0) {
            m = 0x007fffff >> e;
            if ((a.v & m) == 0)
               return a;
            if (a.v >> 31)
               a.v += m;
            a.v &= ~m;
         } else {
            if (a.v >> 31 == 0)
               a.v = 0;
            else if (a.v << 1)
               a.v = 0x3F800000; // return 1.0f
         }
         return a;
      }
      float32_t _eosio_f32_trunc( float32_t a ) {
         int e = (int)(a.v >> 23 & 0xff) - 0x7f + 9;
         uint32_t m;
         if (e >= 23 + 9)
            return a;
         if (e < 9)
            e = 1;
         m = -1U >> e;
         if ((a.v & m) == 0)
            return a;
         a.v &= ~m;
         return a;
      }
      float32_t _eosio_f32_nearest( float32_t a ) {
         int e = a.v>>23 & 0xff;
         int s = a.v>>31;
         float32_t y;
         if (e >= 0x7f+23)
            return a;
         if (s)
            y = f32_add( f32_sub( a, inv_float_eps ), inv_float_eps );
         else
            y = f32_sub( f32_add( a, inv_float_eps ), inv_float_eps );
         if (f32_eq( y, {0} ) )
            return s ? float32_t{0x80000000} : float32_t{0}; // return either -0.0 or 0.0f
         return y;
      }
      // float relops
      bool _eosio_f32_eq( float32_t a, float32_t b ) { return f32_eq( a, b ); }
      bool _eosio_f32_ne( float32_t a, float32_t b ) { return !f32_eq( a, b ); }
      bool _eosio_f32_lt( float32_t a, float32_t b ) { return f32_lt( a, b ); }
      bool _eosio_f32_le( float32_t a, float32_t b ) { return f32_le( a, b ); }
      bool _eosio_f32_gt( float32_t a, float32_t b ) { return !f32_le( a, b ); }
      bool _eosio_f32_ge( float32_t a, float32_t b ) { return !f32_lt( a, b ); }

      // double binops
      float64_t _eosio_f64_add( float64_t a, float64_t b ) { return f64_add( a, b ); }
      float64_t _eosio_f64_sub( float64_t a, float64_t b ) { return f64_sub( a, b ); }
      float64_t _eosio_f64_div( float64_t a, float64_t b ) { return f64_div( a, b ); }
      float64_t _eosio_f64_mul( float64_t a, float64_t b ) { return f64_mul( a, b ); }
      float64_t _eosio_f64_min( float64_t a, float64_t b ) { return f64_lt( a, b ) ? a : b; }
      float64_t _eosio_f64_max( float64_t a, float64_t b ) { return f64_lt( a, b ) ? b : a; }
      float64_t _eosio_f64_copysign( float64_t a, float64_t b ) {
         uint64_t sign_of_a = a.v >> 63;
         uint64_t sign_of_b = b.v >> 63;
         a.v &= ~(uint64_t(1) << 63);             // clear the sign bit
         a.v = a.v | (sign_of_b << 63); // add the sign of b
         return a;
      }

      // double unops
      float64_t _eosio_f64_abs( float64_t a ) { 
         a.v &= ~(uint64_t(1) << 63);  
         return a; 
      }
      float64_t _eosio_f64_neg( float64_t a ) { 
         uint64_t sign = a.v >> 63;
         a.v &= ~(uint64_t(1) << 63);  
         a.v |= (uint64_t(!sign) << 63);
         return a; 
      }
      float64_t _eosio_f64_sqrt( float64_t a ) { return f64_sqrt( a ); }
      // ceil, floor, trunc and nearest are lifted from libc
      float64_t _eosio_f64_ceil( float64_t a ) {
         int e = a.v >> 52 & 0x7ff;
         float64_t y;
         if (e >= 0x3ff+52 || f64_eq( a, { 0 } ))
         return a;
         /* y = int(x) - x, where int(x) is an integer neighbor of x */
         if (a.v >> 63)
            y = f64_sub( f64_add( f64_sub( a, inv_double_eps ), inv_double_eps ), a );
         else
            y = f64_sub( f64_sub( f64_add( a, inv_double_eps ), inv_double_eps ), a );
         /* special case because of non-nearest rounding modes */
         if (e <= 0x3ff-1) {
            return a.v >> 63 ? float64_t{0x8000000000000000} : float64_t{0xBE99999A3F800000}; //either -0.0 or 1
         }
         if (f64_lt( y, { 0 } ))
            return f64_add( f64_add( a, y ), { 0xBE99999A3F800000 } ); // plus 1
         return f64_add( a, y );
      }

      float64_t _eosio_f64_trunc( float64_t a ) {
         int e = (int)(a.v >> 52 & 0x7ff) - 0x3ff + 12;
         uint64_t m;
         if (e >= 52 + 12)
            return a;
         if (e < 12)
            e = 1;
         m = -1ULL >> e;
         if ((a.v & m) == 0)
            return a;
         a.v &= ~m;
         return a;
      }

      // float and double conversions
      float64_t _eosio_f32_promote( float32_t a ) { return f32_to_f64( a ); }
      float32_t _eosio_f64_demote( float64_t a ) { return f64_to_f32( a ); }
      int32_t _eosio_f32_trunc_i32s( float32_t a ) { return f32_to_i32( _eosio_f32_trunc( a ), 0, false ); }
      int32_t _eosio_f64_trunc_i32s( float64_t a ) { return f64_to_i32( _eosio_f64_trunc( a ), 0, false ); }
      uint32_t _eosio_f32_trunc_i32u( float32_t a ) { return f32_to_ui32( _eosio_f32_trunc( a ), 0, false ); }
      uint32_t _eosio_f64_trunc_i32u( float64_t a ) { return f64_to_ui32( _eosio_f64_trunc( a ), 0, false ); }
      int64_t _eosio_f32_trunc_i64s( float32_t a ) { return f32_to_i64( _eosio_f32_trunc( a ), 0, false ); }
      int64_t _eosio_f64_trunc_i64s( float64_t a ) { return f64_to_i64( _eosio_f64_trunc( a ), 0, false ); }
      uint64_t _eosio_f32_trunc_i64u( float32_t a ) { return f32_to_ui64( _eosio_f32_trunc( a ), 0, false ); }
      uint64_t _eosio_f64_trunc_i64u( float64_t a ) { return f64_to_ui64( _eosio_f64_trunc( a ), 0, false ); }
      float32_t _eosio_i32_to_f32( int32_t a ) { return i32_to_f32( a ); }
      float32_t _eosio_i64_to_f32( int64_t a ) { return i64_to_f32( a ); }
      float32_t _eosio_ui32_to_f32( uint32_t a ) { return ui32_to_f32( a ); }
      float32_t _eosio_ui64_to_f32( uint64_t a ) { return ui64_to_f32( a ); }
      float64_t _eosio_i32_to_f64( int32_t a ) { return i32_to_f64( a ); }
      float64_t _eosio_i64_to_f64( int64_t a ) { return i64_to_f64( a ); }
      float64_t _eosio_ui32_to_f64( uint32_t a ) { return ui32_to_f64( a ); }
      float64_t _eosio_ui64_to_f64( uint64_t a ) { return ui64_to_f64( a ); }


   private:
      static constexpr float32_t inv_float_eps = { 0x4B000000 }; 
      static constexpr float64_t inv_double_eps = { 0x4330000000000000 };
};
class producer_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      int get_active_producers(array_ptr<chain::account_name> producers, size_t datalen) {
         auto active_producers = context.get_active_producers();
         size_t len = active_producers.size();
         size_t cpy_len = std::min(datalen, len);
         memcpy(producers, active_producers.data(), cpy_len * sizeof(chain::account_name));
         return len;
      }
};

class crypto_api : public context_aware_api {
   public:
      explicit crypto_api( wasm_interface& wasm )
      :context_aware_api(wasm,true){}

      /**
       * This method can be optimized out during replay as it has
       * no possible side effects other than "passing".
       */
      void assert_recover_key( const fc::sha256& digest,
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

      int recover_key( const fc::sha256& digest,
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
      explicit system_api( wasm_interface& wasm )
      :context_aware_api(wasm,true){}

      void abort() {
         edump(("abort() called"));
         FC_ASSERT( false, "abort() called");
      }

      void eosio_assert(bool condition, null_terminated_ptr str) {
         if( !condition ) {
            std::string message( str );
            edump((message));
            FC_ASSERT( condition, "assertion failed: ${s}", ("s",message));
         }
      }

      void eosio_exit(int32_t code) {
         throw wasm_exit{code};
      }

      fc::time_point_sec now() {
         return context.controller.head_block_time();
      }
};

class action_api : public context_aware_api {
   public:
   action_api( wasm_interface& wasm )
      :context_aware_api(wasm,true){}

      int read_action_data(array_ptr<char> memory, size_t size) {
         FC_ASSERT(size > 0);
         int minlen = std::min<size_t>(context.act.data.size(), size);
         memcpy((void *)memory, context.act.data.data(), minlen);
         return minlen;
      }

      int action_data_size() {
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
      console_api( wasm_interface& wasm )
      :context_aware_api(wasm,true){}

      void prints(null_terminated_ptr str) {
         context.console_append<const char*>(str);
      }

      void prints_l(array_ptr<const char> str, size_t str_len ) {
         context.console_append(string(str, str_len));
      }

      void printui(uint64_t val) {
         context.console_append(val);
      }

      void printi(int64_t val) {
         context.console_append(val);
      }

      void printi128(const unsigned __int128& val) {
         fc::uint128_t v(val>>64, uint64_t(val) );
         context.console_append(fc::variant(v).get_string());
      }

      void printdi( uint64_t val ) {
         context.console_append(*((double*)&val));
      }

      void printd( float64_t val ) {
         context.console_append(*((double*)&val));
      }

      void printn(const name& value) {
         context.console_append(value.to_string());
      }

      void printhex(array_ptr<const char> data, size_t data_len ) {
         context.console_append(fc::to_hex(data, data_len));
      }
};

#define DB_API_METHOD_WRAPPERS_SIMPLE_SECONDARY(IDX, TYPE)\
      int db_##IDX##_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE& secondary ) {\
         return context.IDX.store( scope, table, payer, id, secondary );\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, const TYPE& secondary ) {\
         return context.IDX.update( iterator, payer, secondary );\
      }\
      void db_##IDX##_remove( int iterator ) {\
         return context.IDX.remove( iterator );\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE& secondary, uint64_t& primary ) {\
         return context.IDX.find_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t primary ) {\
         return context.IDX.find_primary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE& secondary, uint64_t& primary ) {\
         return context.IDX.lowerbound_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE& secondary, uint64_t& primary ) {\
         return context.IDX.upperbound_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_end( uint64_t code, uint64_t scope, uint64_t table ) {\
         return context.IDX.end_secondary(code, scope, table);\
      }\
      int db_##IDX##_next( int iterator, uint64_t& primary  ) {\
         return context.IDX.next_secondary(iterator, primary);\
      }\
      int db_##IDX##_previous( int iterator, uint64_t& primary ) {\
         return context.IDX.previous_secondary(iterator, primary);\
      }

#define DB_API_METHOD_WRAPPERS_ARRAY_SECONDARY(IDX, ARR_SIZE, ARR_ELEMENT_TYPE)\
      int db_##IDX##_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, array_ptr<const ARR_ELEMENT_TYPE> data, size_t data_len) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.store(scope, table, payer, id, data.value);\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, array_ptr<const ARR_ELEMENT_TYPE> data, size_t data_len ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.update(iterator, payer, data.value);\
      }\
      void db_##IDX##_remove( int iterator ) {\
         return context.IDX.remove(iterator);\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, array_ptr<const ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t& primary ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.find_secondary(code, scope, table, data, primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, array_ptr<ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t primary ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.find_primary(code, scope, table, data.value, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table, array_ptr<ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t& primary ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.lowerbound_secondary(code, scope, table, data.value, primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table, array_ptr<ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t& primary ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.upperbound_secondary(code, scope, table, data.value, primary);\
      }\
      int db_##IDX##_end( uint64_t code, uint64_t scope, uint64_t table ) {\
         return context.IDX.end_secondary(code, scope, table);\
      }\
      int db_##IDX##_next( int iterator, uint64_t& primary  ) {\
         return context.IDX.next_secondary(iterator, primary);\
      }\
      int db_##IDX##_previous( int iterator, uint64_t& primary ) {\
         return context.IDX.previous_secondary(iterator, primary);\
      }


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
      int db_end_i64( uint64_t code, uint64_t scope, uint64_t table ) {
         return context.db_end_i64( code, scope, table );
      }

      DB_API_METHOD_WRAPPERS_SIMPLE_SECONDARY(idx64,  uint64_t)
      DB_API_METHOD_WRAPPERS_SIMPLE_SECONDARY(idx128, uint128_t)
      DB_API_METHOD_WRAPPERS_ARRAY_SECONDARY(idx256, 2, uint128_t)
      DB_API_METHOD_WRAPPERS_SIMPLE_SECONDARY(idx_double, uint64_t)
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

      int remove_str(const scope_name& scope, const name& table, array_ptr<const char> &key, uint32_t key_len) {
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

         switch(vm) {
            case wasm_interface::vm_type::wavm:
               return (uint32_t)code.wavm.sbrk(num_bytes);
            case wasm_interface::vm_type::binaryen:
               return (uint32_t)code.binaryen.sbrk(num_bytes);
         }
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
      
      void __addtf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         ret = f128_add( a, b ); 
      }
      void __subtf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         ret = f128_sub( a, b ); 
      }
      void __multf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         ret = f128_mul( a, b ); 
      }
      void __divtf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         ret = f128_div( a, b ); 
      }
      int __eqtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ la, ha }};
         return f128_eq( a, b ); 
      }
      int __netf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ la, ha }};
         return !f128_eq( a, b ); 
      }
      int __getf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ la, ha }};
         return !f128_lt( a, b ); 
      }
      int __gttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ la, ha }};
         return !f128_lt( a, b ) && !f128_eq( a, b ); 
      }
      int __letf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ la, ha }};
         return f128_le( a, b ); 
      }
      int __lttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ la, ha }};
         return f128_lt( a, b ); 
      }
      int __cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ la, ha }};
         if ( f128_lt( a, b ) )
            return -1;
         if ( f128_eq( a, b ) )
            return 0;
         return 1;
      }
      int __unordtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) { 
         float128_t a = {{ la, ha }};
         float128_t b = {{ la, ha }};
         if ( f128_isSignalingNaN( a ) || f128_isSignalingNaN( b ) )
            return 1;
         return 0;
      }
      float64_t __floatsidf( int32_t i ) {
         edump((i)( "warning returning float64") );
         return i32_to_f64(i);
      }
      void __floatsitf( float128_t& ret, int32_t i ) {
         ret = i32_to_f128(i); /// TODO: should be 128
      }
      void __floatunsitf( float128_t& ret, uint32_t i ) {
         ret = ui32_to_f128(i); /// TODO: should be 128
      }
      /*
      float128_t __floatsit( int32_t i ) {
         return i32_to_f128(i);
      }
      */
      void __extendsftf2( float128_t& ret, uint32_t f ) { 
         float32_t in = { f };
         ret = f32_to_f128( in ); 
      }
      void __extenddftf2( float128_t& ret, float64_t in ) { 
         edump(("warning in flaot64..." ));
//         float64_t in = { f };
         ret = f64_to_f128( in ); 
      }
      int64_t __fixtfdi( uint64_t l, uint64_t h ) { 
         float128_t f = {{ l, h }};
         return f128_to_i64( f, 0, false ); 
      } 
      int32_t __fixtfsi( uint64_t l, uint64_t h ) { 
         float128_t f = {{ l, h }};
         return f128_to_i32( f, 0, false ); 
      } 
      uint64_t __fixunstfdi( uint64_t l, uint64_t h ) { 
         float128_t f = {{ l, h }};
         return f128_to_ui64( f, 0, false ); 
      } 
      uint32_t __fixunstfsi( uint64_t l, uint64_t h ) { 
         float128_t f = {{ l, h }};
         return f128_to_ui32( f, 0, false ); 
      } 
      uint64_t __trunctfdf2( uint64_t l, uint64_t h ) { 
         float128_t f = {{ l, h }};
         return f128_to_f64( f ).v; 
      } 
      uint32_t __trunctfsf2( uint64_t l, uint64_t h ) { 
         float128_t f = {{ l, h }};
         return f128_to_f32( f ).v; 
      } 

      static constexpr uint32_t SHIFT_WIDTH = (sizeof(uint64_t)*8)-1;
};

class math_api : public context_aware_api {
   public:
      math_api( wasm_interface& wasm )
      :context_aware_api(wasm,true){}

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
   (__addtf3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__subtf3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__multf3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__divtf3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__eqtf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__netf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__getf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__gttf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__lttf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__cmptf2,      int(int64_t, int64_t, int64_t, int64_t)        )
   (__unordtf2,    int(int64_t, int64_t, int64_t, int64_t)        )
   (__floatsitf,   void (int, int)                                )
   (__floatunsitf, void (int, int)                                )
   (__floatsidf,   float64_t(int)                                 )
   (__extendsftf2, void(int, int)                                 )      
   (__extenddftf2, void(int, float64_t)                           )      
   (__fixtfdi,     int64_t(int64_t, int64_t)                      )
   (__fixtfsi,     int(int64_t, int64_t)                          )
   (__fixunstfdi,  int64_t(int64_t, int64_t)                      )
   (__fixunstfsi,  int(int64_t, int64_t)                          )
   (__trunctfdf2,  int64_t(int64_t, int64_t)                      )
   (__trunctfsf2,  int(int64_t, int64_t)                          )
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

#define DB_SECONDARY_INDEX_METHODS_SIMPLE(IDX) \
   (db_##IDX##_store,          int(int64_t,int64_t,int64_t,int64_t,int))\
   (db_##IDX##_remove,         void(int))\
   (db_##IDX##_update,         void(int,int64_t,int))\
   (db_##IDX##_find_primary,   int(int64_t,int64_t,int64_t,int,int64_t))\
   (db_##IDX##_find_secondary, int(int64_t,int64_t,int64_t,int,int))\
   (db_##IDX##_lowerbound,     int(int64_t,int64_t,int64_t,int,int))\
   (db_##IDX##_upperbound,     int(int64_t,int64_t,int64_t,int,int))\
   (db_##IDX##_end,            int(int64_t,int64_t,int64_t))\
   (db_##IDX##_next,           int(int, int))\
   (db_##IDX##_previous,       int(int, int))

#define DB_SECONDARY_INDEX_METHODS_ARRAY(IDX) \
      (db_##IDX##_store,          int(int64_t,int64_t,int64_t,int64_t,int,int))\
      (db_##IDX##_remove,         void(int))\
      (db_##IDX##_update,         void(int,int64_t,int,int))\
      (db_##IDX##_find_primary,   int(int64_t,int64_t,int64_t,int,int,int64_t))\
      (db_##IDX##_find_secondary, int(int64_t,int64_t,int64_t,int,int,int))\
      (db_##IDX##_lowerbound,     int(int64_t,int64_t,int64_t,int,int,int))\
      (db_##IDX##_upperbound,     int(int64_t,int64_t,int64_t,int,int,int))\
      (db_##IDX##_end,            int(int64_t,int64_t,int64_t))\
      (db_##IDX##_next,           int(int, int))\
      (db_##IDX##_previous,       int(int, int))

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
   (db_end_i64,          int(int64_t,int64_t,int64_t))

   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx64)
   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx128)
   DB_SECONDARY_INDEX_METHODS_ARRAY(idx256)
   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_double)
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
   (eosio_exit,   void(int ))
   (now,          int())
);

REGISTER_INTRINSICS(action_api,
   (read_action_data,       int(int, int)  )
   (action_data_size,       int()          )
   (current_receiver,   int64_t()          )
   (publication_time,   int32_t()          )
   (current_sender,     int64_t()          )
);

REGISTER_INTRINSICS(apply_context,
   (require_write_lock,    void(int64_t)          )
   (require_read_lock,     void(int64_t, int64_t) )
   (require_recipient,     void(int64_t)          )
   (require_authorization, void(int64_t), "require_auth", void(apply_context::*)(const account_name&)const)
   (has_authorization,     int(int64_t), "has_auth", bool(apply_context::*)(const account_name&)const)
   (is_account,            int(int64_t)           )
);

REGISTER_INTRINSICS(console_api,
   (prints,                void(int)       )
   (prints_l,              void(int, int)  )
   (printui,               void(int64_t)   )
   (printi,                void(int64_t)   )
   (printi128,             void(int)       )
   (printd,                void(float64_t) )
   (printdi,               void(int64_t)   )
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


} } /// eosio::chain
