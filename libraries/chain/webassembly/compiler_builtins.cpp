#include <eosio/chain/webassembly/interface.hpp>

#include <compiler_builtins.hpp>
#include <softfloat.hpp>

#include <fc/uint128.hpp>

namespace eosio { namespace chain { namespace webassembly {

   void interface::__ashlti3(legacy_ptr<__int128> ret, uint64_t low, uint64_t high, uint32_t shift) const {
      fc::uint128 i(high, low);
      i <<= shift;
      ret.store((unsigned __int128)i);
   }

   void interface::__ashrti3(legacy_ptr<__int128> ret, uint64_t low, uint64_t high, uint32_t shift) const {
      // retain the signedness
      __int128 tmp;
      tmp = high;
      tmp <<= 64;
      tmp |= low;
      tmp >>= shift;
      ret.store(tmp);
   }

   void interface::__lshlti3(legacy_ptr<__int128> ret, uint64_t low, uint64_t high, uint32_t shift) const {
      fc::uint128 i(high, low);
      i <<= shift;
      ret.store( (unsigned __int128)i );
   }

   void interface::__lshrti3(legacy_ptr<__int128> ret, uint64_t low, uint64_t high, uint32_t shift) const {
      fc::uint128 i(high, low);
      i >>= shift;
      ret.store((unsigned __int128)i);
   }

   void interface::__divti3(legacy_ptr<__int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) const {
      __int128 lhs = ha;
      __int128 rhs = hb;

      lhs <<= 64;
      lhs |=  la;

      rhs <<= 64;
      rhs |=  lb;

      EOS_ASSERT(rhs != 0, arithmetic_exception, "divide by zero");

      lhs /= rhs;

      ret.store( lhs );
   }

   void interface::__udivti3(legacy_ptr<unsigned __int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) const {
      unsigned __int128 lhs = ha;
      unsigned __int128 rhs = hb;

      lhs <<= 64;
      lhs |=  la;

      rhs <<= 64;
      rhs |=  lb;

      EOS_ASSERT(rhs != 0, arithmetic_exception, "divide by zero");

      lhs /= rhs;
      ret.store(lhs);
   }

   void interface::__multi3(legacy_ptr<__int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) const {
      __int128 lhs = ha;
      __int128 rhs = hb;

      lhs <<= 64;
      lhs |=  la;

      rhs <<= 64;
      rhs |=  lb;

      lhs *= rhs;
      ret.store(lhs);
   }

   void interface::__modti3(legacy_ptr<__int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) const {
      __int128 lhs = ha;
      __int128 rhs = hb;

      lhs <<= 64;
      lhs |=  la;

      rhs <<= 64;
      rhs |=  lb;

      EOS_ASSERT(rhs != 0, arithmetic_exception, "divide by zero");

      lhs %= rhs;
      ret.store(lhs);
   }

   void interface::__umodti3(legacy_ptr<unsigned __int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) const {
      unsigned __int128 lhs = ha;
      unsigned __int128 rhs = hb;

      lhs <<= 64;
      lhs |=  la;

      rhs <<= 64;
      rhs |=  lb;

      EOS_ASSERT(rhs != 0, arithmetic_exception, "divide by zero");

      lhs %= rhs;
      ret.store(lhs);
   }

   // arithmetic long double
   void interface::__addtf3( legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      float128_t a = {{ la, ha }};
      float128_t b = {{ lb, hb }};
      ret.store(f128_add( a, b ));
   }
   void interface::__subtf3( legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      float128_t a = {{ la, ha }};
      float128_t b = {{ lb, hb }};
      ret.store(f128_sub( a, b ));
   }
   void interface::__multf3( legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      float128_t a = {{ la, ha }};
      float128_t b = {{ lb, hb }};
      ret.store(f128_mul( a, b ));
   }
   void interface::__divtf3( legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      float128_t a = {{ la, ha }};
      float128_t b = {{ lb, hb }};
      ret.store(f128_div( a, b ));
   }
   void interface::__negtf2( legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha ) const {
      ret.store({{ la, (ha ^ (uint64_t)1 << 63) }});
   }

