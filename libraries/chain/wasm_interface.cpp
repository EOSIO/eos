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

#include <softfloat.hpp>
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

   wasm_interface::wasm_interface(vm_type vm)
      :my( new wasm_interface_impl(vm) ) {
      }

   wasm_interface::~wasm_interface() {}

   void wasm_interface::validate(const bytes& code) {
      Module module;
      Serialization::MemoryInputStream stream((U8*)code.data(), code.size());
      WASM::serialize(stream, module);

      wasm_validations::wasm_binary_validation validator(module);
      validator.validate();

      root_resolver resolver;
      LinkResult link_result = linkModule(module, resolver);

      //there are a couple opportunties for improvement here--
      //Easy: Cache the Module created here so it can be reused for instantiaion
      //Hard: Kick off instantiation in a separate thread at this location
   }

   void wasm_interface::apply( const digest_type& code_id, const shared_vector<char>& code, apply_context& context ) {
      my->get_instantiated_module(code_id, code)->apply(context);
   }

   wasm_instantiated_module_interface::~wasm_instantiated_module_interface() {}
   wasm_runtime_interface::~wasm_runtime_interface() {}

#if defined(assert)
   #undef assert
#endif

class context_aware_api {
   public:
      context_aware_api(apply_context& ctx, bool context_free = false )
      :context(ctx)
      {
         if( context.context_free )
            FC_ASSERT( context_free, "only context free api's can be used in this context" );
         context.used_context_free_api |= !context_free;
      }

   protected:
      apply_context&             context;

};

class context_free_api : public context_aware_api {
   public:
      context_free_api( apply_context& ctx )
      :context_aware_api(ctx, true) {
         /* the context_free_data is not available during normal application because it is prunable */
         FC_ASSERT( context.context_free, "this API may only be called from context_free apply" );
      }

      int get_context_free_data( uint32_t index, array_ptr<char> buffer, size_t buffer_size )const {
         return context.get_context_free_data( index, buffer, buffer_size );
      }
};

class privileged_api : public context_aware_api {
   public:
      privileged_api( apply_context& ctx )
      :context_aware_api(ctx)
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
   explicit checktime_api( apply_context& ctx )
   :context_aware_api(ctx,true){}

   void checktime(uint32_t instruction_count) {
      context.checktime(instruction_count);
   }
};

