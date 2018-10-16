#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wasm_interface_private.hpp>
#include <eosio/chain/wasm_eosio_validation.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/account_object.hpp>
#include <fc/exception/exception.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/io/raw.hpp>

#include <softfloat.hpp>
#include <compiler_builtins.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <fstream>

namespace eosio { namespace chain {
   using namespace webassembly;
   using namespace webassembly::common;

   wasm_interface::wasm_interface(vm_type vm) : my( new wasm_interface_impl(vm) ) {}

   wasm_interface::~wasm_interface() {}

   void wasm_interface::validate(const controller& control, const bytes& code) {
      Module module;
      try {
         Serialization::MemoryInputStream stream((U8*)code.data(), code.size());
         WASM::serialize(stream, module);
      } catch(const Serialization::FatalSerializationException& e) {
         EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
      } catch(const IR::ValidationException& e) {
         EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
      }

      wasm_validations::wasm_binary_validation validator(control, module);
      validator.validate();

      root_resolver resolver(true);
      LinkResult link_result = linkModule(module, resolver);

      //there are a couple opportunties for improvement here--
      //Easy: Cache the Module created here so it can be reused for instantiaion
      //Hard: Kick off instantiation in a separate thread at this location
	 }

   void wasm_interface::apply( const digest_type& code_id, const shared_string& code, apply_context& context ) {
      my->get_instantiated_module(code_id, code, context.trx_context)->apply(context);
   }

   void wasm_interface::exit() {
      my->runtime_interface->immediately_exit_currently_running_module();
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
            EOS_ASSERT( context_free, unaccessible_api, "only context free api's can be used in this context" );
         context.used_context_free_api |= !context_free;
      }

      void checktime() {
         context.trx_context.checktime();
      }