   // conversion long double
   void interface::__extendsftf2( legacy_ptr<float128_t> ret, float f ) const {
      ret.store(f32_to_f128( to_softfloat32(f) ));
   }
   void interface::__extenddftf2( legacy_ptr<float128_t> ret, double d ) const {
      ret.store( f64_to_f128( to_softfloat64(d) ));
   }
   double interface::__trunctfdf2( uint64_t l, uint64_t h ) const {
      float128_t f = {{ l, h }};
      return from_softfloat64(f128_to_f64( f ));
   }
   float interface::__trunctfsf2( uint64_t l, uint64_t h ) const {
      float128_t f = {{ l, h }};
      return from_softfloat32(f128_to_f32( f ));
   }
   int32_t interface::__fixtfsi( uint64_t l, uint64_t h ) const {
      float128_t f = {{ l, h }};
      return f128_to_i32( f, 0, false );
   }
   int64_t interface::__fixtfdi( uint64_t l, uint64_t h ) const {
      float128_t f = {{ l, h }};
      return f128_to_i64( f, 0, false );
   }
   void interface::__fixtfti( legacy_ptr<__int128> ret, uint64_t l, uint64_t h ) const {
      float128_t f = {{ l, h }};
      ret.store(___fixtfti( f ));
   }
   uint32_t interface::__fixunstfsi( uint64_t l, uint64_t h ) const {
      float128_t f = {{ l, h }};
      return f128_to_ui32( f, 0, false );
   }
   uint64_t interface::__fixunstfdi( uint64_t l, uint64_t h ) const {
      float128_t f = {{ l, h }};
      return f128_to_ui64( f, 0, false );
   }
   void interface::__fixunstfti( legacy_ptr<unsigned __int128> ret, uint64_t l, uint64_t h ) const {
      float128_t f = {{ l, h }};
      ret.store( ___fixunstfti( f ));
   }
   void interface::__fixsfti( legacy_ptr<__int128> ret, float a ) const {
      ret.store(___fixsfti( to_softfloat32(a).v ));
   }
   void interface::__fixdfti( legacy_ptr<__int128> ret, double a ) const {
      ret.store( ___fixdfti( to_softfloat64(a).v ));
   }
   void interface::__fixunssfti( legacy_ptr<unsigned __int128> ret, float a ) const {
      ret.store( ___fixunssfti( to_softfloat32(a).v ));
   }
   void interface::__fixunsdfti( legacy_ptr<unsigned __int128> ret, double a ) const {
      ret.store( ___fixunsdfti( to_softfloat64(a).v ));
   }
   double interface::__floatsidf( int32_t i ) const {
      return from_softfloat64(i32_to_f64(i));
   }
   void interface::__floatsitf( legacy_ptr<float128_t> ret, int32_t i ) const {
      ret.store(i32_to_f128(i));
   }
   void interface::__floatditf( legacy_ptr<float128_t> ret, uint64_t a ) const {
      ret.store(i64_to_f128( a ));
   }
   void interface::__floatunsitf( legacy_ptr<float128_t> ret, uint32_t i ) const {
      ret.store(ui32_to_f128(i));
   }
   void interface::__floatunditf( legacy_ptr<float128_t> ret, uint64_t a ) const {
      ret.store(ui64_to_f128( a ));
   }
   double interface::__floattidf( uint64_t l, uint64_t h ) const {
      fc::uint128 v(h, l);
      unsigned __int128 val = (unsigned __int128)v;
      return ___floattidf( *(__int128*)&val );
   }
   double interface::__floatuntidf( uint64_t l, uint64_t h ) const {
      fc::uint128 v(h, l);
      return ___floatuntidf( (unsigned __int128)v );
   }

   inline static int cmptf2_impl( const interface& i, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb, int return_value_if_nan ) {
      float128_t a = {{ la, ha }};
      float128_t b = {{ lb, hb }};
      if ( i.__unordtf2(la, ha, lb, hb) )
         return return_value_if_nan;
      if ( f128_lt( a, b ) )
         return -1;
      if ( f128_eq( a, b ) )
         return 0;
      return 1;
   }
   int interface::__eqtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      return cmptf2_impl(*this, la, ha, lb, hb, 1);
   }
   int interface::__netf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      return cmptf2_impl(*this, la, ha, lb, hb, 1);
   }
   int interface::__getf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      return cmptf2_impl(*this, la, ha, lb, hb, -1);
   }
   int interface::__gttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      return cmptf2_impl(*this, la, ha, lb, hb, 0);
   }
   int interface::__letf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      return cmptf2_impl(*this, la, ha, lb, hb, 1);
   }
   int interface::__lttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      return cmptf2_impl(*this, la, ha, lb, hb, 0);
   }
   int interface::__cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      return cmptf2_impl(*this, la, ha, lb, hb, 1);
   }
   int interface::__unordtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) const {
      float128_t a = {{ la, ha }};
      float128_t b = {{ lb, hb }};
      if ( f128_is_nan(a) || f128_is_nan(b) )
         return 1;
      return 0;
   }
}}} // ns eosio::chain::webassembly