class softfloat_api : public context_aware_api {
   public:
      // TODO add traps on truncations for special cases (NaN or outside the range which rounds to an integer)
      using context_aware_api::context_aware_api;
      // float binops
      float _eosio_f32_add( float a, float b ) { 
         float32_t ret = f32_add( to_softfloat32(a), to_softfloat32(b) );
         return *reinterpret_cast<float*>(&ret);
      }
      float _eosio_f32_sub( float a, float b ) { 
         float32_t ret = f32_sub( to_softfloat32(a), to_softfloat32(b) );
         return *reinterpret_cast<float*>(&ret);
      }
      float _eosio_f32_div( float a, float b ) { 
         float32_t ret = f32_div( to_softfloat32(a), to_softfloat32(b) );
         return *reinterpret_cast<float*>(&ret);
      }
      float _eosio_f32_mul( float a, float b ) { 
         float32_t ret = f32_mul( to_softfloat32(a), to_softfloat32(b) );
         return *reinterpret_cast<float*>(&ret);
      }
      float _eosio_f32_min( float af, float bf ) { 
         float32_t a = to_softfloat32(af);
         float32_t b = to_softfloat32(bf);
         if (is_nan(a)) {
            return af;
         } 
         if (is_nan(b)) {
            return bf;
         }
         if ( sign_bit(a) != sign_bit(b) ) {
            return sign_bit(a) ? af : bf;
         }
         return f32_lt(a,b) ? af : bf; 
      }
      float _eosio_f32_max( float af, float bf ) { 
         float32_t a = to_softfloat32(af);
         float32_t b = to_softfloat32(bf);
         if (is_nan(a)) {
            return af;
         } 
         if (is_nan(b)) {
            return bf;
         }
         if ( sign_bit(a) != sign_bit(b) ) {
            return sign_bit(a) ? bf : af;
         }
         return f32_lt( a, b ) ? bf : af; 
      }
      float _eosio_f32_copysign( float af, float bf ) { 
         float32_t a = to_softfloat32(af);
         float32_t b = to_softfloat32(bf);
         uint32_t sign_of_a = a.v >> 31;
         uint32_t sign_of_b = b.v >> 31;
         a.v &= ~(1 << 31);             // clear the sign bit
         a.v = a.v | (sign_of_b << 31); // add the sign of b
         return from_softfloat32(a);
      }
      // float unops
      float _eosio_f32_abs( float af ) { 
         float32_t a = to_softfloat32(af);
         a.v &= ~(1 << 31);  
         return from_softfloat32(a); 
      }
      float _eosio_f32_neg( float af ) { 
         float32_t a = to_softfloat32(af);
         uint32_t sign = a.v >> 31;
         a.v &= ~(1 << 31);  
         a.v |= (!sign << 31);
         return from_softfloat32(a); 
      }
      float _eosio_f32_sqrt( float a ) { 
         float32_t ret = f32_sqrt( to_softfloat32(a) ); 
         return from_softfloat32(ret); 
      }
      // ceil, floor, trunc and nearest are lifted from libc
      float _eosio_f32_ceil( float af ) {
         float32_t a = to_softfloat32(af);
         int e = (int)(a.v >> 23 & 0xFF) - 0X7F;
         uint32_t m;
         if (e >= 23)
            return af; 
         if (e >= 0) {
            m = 0x007FFFFF >> e;
            if ((a.v & m) == 0)
               return af; 
            if (a.v >> 31 == 0)
               a.v += m;
            a.v &= ~m;
         } else {
            if (a.v >> 31)
               a.v = 0x80000000; // return -0.0f
            else if (a.v << 1)
               a.v = 0x3F800000; // return 1.0f
         }

         return from_softfloat32(a); 
      }
      float _eosio_f32_floor( float af ) {
         float32_t a = to_softfloat32(af);
         int e = (int)(a.v >> 23 & 0xFF) - 0X7F;
         uint32_t m;
         if (e >= 23)
            return af; 
         if (e >= 0) {
            m = 0x007FFFFF >> e;
            if ((a.v & m) == 0)
               return af; 
            if (a.v >> 31)
               a.v += m;
            a.v &= ~m;
         } else {
            if (a.v >> 31 == 0)
               a.v = 0;
            else if (a.v << 1)
               a.v = 0xBF800000; // return -1.0f
         }
         return from_softfloat32(a); 
      }
      float _eosio_f32_trunc( float af ) {
         float32_t a = to_softfloat32(af);
         int e = (int)(a.v >> 23 & 0xff) - 0x7f + 9;
         uint32_t m;
         if (e >= 23 + 9)
            return af; 
         if (e < 9)
            e = 1;
         m = -1U >> e;
         if ((a.v & m) == 0)
            return af; 
         a.v &= ~m;
         return from_softfloat32(a); 
      }
      float _eosio_f32_nearest( float af ) {
         float32_t a = to_softfloat32(af);
         int e = a.v>>23 & 0xff;
         int s = a.v>>31;
         float32_t y;
         if (e >= 0x7f+23)
            return af; 
         if (s)
            y = f32_add( f32_sub( a, float32_t{inv_float_eps} ), float32_t{inv_float_eps} );
         else
            y = f32_sub( f32_add( a, float32_t{inv_float_eps} ), float32_t{inv_float_eps} );
         if (f32_eq( y, {0} ) )
            return s ? -0.0f : 0.0f; 
         return from_softfloat32(y); 
      }

