#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <fc/uint128.hpp>

namespace eosio { namespace chain { namespace webassembly {

   template <typename Ctx, typename F>
   inline static void predicated_print(Ctx& context, F&& print_func) {
      if (UNLIKELY(context.control.contracts_console()))
         print_func();
   }

   // Kept as intrinsic rather than implementing on WASM side (using prints_l and strlen) because strlen is faster on native side.
   // TODO predicate these for ignore
   void interface::prints(null_terminated_ptr str) {
      predicated_print(context,
      [&]() { context.console_append( static_cast<const char*>(str.data()) ); });
   }

   void interface::prints_l(legacy_span<const char> str ) {
		predicated_print(context,
      [&]() { context.console_append(std::string_view(str.data(), str.size())); });
   }

   void interface::printi(int64_t val) {
		predicated_print(context,
      [&]() {
			std::ostringstream oss;
			oss << val;
			context.console_append( oss.str() );
      });
   }

   void interface::printui(uint64_t val) {
		predicated_print(context,
      [&]() {
			std::ostringstream oss;
			oss << val;
			context.console_append( oss.str() );
      });
   }

   void interface::printi128(legacy_ptr<const __int128> val) {
		predicated_print(context,
      [&]() {
			bool is_negative = (*val < 0);
			unsigned __int128 val_magnitude;

			if( is_negative )
				val_magnitude = static_cast<unsigned __int128>(-*val); // Works even if val is at the lowest possible value of a int128_t
			else
				val_magnitude = static_cast<unsigned __int128>(*val);

			fc::uint128 v(val_magnitude>>64, static_cast<uint64_t>(val_magnitude) );

			string s;
			if( is_negative ) {
				s += '-';
			}
			s += fc::variant(v).get_string();

			context.console_append( s );
      });
   }

   void interface::printui128(legacy_ptr<const unsigned __int128> val) {
		predicated_print(context,
      [&]() {
			fc::uint128 v(*val>>64, static_cast<uint64_t>(*val) );
			context.console_append(fc::variant(v).get_string());
      });
   }

   void interface::printsf( float32_t val ) {
		predicated_print(context,
      [&]() {
			// Assumes float representation on native side is the same as on the WASM side
			std::ostringstream oss;
			oss.setf( std::ios::scientific, std::ios::floatfield );
			oss.precision( std::numeric_limits<float>::digits10 );
			oss << ::from_softfloat32(val);
			context.console_append( oss.str() );
      });
   }

   void interface::printdf( float64_t val ) {
		predicated_print(context,
      [&]() {
			// Assumes double representation on native side is the same as on the WASM side
			std::ostringstream oss;
			oss.setf( std::ios::scientific, std::ios::floatfield );
			oss.precision( std::numeric_limits<double>::digits10 );
			oss << ::from_softfloat64(val);
			context.console_append( oss.str() );
      });
   }

   void interface::printqf( legacy_ptr<const float128_t> val ) {
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

		predicated_print(context,
      [&]() {
			std::ostringstream oss;
			oss.setf( std::ios::scientific, std::ios::floatfield );

#ifdef __x86_64__
			oss.precision( std::numeric_limits<long double>::digits10 );
			extFloat80_t val_approx;
			f128M_to_extF80M(val.get(), &val_approx);
			long double _val;
			std::memcpy((char*)&_val, (char*)&val_approx, sizeof(long double));
			oss << _val;
#else
			oss.precision( std::numeric_limits<double>::digits10 );
			double val_approx = from_softfloat64( f128M_to_f64(val.get()) );
			oss << val_approx;
#endif
			context.console_append( oss.str() );
      });
   }

   void interface::printn(name value) {
		predicated_print(context, [&]() { context.console_append(value.to_string()); });
   }

   void interface::printhex(legacy_span<const char> data ) {
      predicated_print(context, [&]() { context.console_append(fc::to_hex(data.data(), data.size())); });
   }
}}} // ns eosio::chain::webassembly
