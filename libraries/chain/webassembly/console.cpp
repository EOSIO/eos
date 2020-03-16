#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   // Kept as intrinsic rather than implementing on WASM side (using prints_l and strlen) because strlen is faster on native side.
   // TODO predicate these for ignore
   void interface::prints(null_terminated_ptr str) {
      ctx.console_append( static_cast<const char*>(str) );
   }

   void interface::prints_l(legacy_array_ptr<const char> str, uint32_t str_len ) {
      ctx.console_append(string(str, str_len));
   }

   void interface::printi(int64_t val) {
      std::ostringstream oss;
      oss << val;
      ctx.console_append( oss.str() );
   }

   void interface::printui(uint64_t val) {
      std::ostringstream oss;
      oss << val;
      ctx.console_append( oss.str() );
   }

   void interface::printi128(const __int128& val) {
      bool is_negative = (val < 0);
      unsigned __int128 val_magnitude;

      if( is_negative )
         val_magnitude = static_cast<unsigned __int128>(-val); // Works even if val is at the lowest possible value of a int128_t
      else
         val_magnitude = static_cast<unsigned __int128>(val);

      fc::uint128_t v(val_magnitude>>64, static_cast<uint64_t>(val_magnitude) );

      string s;
      if( is_negative ) {
         s += '-';
      }
      s += fc::variant(v).get_string();

      ctx.console_append( s );
   }

   void interface::printui128(const unsigned __int128& val) {
      fc::uint128_t v(val>>64, static_cast<uint64_t>(val) );
      ctx.console_append(fc::variant(v).get_string());
   }

   void interface::printsf( float val ) {
      // Assumes float representation on native side is the same as on the WASM side
      std::ostringstream oss;
      oss.setf( std::ios::scientific, std::ios::floatfield );
      oss.precision( std::numeric_limits<float>::digits10 );
      oss << val;
      ctx.console_append( oss.str() );
   }

   void interface::printdf( double val ) {
      // Assumes double representation on native side is the same as on the WASM side
      std::ostringstream oss;
      oss.setf( std::ios::scientific, std::ios::floatfield );
      oss.precision( std::numeric_limits<double>::digits10 );
      oss << val;
      ctx.console_append( oss.str() );
   }

   void interface::printqf( const float128_t& val ) {
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

      std::ostringstream oss;
      oss.setf( std::ios::scientific, std::ios::floatfield );

#ifdef __x86_64__
      oss.precision( std::numeric_limits<long double>::digits10 );
      extFloat80_t val_approx;
      f128M_to_extF80M(&val, &val_approx);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
      oss << *(long double*)(&val_approx);
#pragma GCC diagnostic pop
#else
      oss.precision( std::numeric_limits<double>::digits10 );
      double val_approx = from_softfloat64( f128M_to_f64(&val) );
      oss << val_approx;
#endif
      ctx.console_append( oss.str() );
   }

   void interface::printn(name value) {
      ctx.console_append(value.to_string());
   }

   void interface::printhex(legacy_array_ptr<const char> data, uint32_t data_len ) {
      ctx.console_append(fc::to_hex(data, data_len));
   }
}}} // ns eosio::chain::webassembly