      // float relops
      bool _eosio_f32_eq( float a, float b ) {  return f32_eq( to_softfloat32(a), to_softfloat32(b) ); }
      bool _eosio_f32_ne( float a, float b ) { return !f32_eq( to_softfloat32(a), to_softfloat32(b) ); }
      bool _eosio_f32_lt( float a, float b ) { return f32_lt( to_softfloat32(a), to_softfloat32(b) ); }
      bool _eosio_f32_le( float a, float b ) { return f32_le( to_softfloat32(a), to_softfloat32(b) ); }
      bool _eosio_f32_gt( float af, float bf ) {  
         float32_t a = to_softfloat32(af);
         float32_t b = to_softfloat32(bf);
         if (is_nan(a))
            return false;
         if (is_nan(b))
            return false;
         return !f32_le( a, b ); 
      }
      bool _eosio_f32_ge( float af, float bf ) {
         float32_t a = to_softfloat32(af);
         float32_t b = to_softfloat32(bf);
         if (is_nan(a))
            return false;
         if (is_nan(b))
            return false;
         return !f32_lt( a, b ); 
      }

      // double binops
      double _eosio_f64_add( double a, double b ) { 
         float64_t ret = f64_add( to_softfloat64(a), to_softfloat64(b) ); 
         return from_softfloat64(ret); 
      }
      double _eosio_f64_sub( double a, double b ) { 
         float64_t ret = f64_sub( to_softfloat64(a), to_softfloat64(b) ); 
         return from_softfloat64(ret); 
      }
      double _eosio_f64_div( double a, double b ) { 
         float64_t ret = f64_div( to_softfloat64(a), to_softfloat64(b) ); 
         return from_softfloat64(ret); 
      }
      double _eosio_f64_mul( double a, double b ) { 
         float64_t ret = f64_mul( to_softfloat64(a), to_softfloat64(b) ); 
         return from_softfloat64(ret); 
      }
      double _eosio_f64_min( double af, double bf ) { 
         float64_t a = to_softfloat64(af);
         float64_t b = to_softfloat64(bf);
         if (is_nan(a))
            return af;
         if (is_nan(b))
            return bf;
         if (sign_bit(a) != sign_bit(b))
            return sign_bit(a) ? af : bf;
         return f64_lt( a, b ) ? af : bf;
      } 
      double _eosio_f64_max( double af, double bf ) { 
         float64_t a = to_softfloat64(af);
         float64_t b = to_softfloat64(bf);
         if (is_nan(a))
            return af;
         if (is_nan(b))
            return bf;
         if (sign_bit(a) != sign_bit(b))
            return sign_bit(a) ? bf : af;
         return f64_lt( a, b ) ? bf : af;
      }
      double _eosio_f64_copysign( double af, double bf ) {
         float64_t a = to_softfloat64(af);
         float64_t b = to_softfloat64(bf);
         uint64_t sign_of_a = a.v >> 63;
         uint64_t sign_of_b = b.v >> 63;
         a.v &= ~(uint64_t(1) << 63);             // clear the sign bit
         a.v = a.v | (sign_of_b << 63); // add the sign of b
         return from_softfloat64(a); 
      }