   protected:
      apply_context&             context;

};

class context_free_api : public context_aware_api {
   public:
      context_free_api( apply_context& ctx )
      :context_aware_api(ctx, true) {
         /* the context_free_data is not available during normal application because it is prunable */
         EOS_ASSERT( context.context_free, unaccessible_api, "this API may only be called from context_free apply" );
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
         EOS_ASSERT( context.privileged, unaccessible_api, "${code} does not have permission to call this API", ("code",context.receiver) );
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

      /**
       *  This should schedule the feature to be activated once the
       *  block that includes this call is irreversible. It should
       *  fail if the feature is already pending.
       *
       *  Feature name should be base32 encoded name.
       */
      void activate_feature( int64_t feature_name ) {
         EOS_ASSERT( false, unsupported_feature, "Unsupported Hardfork Detected" );
      }

      /**
       * update the resource limits associated with an account.  Note these new values will not take effect until the
       * next resource "tick" which is currently defined as a cycle boundary inside a block.
       *
       * @param account - the account whose limits are being modified
       * @param ram_bytes - the limit for ram bytes
       * @param net_weight - the weight for determining share of network capacity
       * @param cpu_weight - the weight for determining share of compute capacity
       */
      void set_resource_limits( account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight) {
         EOS_ASSERT(ram_bytes >= -1, wasm_execution_error, "invalid value for ram resource limit expected [-1,INT64_MAX]");
         EOS_ASSERT(net_weight >= -1, wasm_execution_error, "invalid value for net resource weight expected [-1,INT64_MAX]");
         EOS_ASSERT(cpu_weight >= -1, wasm_execution_error, "invalid value for cpu resource weight expected [-1,INT64_MAX]");
         if( context.control.get_mutable_resource_limits_manager().set_account_limits(account, ram_bytes, net_weight, cpu_weight) ) {
            context.trx_context.validate_ram_usage.insert( account );
         }
      }

      void get_resource_limits( account_name account, int64_t& ram_bytes, int64_t& net_weight, int64_t& cpu_weight ) {
         context.control.get_resource_limits_manager().get_account_limits( account, ram_bytes, net_weight, cpu_weight);
      }

      int64_t set_proposed_producers( array_ptr<char> packed_producer_schedule, size_t datalen) {
         datastream<const char*> ds( packed_producer_schedule, datalen );
         vector<producer_key> producers;
         fc::raw::unpack(ds, producers);
         EOS_ASSERT(producers.size() <= config::max_producers, wasm_execution_error, "Producer schedule exceeds the maximum producer count for this chain");
         // check that producers are unique
         std::set<account_name> unique_producers;
         for (const auto& p: producers) {
            EOS_ASSERT( context.is_account(p.producer_name), wasm_execution_error, "producer schedule includes a nonexisting account" );
            EOS_ASSERT( p.block_signing_key.valid(), wasm_execution_error, "producer schedule includes an invalid key" );
            unique_producers.insert(p.producer_name);
         }
         EOS_ASSERT( producers.size() == unique_producers.size(), wasm_execution_error, "duplicate producer name in producer schedule" );
         return context.control.set_proposed_producers( std::move(producers) );
      }

      uint32_t get_blockchain_parameters_packed( array_ptr<char> packed_blockchain_parameters, size_t buffer_size) {
         auto& gpo = context.control.get_global_properties();

         auto s = fc::raw::pack_size( gpo.configuration );
         if( buffer_size == 0 ) return s;

         if ( s <= buffer_size ) {
            datastream<char*> ds( packed_blockchain_parameters, s );
            fc::raw::pack(ds, gpo.configuration);
            return s;
         }
         return 0;
      }

      void set_blockchain_parameters_packed( array_ptr<char> packed_blockchain_parameters, size_t datalen) {
         datastream<const char*> ds( packed_blockchain_parameters, datalen );
         chain::chain_config cfg;
         fc::raw::unpack(ds, cfg);
         cfg.validate();
         context.db.modify( context.control.get_global_properties(),
            [&]( auto& gprops ) {
                 gprops.configuration = cfg;
         });
      }

      bool is_privileged( account_name n )const {
         return context.db.get<account_object, by_name>( n ).privileged;
      }

      void set_privileged( account_name n, bool is_priv ) {
         const auto& a = context.db.get<account_object, by_name>( n );
         context.db.modify( a, [&]( auto& ma ){
            ma.privileged = is_priv;
         });
      }

};

class softfloat_api : public context_aware_api {
   public:
      // TODO add traps on truncations for special cases (NaN or outside the range which rounds to an integer)
      softfloat_api( apply_context& ctx )
      :context_aware_api(ctx, true) {}

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
         if (_eosio_f32_ge(af, 2147483648.0f) || _eosio_f32_lt(af, -2147483648.0f))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 overflow" );

         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i32 unrepresentable");
         return f32_to_i32( to_softfloat32(_eosio_f32_trunc( af )), 0, false );
      }
      int32_t _eosio_f64_trunc_i32s( double af ) {
         float64_t a = to_softfloat64(af);
         if (_eosio_f64_ge(af, 2147483648.0) || _eosio_f64_lt(af, -2147483648.0))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_s/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_s/i32 unrepresentable");
         return f64_to_i32( to_softfloat64(_eosio_f64_trunc( af )), 0, false );
      }
      uint32_t _eosio_f32_trunc_i32u( float af ) {
         float32_t a = to_softfloat32(af);
         if (_eosio_f32_ge(af, 4294967296.0f) || _eosio_f32_le(af, -1.0f))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_u/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_u/i32 unrepresentable");
         return f32_to_ui32( to_softfloat32(_eosio_f32_trunc( af )), 0, false );
      }
      uint32_t _eosio_f64_trunc_i32u( double af ) {
         float64_t a = to_softfloat64(af);
         if (_eosio_f64_ge(af, 4294967296.0) || _eosio_f64_le(af, -1.0))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_u/i32 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_u/i32 unrepresentable");
         return f64_to_ui32( to_softfloat64(_eosio_f64_trunc( af )), 0, false );
      }
      int64_t _eosio_f32_trunc_i64s( float af ) {
         float32_t a = to_softfloat32(af);
         if (_eosio_f32_ge(af, 9223372036854775808.0f) || _eosio_f32_lt(af, -9223372036854775808.0f))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i64 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_s/i64 unrepresentable");
         return f32_to_i64( to_softfloat32(_eosio_f32_trunc( af )), 0, false );
      }
      int64_t _eosio_f64_trunc_i64s( double af ) {
         float64_t a = to_softfloat64(af);
         if (_eosio_f64_ge(af, 9223372036854775808.0) || _eosio_f64_lt(af, -9223372036854775808.0))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_s/i64 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_s/i64 unrepresentable");

         return f64_to_i64( to_softfloat64(_eosio_f64_trunc( af )), 0, false );
      }
      uint64_t _eosio_f32_trunc_i64u( float af ) {
         float32_t a = to_softfloat32(af);
         if (_eosio_f32_ge(af, 18446744073709551616.0f) || _eosio_f32_le(af, -1.0f))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_u/i64 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f32.convert_u/i64 unrepresentable");
         return f32_to_ui64( to_softfloat32(_eosio_f32_trunc( af )), 0, false );
      }
      uint64_t _eosio_f64_trunc_i64u( double af ) {
         float64_t a = to_softfloat64(af);
         if (_eosio_f64_ge(af, 18446744073709551616.0) || _eosio_f64_le(af, -1.0))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_u/i64 overflow");
         if (is_nan(a))
            FC_THROW_EXCEPTION( eosio::chain::wasm_execution_error, "Error, f64.convert_u/i64 unrepresentable");
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

      static bool is_nan( const float32_t f ) {
         return ((f.v & 0x7FFFFFFF) > 0x7F800000);
      }
      static bool is_nan( const float64_t f ) {
         return ((f.v & 0x7FFFFFFFFFFFFFFF) > 0x7FF0000000000000);
      }
      static bool is_nan( const float128_t& f ) {
         return (((~(f.v[1]) & uint64_t( 0x7FFF000000000000 )) == 0) && (f.v[0] || ((f.v[1]) & uint64_t( 0x0000FFFFFFFFFFFF ))));
      }
      static float32_t to_softfloat32( float f ) {
         return *reinterpret_cast<float32_t*>(&f);
      }
      static float64_t to_softfloat64( double d ) {
         return *reinterpret_cast<float64_t*>(&d);
      }
      static float from_softfloat32( float32_t f ) {
         return *reinterpret_cast<float*>(&f);
      }
      static double from_softfloat64( float64_t d ) {
         return *reinterpret_cast<double*>(&d);
      }
      static constexpr uint32_t inv_float_eps = 0x4B000000;
      static constexpr uint64_t inv_double_eps = 0x4330000000000000;

      static bool sign_bit( float32_t f ) { return f.v >> 31; }
      static bool sign_bit( float64_t f ) { return f.v >> 63; }

};

class producer_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      int get_active_producers(array_ptr<chain::account_name> producers, size_t buffer_size) {
         auto active_producers = context.get_active_producers();