      // double unops
      double _eosio_f64_abs( double af ) { 
         float64_t a = to_softfloat64(af);
         a.v &= ~(uint64_t(1) << 63);  
         return from_softfloat64(a); 
      }
      double _eosio_f64_neg( double af ) { 
         float64_t a = to_softfloat64(af);
         uint64_t sign = a.v >> 63;
         a.v &= ~(uint64_t(1) << 63);  
         a.v |= (uint64_t(!sign) << 63);
         return from_softfloat64(a); 
      }
      double _eosio_f64_sqrt( double a ) { 
         float64_t ret = f64_sqrt( to_softfloat64(a) );
         return from_softfloat64(ret); 
      }
      // ceil, floor, trunc and nearest are lifted from libc
      double _eosio_f64_ceil( double af ) {
         float64_t a = to_softfloat64( af ); 
         float64_t ret;
         int e = a.v >> 52 & 0x7ff;
         float64_t y;
         if (e >= 0x3ff+52 || f64_eq( a, { 0 } ))
            return af;
         /* y = int(x) - x, where int(x) is an integer neighbor of x */
         if (a.v >> 63)
            y = f64_sub( f64_add( f64_sub( a, float64_t{inv_double_eps} ), float64_t{inv_double_eps} ), a );
         else
            y = f64_sub( f64_sub( f64_add( a, float64_t{inv_double_eps} ), float64_t{inv_double_eps} ), a );
         /* special case because of non-nearest rounding modes */
         if (e <= 0x3ff-1) {
            return a.v >> 63 ? -0.0 : 1.0; //float64_t{0x8000000000000000} : float64_t{0xBE99999A3F800000}; //either -0.0 or 1
         }
         if (f64_lt( y, to_softfloat64(0) )) {
            ret = f64_add( f64_add( a, y ), to_softfloat64(1) ); // 0xBE99999A3F800000 } ); // plus 1
            return from_softfloat64(ret); 
         }
         ret = f64_add( a, y );
         return from_softfloat64(ret); 
      }
      double _eosio_f64_floor( double af ) {
         float64_t a = to_softfloat64( af ); 
         float64_t ret;
         int e = a.v >> 52 & 0x7FF;
         float64_t y;
         double de = 1/DBL_EPSILON;
         if ( a.v == 0x8000000000000000) {
            return af;
         }
         if (e >= 0x3FF+52 || a.v == 0) {
            return af;
         }
         if (a.v >> 63)
            y = f64_sub( f64_add( f64_sub( a, float64_t{inv_double_eps} ), float64_t{inv_double_eps} ), a );
         else
            y = f64_sub( f64_sub( f64_add( a, float64_t{inv_double_eps} ), float64_t{inv_double_eps} ), a );
         if (e <= 0x3FF-1) {
            return a.v>>63 ? -1.0 : 0.0; //float64_t{0xBFF0000000000000} : float64_t{0}; // -1 or 0
         }
         if ( !f64_le( y, float64_t{0} ) ) {
            ret = f64_sub( f64_add(a,y), to_softfloat64(1.0));
            return from_softfloat64(ret); 
         }
         ret = f64_add( a, y );
         return from_softfloat64(ret); 
      }
      double _eosio_f64_trunc( double af ) {
         float64_t a = to_softfloat64( af ); 
         int e = (int)(a.v >> 52 & 0x7ff) - 0x3ff + 12;
         uint64_t m;
         if (e >= 52 + 12)
            return af;
         if (e < 12)
            e = 1;
         m = -1ULL >> e;
         if ((a.v & m) == 0)
            return af;
         a.v &= ~m;
         return from_softfloat64(a); 
      }

      double _eosio_f64_nearest( double af ) {
         float64_t a = to_softfloat64( af ); 
         int e = (a.v >> 52 & 0x7FF);
         int s = a.v >> 63;
         float64_t y;
         if ( e >= 0x3FF+52 )
            return af;
         if ( s )
            y = f64_add( f64_sub( a, float64_t{inv_double_eps} ), float64_t{inv_double_eps} );
         else
            y = f64_sub( f64_add( a, float64_t{inv_double_eps} ), float64_t{inv_double_eps} );
         if ( f64_eq( y, float64_t{0} ) )
            return s ? -0.0 : 0.0;
         return from_softfloat64(y); 
      }

      // double relops
      bool _eosio_f64_eq( double a, double b ) { return f64_eq( to_softfloat64(a), to_softfloat64(b) ); }
      bool _eosio_f64_ne( double a, double b ) { return !f64_eq( to_softfloat64(a), to_softfloat64(b) ); }
      bool _eosio_f64_lt( double a, double b ) { return f64_lt( to_softfloat64(a), to_softfloat64(b) ); }
      bool _eosio_f64_le( double a, double b ) { return f64_le( to_softfloat64(a), to_softfloat64(b) ); }
      bool _eosio_f64_gt( double af, double bf ) {  
         float64_t a = to_softfloat64(af);
         float64_t b = to_softfloat64(bf);
         if (is_nan(a))
            return false;
         if (is_nan(b))
            return false;
         return !f64_le( a, b ); 
      }
      bool _eosio_f64_ge( double af, double bf ) {
         float64_t a = to_softfloat64(af);
         float64_t b = to_softfloat64(bf);
         if (is_nan(a))
            return false;
         if (is_nan(b))
            return false;
         return !f64_lt( a, b ); 
      }