         size_t len = active_producers.size();
         auto s = len * sizeof(chain::account_name);
         if( buffer_size == 0 ) return s;

         auto copy_size = std::min( buffer_size, s );
         memcpy( producers, active_producers.data(), copy_size );

         return copy_size;
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
         EOS_ASSERT( check == p, crypto_api_exception, "Error expected key different than recovered key" );
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

      template<class Encoder> auto encode(char* data, size_t datalen) {
         Encoder e;
         const size_t bs = eosio::chain::config::hashing_checktime_block_size;
         while ( datalen > bs ) {
            e.write( data, bs );
            data += bs;
            datalen -= bs;
            context.trx_context.checktime();
         }
         e.write( data, datalen );
         return e.result();
      }

      void assert_sha256(array_ptr<char> data, size_t datalen, const fc::sha256& hash_val) {
         auto result = encode<fc::sha256::encoder>( data, datalen );
         EOS_ASSERT( result == hash_val, crypto_api_exception, "hash mismatch" );
      }

      void assert_sha1(array_ptr<char> data, size_t datalen, const fc::sha1& hash_val) {
         auto result = encode<fc::sha1::encoder>( data, datalen );
         EOS_ASSERT( result == hash_val, crypto_api_exception, "hash mismatch" );
      }

      void assert_sha512(array_ptr<char> data, size_t datalen, const fc::sha512& hash_val) {
         auto result = encode<fc::sha512::encoder>( data, datalen );
         EOS_ASSERT( result == hash_val, crypto_api_exception, "hash mismatch" );
      }

      void assert_ripemd160(array_ptr<char> data, size_t datalen, const fc::ripemd160& hash_val) {
         auto result = encode<fc::ripemd160::encoder>( data, datalen );
         EOS_ASSERT( result == hash_val, crypto_api_exception, "hash mismatch" );
      }

      void sha1(array_ptr<char> data, size_t datalen, fc::sha1& hash_val) {
         hash_val = encode<fc::sha1::encoder>( data, datalen );
      }

      void sha256(array_ptr<char> data, size_t datalen, fc::sha256& hash_val) {
         hash_val = encode<fc::sha256::encoder>( data, datalen );
      }

      void sha512(array_ptr<char> data, size_t datalen, fc::sha512& hash_val) {
         hash_val = encode<fc::sha512::encoder>( data, datalen );
      }

      void ripemd160(array_ptr<char> data, size_t datalen, fc::ripemd160& hash_val) {
         hash_val = encode<fc::ripemd160::encoder>( data, datalen );
      }
};

class permission_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      bool check_transaction_authorization( array_ptr<char> trx_data,     size_t trx_size,
                                            array_ptr<char> pubkeys_data, size_t pubkeys_size,
                                            array_ptr<char> perms_data,   size_t perms_size
                                          )
      {
         transaction trx = fc::raw::unpack<transaction>( trx_data, trx_size );

         flat_set<public_key_type> provided_keys;
         unpack_provided_keys( provided_keys, pubkeys_data, pubkeys_size );

         flat_set<permission_level> provided_permissions;
         unpack_provided_permissions( provided_permissions, perms_data, perms_size );

         try {
            context.control
                   .get_authorization_manager()
                   .check_authorization( trx.actions,
                                         provided_keys,
                                         provided_permissions,
                                         fc::seconds(trx.delay_sec),
                                         std::bind(&transaction_context::checktime, &context.trx_context),
                                         false
                                       );
            return true;
         } catch( const authorization_exception& e ) {}

         return false;
      }

      bool check_permission_authorization( account_name account, permission_name permission,
                                           array_ptr<char> pubkeys_data, size_t pubkeys_size,
                                           array_ptr<char> perms_data,   size_t perms_size,
                                           uint64_t delay_us
                                         )
      {
         EOS_ASSERT( delay_us <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()),
                     action_validate_exception, "provided delay is too large" );

         flat_set<public_key_type> provided_keys;
         unpack_provided_keys( provided_keys, pubkeys_data, pubkeys_size );

         flat_set<permission_level> provided_permissions;
         unpack_provided_permissions( provided_permissions, perms_data, perms_size );

         try {
            context.control
                   .get_authorization_manager()
                   .check_authorization( account,
                                         permission,
                                         provided_keys,
                                         provided_permissions,
                                         fc::microseconds(delay_us),
                                         std::bind(&transaction_context::checktime, &context.trx_context),
                                         false
                                       );
            return true;
         } catch( const authorization_exception& e ) {}

         return false;
      }

      int64_t get_permission_last_used( account_name account, permission_name permission ) {
         const auto& am = context.control.get_authorization_manager();
         return am.get_permission_last_used( am.get_permission({account, permission}) ).time_since_epoch().count();
      };

      int64_t get_account_creation_time( account_name account ) {
         auto* acct = context.db.find<account_object, by_name>(account);
         EOS_ASSERT( acct != nullptr, action_validate_exception,
                     "account '${account}' does not exist", ("account", account) );
         return time_point(acct->creation_date).time_since_epoch().count();
      }

   private:
      void unpack_provided_keys( flat_set<public_key_type>& keys, const char* pubkeys_data, size_t pubkeys_size ) {
         keys.clear();
         if( pubkeys_size == 0 ) return;

         keys = fc::raw::unpack<flat_set<public_key_type>>( pubkeys_data, pubkeys_size );
      }

      void unpack_provided_permissions( flat_set<permission_level>& permissions, const char* perms_data, size_t perms_size ) {
         permissions.clear();
         if( perms_size == 0 ) return;

         permissions = fc::raw::unpack<flat_set<permission_level>>( perms_data, perms_size );
      }

};

class authorization_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

   void require_authorization( const account_name& account ) {
      context.require_authorization( account );
   }

   bool has_authorization( const account_name& account )const {
      return context.has_authorization( account );
   }

   void require_authorization(const account_name& account,
                                                 const permission_name& permission) {
      context.require_authorization( account, permission );
   }

   void require_recipient( account_name recipient ) {
      context.require_recipient( recipient );
   }

   bool is_account( const account_name& account )const {
      return context.is_account( account );
   }

};

class system_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      uint64_t current_time() {
         return static_cast<uint64_t>( context.control.pending_block_time().time_since_epoch().count() );
      }

      uint64_t publication_time() {
         return static_cast<uint64_t>( context.trx_context.published.time_since_epoch().count() );
      }

};

class context_free_system_api :  public context_aware_api {
public:
   explicit context_free_system_api( apply_context& ctx )
   :context_aware_api(ctx,true){}

   void abort() {
      edump(("abort() called"));
      EOS_ASSERT( false, abort_called, "abort() called");
   }

   // Kept as intrinsic rather than implementing on WASM side (using eosio_assert_message and strlen) because strlen is faster on native side.
   void eosio_assert( bool condition, null_terminated_ptr msg ) {
      if( BOOST_UNLIKELY( !condition ) ) {
         std::string message( msg );
         edump((message));
         EOS_THROW( eosio_assert_message_exception, "assertion failure with message: ${s}", ("s",message) );
      }
   }

   void eosio_assert_message( bool condition, array_ptr<const char> msg, size_t msg_len ) {
      if( BOOST_UNLIKELY( !condition ) ) {
         std::string message( msg, msg_len );
         edump((message));
         EOS_THROW( eosio_assert_message_exception, "assertion failure with message: ${s}", ("s",message) );
      }
   }

   void eosio_assert_code( bool condition, uint64_t error_code ) {
      if( BOOST_UNLIKELY( !condition ) ) {
         edump((error_code));
         EOS_THROW( eosio_assert_code_exception,
                    "assertion failure with error code: ${error_code}", ("error_code", error_code) );
      }
   }

   void eosio_exit(int32_t code) {
      context.control.get_wasm_interface().exit();
   }

};

class action_api : public context_aware_api {
   public:
   action_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