      // float and double conversions
      double _eosio_f32_promote( float a ) { 
         return from_softfloat64(f32_to_f64( to_softfloat32(a)) ); 
      }
      float _eosio_f64_demote( double a ) { 
         return from_softfloat32(f64_to_f32( to_softfloat64(a)) ); 
      }
      int32_t _eosio_f32_trunc_i32s( float af ) { 
         float32_t a = to_softfloat32(af);
         if (a.v == 0x4F000000 || a.v == 0xCF000001 || a.v == 0x7F800000 || a.v == 0xFF800000)
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 unrepresentable");
         return f32_to_i32( to_softfloat32(_eosio_f32_trunc( af )), 0, false ); 
      }
      int32_t _eosio_f64_trunc_i32s( double af ) { 
         float64_t a = to_softfloat64(af);
         if (a.v == 0x41E0000000000000 || a.v == 0xC1E0000000200000 || a.v == 0x7FF0000000000000 || a.v == 0xFFF0000000000000)
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_s/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_s/i32 unrepresentable");
         return f64_to_i32( to_softfloat64(_eosio_f64_trunc( af )), 0, false ); 
      }
      uint32_t _eosio_f32_trunc_i32u( float af ) { 
         float32_t a = to_softfloat32(af);
         if (a.v == 0x4F000000 || a.v == 0xCF000001 || a.v == 0x7F800000 || a.v == 0xFF800000)
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 unrepresentable");
         return f32_to_ui32( to_softfloat32(_eosio_f32_trunc( af )), 0, false ); 
      }
      uint32_t _eosio_f64_trunc_i32u( double af ) { 
         float64_t a = to_softfloat64(af);
         if (a.v == 0x41E0000000000000 || a.v == 0xC1E0000000200000 || a.v == 0x7FF0000000000000 || a.v == 0xFFF0000000000000)
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_s/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_s/i32 unrepresentable");
         return f64_to_ui32( to_softfloat64(_eosio_f64_trunc( af )), 0, false ); 
      }
      int64_t _eosio_f32_trunc_i64s( float af ) { 
         float32_t a = to_softfloat32(af);
         if (a.v ==  0x5F000000 || a.v ==  0xDF000001|| a.v == 0x7F800000 || a.v == 0xFF800000)
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 unrepresentable");
         return f32_to_i64( to_softfloat32(_eosio_f32_trunc( af )), 0, false ); 
      }
      int64_t _eosio_f64_trunc_i64s( double af ) { 
         float64_t a = to_softfloat64(af);
         if (a.v == 0x43E0000000000000 || a.v ==  0xC3E0000020000000 || a.v == 0x7FF0000000000000 || a.v == 0xFFF0000000000000)
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 unrepresentable");
 
         return f64_to_i64( to_softfloat64(_eosio_f64_trunc( af )), 0, false ); 
      }
      uint64_t _eosio_f32_trunc_i64u( float af ) { 
         float32_t a = to_softfloat32(af);
         if (a.v ==  0x5F000000 || a.v ==  0xDF000001|| a.v == 0x7F800000 || a.v == 0xFF800000)
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 unrepresentable");
         return f32_to_ui64( to_softfloat32(_eosio_f32_trunc( af )), 0, false ); 
      }
      uint64_t _eosio_f64_trunc_i64u( double af ) { 
         float64_t a = to_softfloat64(af);
         if (a.v == 0x43E0000000000000 || a.v ==  0xC3E0000020000000 || a.v == 0x7FF0000000000000 || a.v == 0xFFF0000000000000)
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 unrepresentable");
         return f64_to_ui64( to_softfloat64(_eosio_f64_trunc( af )), 0, false ); 
      }
      float _eosio_i32_to_f32( int32_t a )  { 
         return from_softfloat32(i32_to_f32( a )); 
      }
      float _eosio_i64_to_f32( int64_t a ) { 
         return from_softfloat32(i64_to_f32( a )); 
      }
      float _eosio_ui32_to_f32( uint32_t a ) { 
         return from_softfloat32(ui32_to_f32( a )); 
      }
      float _eosio_ui64_to_f32( uint64_t a ) { 
         return from_softfloat32(ui64_to_f32( a )); 
      }
      double _eosio_i32_to_f64( int32_t a ) { 
         return from_softfloat64(i32_to_f64( a )); 
      }
      double _eosio_i64_to_f64( int64_t a ) { 
         return from_softfloat64(i64_to_f64( a )); 
      }
      double _eosio_ui32_to_f64( uint32_t a ) { 
         return from_softfloat64(ui32_to_f64( a )); 
      }
      double _eosio_ui64_to_f64( uint64_t a ) {
         return from_softfloat64(ui64_to_f64( a )); 
      }


   private:
      inline float32_t to_softfloat32( float f ) {
         return *reinterpret_cast<float32_t*>(&f);
      }
      inline float64_t to_softfloat64( double d ) {
         return *reinterpret_cast<float64_t*>(&d);
      }
      inline float from_softfloat32( float32_t f ) {
         return *reinterpret_cast<float*>(&f);
      }
      inline double from_softfloat64( float64_t d ) {
         return *reinterpret_cast<double*>(&d);
      }
      static constexpr uint32_t inv_float_eps = 0x4B000000; 
      static constexpr uint64_t inv_double_eps = 0x4330000000000000;

      inline bool sign_bit( float32_t f ) { return f.v >> 31; }
      inline bool sign_bit( float64_t f ) { return f.v >> 63; }
      inline bool is_nan( float32_t f ) {
         return ((f.v & 0x7FFFFFFF) > 0x7F800000);
      }
      inline bool is_nan( float64_t f ) {
         return ((f.v & 0x7FFFFFFFFFFFFFFF) > 0x7FF0000000000000);
      }

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
      explicit crypto_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

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
      explicit system_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

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
   action_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

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
      console_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

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

      void printdf( double val ) {
         context.console_append(val);
      }

      void printff( float val ) {
         context.console_append(val);
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

class memory_api : public context_aware_api {
   public:
      memory_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

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
      context_free_transaction_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

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
      double __floatsidf( int32_t i ) {
         edump((i)( "warning returning float64") );
         float64_t ret = i32_to_f64(i); 
         return *reinterpret_cast<double*>(&ret);
      }
      void __floatsitf( float128_t& ret, int32_t i ) {
         ret = i32_to_f128(i); /// TODO: should be 128
      }
      void __floatunsitf( float128_t& ret, uint32_t i ) {
         ret = ui32_to_f128(i); /// TODO: should be 128
      }
      void __extendsftf2( float128_t& ret, uint32_t f ) { 
         float32_t in = { f };
         ret = f32_to_f128( in ); 
      }
      void __extenddftf2( float128_t& ret, double in ) { 
         edump(("warning in flaot64..." ));
         ret = f64_to_f128( float64_t{*(uint64_t*)&in} ); 
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
      math_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

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
   (__floatsidf,   double(int)                                 )
   (__extendsftf2, void(int, int)                                 )      
   (__extenddftf2, void(int, double)                           )      
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

   //(printdi,               void(int64_t)   )
REGISTER_INTRINSICS(console_api,
   (prints,                void(int)       )
   (prints_l,              void(int, int)  )
   (printui,               void(int64_t)   )
   (printi,                void(int64_t)   )
   (printi128,             void(int)       )
   (printdf,               void(double)    )
   (printff,               void(float)     )
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
);

REGISTER_INTRINSICS(softfloat_api,
      (_eosio_f32_add,       float(float, float)    )
      (_eosio_f32_sub,       float(float, float)    )
      (_eosio_f32_mul,       float(float, float)    )
      (_eosio_f32_div,       float(float, float)    )
      (_eosio_f32_min,       float(float, float)    )
      (_eosio_f32_max,       float(float, float)    )
      (_eosio_f32_copysign,  float(float, float)    )
      (_eosio_f32_abs,       float(float)           )
      (_eosio_f32_neg,       float(float)           )
      (_eosio_f32_sqrt,      float(float)           )
      (_eosio_f32_ceil,      float(float)           )
      (_eosio_f32_floor,     float(float)           )
      (_eosio_f32_trunc,     float(float)           )
      (_eosio_f32_nearest,   float(float)           )
      (_eosio_f32_eq,        int(float, float)      )
      (_eosio_f32_ne,        int(float, float)      )
      (_eosio_f32_lt,        int(float, float)      )
      (_eosio_f32_le,        int(float, float)      )
      (_eosio_f32_gt,        int(float, float)      )
      (_eosio_f32_ge,        int(float, float)      )
      (_eosio_f64_add,       double(double, double) )
      (_eosio_f64_sub,       double(double, double) )
      (_eosio_f64_mul,       double(double, double) )
      (_eosio_f64_div,       double(double, double) )
      (_eosio_f64_min,       double(double, double) )
      (_eosio_f64_max,       double(double, double) )
      (_eosio_f64_copysign,  double(double, double) )
      (_eosio_f64_abs,       double(double)         )
      (_eosio_f64_neg,       double(double)         )
      (_eosio_f64_sqrt,      double(double)         )
      (_eosio_f64_ceil,      double(double)         )
      (_eosio_f64_floor,     double(double)         )
      (_eosio_f64_trunc,     double(double)         )
      (_eosio_f64_nearest,   double(double)         )
      (_eosio_f64_eq,        int(double, double)    )
      (_eosio_f64_ne,        int(double, double)    )
      (_eosio_f64_lt,        int(double, double)    )
      (_eosio_f64_le,        int(double, double)    )
      (_eosio_f64_gt,        int(double, double)    )
      (_eosio_f64_ge,        int(double, double)    )
      (_eosio_f32_promote,    double(float)         )
      (_eosio_f64_demote,     float(double)         )
      (_eosio_f32_trunc_i32s, int(float)            )
      (_eosio_f64_trunc_i32s, int(double)           )
      (_eosio_f32_trunc_i32u, int(float)            )
      (_eosio_f64_trunc_i32u, int(double)           )
      (_eosio_f32_trunc_i64s, int64_t(float)        )
      (_eosio_f64_trunc_i64s, int64_t(double)       )
      (_eosio_f32_trunc_i64u, int64_t(float)        )
      (_eosio_f64_trunc_i64u, int64_t(double)       )
      (_eosio_i32_to_f32,     float(int32_t)        )
      (_eosio_i64_to_f32,     float(int64_t)        )
      (_eosio_ui32_to_f32,    float(int32_t)        )
      (_eosio_ui64_to_f32,    float(int64_t)        )
      (_eosio_i32_to_f64,     double(int32_t)       )
      (_eosio_i64_to_f64,     double(int64_t)       )
      (_eosio_ui32_to_f64,    double(int32_t)       )
      (_eosio_ui64_to_f64,    double(int64_t)       )
);

std::istream& operator>>(std::istream& in, wasm_interface::vm_type& runtime) {
   std::string s;
   in >> s;
   if (s == "wavm")
      runtime = eosio::chain::wasm_interface::vm_type::wavm;
   else if (s == "binaryen")
      runtime = eosio::chain::wasm_interface::vm_type::binaryen;
   else
      in.setstate(std::ios_base::failbit);
   return in;
}

} } /// eosio::chain