      int read_action_data(array_ptr<char> memory, size_t buffer_size) {
         auto s = context.act.data.size();
         if( buffer_size == 0 ) return s;

         auto copy_size = std::min( buffer_size, s );
         memcpy( memory, context.act.data.data(), copy_size );

         return copy_size;
      }

      int action_data_size() {
         return context.act.data.size();
      }

      name current_receiver() {
         return context.receiver;
      }
};

class console_api : public context_aware_api {
   public:
      console_api( apply_context& ctx )
      : context_aware_api(ctx,true)
      , ignore(!ctx.control.contracts_console()) {}

      // Kept as intrinsic rather than implementing on WASM side (using prints_l and strlen) because strlen is faster on native side.
      void prints(null_terminated_ptr str) {
         if ( !ignore ) {
            context.console_append<const char*>(str);
         }
      }

      void prints_l(array_ptr<const char> str, size_t str_len ) {
         if ( !ignore ) {
            context.console_append(string(str, str_len));
         }
      }

      void printi(int64_t val) {
         if ( !ignore ) {
            context.console_append(val);
         }
      }

      void printui(uint64_t val) {
         if ( !ignore ) {
            context.console_append(val);
         }
      }

      void printi128(const __int128& val) {
         if ( !ignore ) {
            bool is_negative = (val < 0);
            unsigned __int128 val_magnitude;

            if( is_negative )
               val_magnitude = static_cast<unsigned __int128>(-val); // Works even if val is at the lowest possible value of a int128_t
            else
               val_magnitude = static_cast<unsigned __int128>(val);

            fc::uint128_t v(val_magnitude>>64, static_cast<uint64_t>(val_magnitude) );

            if( is_negative ) {
               context.console_append("-");
            }

            context.console_append(fc::variant(v).get_string());
         }
      }

      void printui128(const unsigned __int128& val) {
         if ( !ignore ) {
            fc::uint128_t v(val>>64, static_cast<uint64_t>(val) );
            context.console_append(fc::variant(v).get_string());
         }
      }

      void printsf( float val ) {
         if ( !ignore ) {
            // Assumes float representation on native side is the same as on the WASM side
            auto& console = context.get_console_stream();
            auto orig_prec = console.precision();

            console.precision( std::numeric_limits<float>::digits10 );
            context.console_append(val);

            console.precision( orig_prec );
         }
      }

      void printdf( double val ) {
         if ( !ignore ) {
            // Assumes double representation on native side is the same as on the WASM side
            auto& console = context.get_console_stream();
            auto orig_prec = console.precision();

            console.precision( std::numeric_limits<double>::digits10 );
            context.console_append(val);

            console.precision( orig_prec );
         }
      }

      void printqf( const float128_t& val ) {
         /*
          * Native-side long double uses an 80-bit extended-precision floating-point number.
          * The easiest solution for now was to use the Berkeley softfloat library to round the 128-bit
          * quadruple-precision floating-point number to an 80-bit extended-precision floating-point number
          * (losing precision) which then allows us to simply cast it into a long double for printing purposes.
          *
          * Later we might find a better solution to print the full quadruple-precision floating-point number.
          * Maybe with some compilation flag that turns long double into a quadruple-precision floating-point number,
          * or maybe with some library that allows us to print out quadruple-precision floating-point numbers without
          * having to deal with long doubles at all.
          */

         if ( !ignore ) {
            auto& console = context.get_console_stream();
            auto orig_prec = console.precision();

            console.precision( std::numeric_limits<long double>::digits10 );

            extFloat80_t val_approx;
            f128M_to_extF80M(&val, &val_approx);
            context.console_append( *(long double*)(&val_approx) );

            console.precision( orig_prec );
         }
      }

      void printn(const name& value) {
         if ( !ignore ) {
            context.console_append(value.to_string());
         }
      }

      void printhex(array_ptr<const char> data, size_t data_len ) {
         if ( !ignore ) {
            context.console_append(fc::to_hex(data, data_len));
         }
      }

   private:
      bool ignore;
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
         EOS_ASSERT( data_len == ARR_SIZE,\
                    db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.store(scope, table, payer, id, data.value);\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, array_ptr<const ARR_ELEMENT_TYPE> data, size_t data_len ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
                    db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.update(iterator, payer, data.value);\
      }\
      void db_##IDX##_remove( int iterator ) {\
         return context.IDX.remove(iterator);\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, array_ptr<const ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t& primary ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
                    db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.find_secondary(code, scope, table, data, primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, array_ptr<ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t primary ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
                    db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.find_primary(code, scope, table, data.value, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table, array_ptr<ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t& primary ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
                    db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.lowerbound_secondary(code, scope, table, data.value, primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table, array_ptr<ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t& primary ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
                    db_api_exception,\
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

#define DB_API_METHOD_WRAPPERS_FLOAT_SECONDARY(IDX, TYPE)\
      int db_##IDX##_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE& secondary ) {\
         EOS_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return context.IDX.store( scope, table, payer, id, secondary );\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, const TYPE& secondary ) {\
         EOS_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return context.IDX.update( iterator, payer, secondary );\
      }\
      void db_##IDX##_remove( int iterator ) {\
         return context.IDX.remove( iterator );\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE& secondary, uint64_t& primary ) {\
         EOS_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return context.IDX.find_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t primary ) {\
         return context.IDX.find_primary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE& secondary, uint64_t& primary ) {\
         EOS_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return context.IDX.lowerbound_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE& secondary, uint64_t& primary ) {\
         EOS_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
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
      DB_API_METHOD_WRAPPERS_FLOAT_SECONDARY(idx_double, float64_t)
      DB_API_METHOD_WRAPPERS_FLOAT_SECONDARY(idx_long_double, float128_t)
};

class memory_api : public context_aware_api {
   public:
      memory_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

      char* memcpy( array_ptr<char> dest, array_ptr<const char> src, size_t length) {
         EOS_ASSERT((std::abs((ptrdiff_t)dest.value - (ptrdiff_t)src.value)) >= length,
               overlapping_memory_error, "memcpy can only accept non-aliasing pointers");
         return (char *)::memcpy(dest, src, length);
      }

      char* memmove( array_ptr<char> dest, array_ptr<const char> src, size_t length) {
         return (char *)::memmove(dest, src, length);
      }

      int memcmp( array_ptr<const char> dest, array_ptr<const char> src, size_t length) {
         int ret = ::memcmp(dest, src, length);
         if(ret < 0)
            return -1;
         if(ret > 0)
            return 1;
         return 0;
      }

      char* memset( array_ptr<char> dest, int value, size_t length ) {
         return (char *)::memset( dest, value, length );
      }
};

class transaction_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      void send_inline( array_ptr<char> data, size_t data_len ) {
         //TODO: Why is this limit even needed? And why is it not consistently checked on actions in input or deferred transactions
         EOS_ASSERT( data_len < context.control.get_global_properties().configuration.max_inline_action_size, inline_action_too_big,
                    "inline action too big" );

         action act;
         fc::raw::unpack<action>(data, data_len, act);
         context.execute_inline(std::move(act));
      }

      void send_context_free_inline( array_ptr<char> data, size_t data_len ) {
         //TODO: Why is this limit even needed? And why is it not consistently checked on actions in input or deferred transactions
         EOS_ASSERT( data_len < context.control.get_global_properties().configuration.max_inline_action_size, inline_action_too_big,
                   "inline action too big" );

         action act;
         fc::raw::unpack<action>(data, data_len, act);
         context.execute_context_free_inline(std::move(act));
      }

      void send_deferred( const uint128_t& sender_id, account_name payer, array_ptr<char> data, size_t data_len, uint32_t replace_existing) {
         try {
            transaction trx;
            fc::raw::unpack<transaction>(data, data_len, trx);
            context.schedule_deferred_transaction(sender_id, payer, std::move(trx), replace_existing);
         } FC_RETHROW_EXCEPTIONS(warn, "data as hex: ${data}", ("data", fc::to_hex(data, data_len)))
      }

      bool cancel_deferred( const unsigned __int128& val ) {
         fc::uint128_t sender_id(val>>64, uint64_t(val) );
         return context.cancel_deferred_transaction( (unsigned __int128)sender_id );
      }
};


class context_free_transaction_api : public context_aware_api {
   public:
      context_free_transaction_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

      int read_transaction( array_ptr<char> data, size_t buffer_size ) {
         bytes trx = context.get_packed_transaction();

         auto s = trx.size();
         if( buffer_size == 0) return s;

         auto copy_size = std::min( buffer_size, s );
         memcpy( data, trx.data(), copy_size );

         return copy_size;
      }

      int transaction_size() {
         return context.get_packed_transaction().size();
      }

      int expiration() {
        return context.trx_context.trx.expiration.sec_since_epoch();
      }

      int tapos_block_num() {
        return context.trx_context.trx.ref_block_num;
      }
      int tapos_block_prefix() {
        return context.trx_context.trx.ref_block_prefix;
      }

      int get_action( uint32_t type, uint32_t index, array_ptr<char> buffer, size_t buffer_size )const {
         return context.get_action( type, index, buffer, buffer_size );
      }
};

class compiler_builtins : public context_aware_api {
   public:
      compiler_builtins( apply_context& ctx )
      :context_aware_api(ctx,true){}

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

         EOS_ASSERT(rhs != 0, arithmetic_exception, "divide by zero");

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

         EOS_ASSERT(rhs != 0, arithmetic_exception, "divide by zero");

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

         EOS_ASSERT(rhs != 0, arithmetic_exception, "divide by zero");

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

         EOS_ASSERT(rhs != 0, arithmetic_exception, "divide by zero");

         lhs %= rhs;
         ret = lhs;
      }

      // arithmetic long double
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
      void __negtf2( float128_t& ret, uint64_t la, uint64_t ha ) {
         ret = {{ la, (ha ^ (uint64_t)1 << 63) }};
      }

      // conversion long double
      void __extendsftf2( float128_t& ret, float f ) {
         ret = f32_to_f128( softfloat_api::to_softfloat32(f) );
      }
      void __extenddftf2( float128_t& ret, double d ) {
         ret = f64_to_f128( softfloat_api::to_softfloat64(d) );
      }
      double __trunctfdf2( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return softfloat_api::from_softfloat64(f128_to_f64( f ));
      }
      float __trunctfsf2( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return softfloat_api::from_softfloat32(f128_to_f32( f ));
      }
      int32_t __fixtfsi( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return f128_to_i32( f, 0, false );
      }
      int64_t __fixtfdi( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return f128_to_i64( f, 0, false );
      }
      void __fixtfti( __int128& ret, uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         ret = ___fixtfti( f );
      }
      uint32_t __fixunstfsi( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return f128_to_ui32( f, 0, false );
      }
      uint64_t __fixunstfdi( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return f128_to_ui64( f, 0, false );
      }
      void __fixunstfti( unsigned __int128& ret, uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         ret = ___fixunstfti( f );
      }
      void __fixsfti( __int128& ret, float a ) {
         ret = ___fixsfti( softfloat_api::to_softfloat32(a).v );
      }
      void __fixdfti( __int128& ret, double a ) {
         ret = ___fixdfti( softfloat_api::to_softfloat64(a).v );
      }
      void __fixunssfti( unsigned __int128& ret, float a ) {
         ret = ___fixunssfti( softfloat_api::to_softfloat32(a).v );
      }
      void __fixunsdfti( unsigned __int128& ret, double a ) {
         ret = ___fixunsdfti( softfloat_api::to_softfloat64(a).v );
      }
      double __floatsidf( int32_t i ) {
         return softfloat_api::from_softfloat64(i32_to_f64(i));
      }
      void __floatsitf( float128_t& ret, int32_t i ) {
         ret = i32_to_f128(i);
      }
      void __floatditf( float128_t& ret, uint64_t a ) {
         ret = i64_to_f128( a );
      }
      void __floatunsitf( float128_t& ret, uint32_t i ) {
         ret = ui32_to_f128(i);
      }
      void __floatunditf( float128_t& ret, uint64_t a ) {
         ret = ui64_to_f128( a );
      }
      double __floattidf( uint64_t l, uint64_t h ) {
         fc::uint128_t v(h, l);
         unsigned __int128 val = (unsigned __int128)v;
         return ___floattidf( *(__int128*)&val );
      }
      double __floatuntidf( uint64_t l, uint64_t h ) {
         fc::uint128_t v(h, l);
         return ___floatuntidf( (unsigned __int128)v );
      }
      int ___cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb, int return_value_if_nan ) {
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         if ( __unordtf2(la, ha, lb, hb) )
            return return_value_if_nan;
         if ( f128_lt( a, b ) )
            return -1;
         if ( f128_eq( a, b ) )
            return 0;
         return 1;
      }
      int __eqtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 1);
      }
      int __netf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 1);
      }
      int __getf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, -1);
      }
      int __gttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 0);
      }
      int __letf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 1);
      }
      int __lttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 0);
      }
      int __cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 1);
      }
      int __unordtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         if ( softfloat_api::is_nan(a) || softfloat_api::is_nan(b) )
            return 1;
         return 0;
      }

      static constexpr uint32_t SHIFT_WIDTH = (sizeof(uint64_t)*8)-1;
};


/*
 * This api will be removed with fix for `eos #2561`
 */
class call_depth_api : public context_aware_api {
   public:
      call_depth_api( apply_context& ctx )
      :context_aware_api(ctx,true){}
      void call_depth_assert() {
         FC_THROW_EXCEPTION(wasm_execution_error, "Exceeded call depth maximum");
      }
};

REGISTER_INJECTED_INTRINSICS(call_depth_api,
   (call_depth_assert,  void()               )
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
   (__letf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__cmptf2,      int(int64_t, int64_t, int64_t, int64_t)        )
   (__unordtf2,    int(int64_t, int64_t, int64_t, int64_t)        )
   (__negtf2,      void (int, int64_t, int64_t)                   )
   (__floatsitf,   void (int, int)                                )
   (__floatunsitf, void (int, int)                                )
   (__floatditf,   void (int, int64_t)                            )
   (__floatunditf, void (int, int64_t)                            )
   (__floattidf,   double (int64_t, int64_t)                      )
   (__floatuntidf, double (int64_t, int64_t)                      )
   (__floatsidf,   double(int)                                    )
   (__extendsftf2, void(int, float)                               )
   (__extenddftf2, void(int, double)                              )
   (__fixtfti,     void(int, int64_t, int64_t)                    )
   (__fixtfdi,     int64_t(int64_t, int64_t)                      )
   (__fixtfsi,     int(int64_t, int64_t)                          )
   (__fixunstfti,  void(int, int64_t, int64_t)                    )
   (__fixunstfdi,  int64_t(int64_t, int64_t)                      )
   (__fixunstfsi,  int(int64_t, int64_t)                          )
   (__fixsfti,     void(int, float)                               )
   (__fixdfti,     void(int, double)                              )
   (__fixunssfti,  void(int, float)                               )
   (__fixunsdfti,  void(int, double)                              )
   (__trunctfdf2,  double(int64_t, int64_t)                       )
   (__trunctfsf2,  float(int64_t, int64_t)                        )
);

REGISTER_INTRINSICS(privileged_api,
   (is_feature_active,                int(int64_t)                          )
   (activate_feature,                 void(int64_t)                         )
   (get_resource_limits,              void(int64_t,int,int,int)             )
   (set_resource_limits,              void(int64_t,int64_t,int64_t,int64_t) )
   (set_proposed_producers,           int64_t(int,int)                      )
   (get_blockchain_parameters_packed, int(int, int)                         )
   (set_blockchain_parameters_packed, void(int,int)                         )
   (is_privileged,                    int(int64_t)                          )
   (set_privileged,                   void(int64_t, int)                    )
);

REGISTER_INJECTED_INTRINSICS(transaction_context,
   (checktime,      void())
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
   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_long_double)
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


REGISTER_INTRINSICS(permission_api,
   (check_transaction_authorization, int(int, int, int, int, int, int)                  )
   (check_permission_authorization,  int(int64_t, int64_t, int, int, int, int, int64_t) )
   (get_permission_last_used,        int64_t(int64_t, int64_t) )
   (get_account_creation_time,       int64_t(int64_t) )
);


REGISTER_INTRINSICS(system_api,
   (current_time, int64_t()       )
   (publication_time,   int64_t() )
);

REGISTER_INTRINSICS(context_free_system_api,
   (abort,                void()              )
   (eosio_assert,         void(int, int)      )
   (eosio_assert_message, void(int, int, int) )
   (eosio_assert_code,    void(int, int64_t)  )
   (eosio_exit,           void(int)           )
);

REGISTER_INTRINSICS(action_api,
   (read_action_data,       int(int, int)  )
   (action_data_size,       int()          )
   (current_receiver,   int64_t()          )
);

REGISTER_INTRINSICS(authorization_api,
   (require_recipient,     void(int64_t)          )
   (require_authorization, void(int64_t), "require_auth", void(authorization_api::*)(const account_name&) )
   (require_authorization, void(int64_t, int64_t), "require_auth2", void(authorization_api::*)(const account_name&, const permission_name& permission) )
   (has_authorization,     int(int64_t), "has_auth", bool(authorization_api::*)(const account_name&)const )
   (is_account,            int(int64_t)           )
);

REGISTER_INTRINSICS(console_api,
   (prints,                void(int)      )
   (prints_l,              void(int, int) )
   (printi,                void(int64_t)  )
   (printui,               void(int64_t)  )
   (printi128,             void(int)      )
   (printui128,            void(int)      )
   (printsf,               void(float)    )
   (printdf,               void(double)   )
   (printqf,               void(int)      )
   (printn,                void(int64_t)  )
   (printhex,              void(int, int) )
);

REGISTER_INTRINSICS(context_free_transaction_api,
   (read_transaction,       int(int, int)            )
   (transaction_size,       int()                    )
   (expiration,             int()                    )
   (tapos_block_prefix,     int()                    )
   (tapos_block_num,        int()                    )
   (get_action,             int (int, int, int, int) )
);

REGISTER_INTRINSICS(transaction_api,
   (send_inline,               void(int, int)               )
   (send_context_free_inline,  void(int, int)               )
   (send_deferred,             void(int, int64_t, int, int, int32_t) )
   (cancel_deferred,           int(int)                     )
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

REGISTER_INJECTED_INTRINSICS(softfloat_api,
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
   else if (s == "wabt")
      runtime = eosio::chain::wasm_interface::vm_type::wabt;
   else
      in.setstate(std::ios_base::failbit);
   return in;
}

} } /// eosio::chain
