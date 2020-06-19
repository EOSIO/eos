#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>
#include <compiler_builtins.hpp>
#include <softfloat.hpp>

namespace b1::rodeos {

template <typename Derived>
struct compiler_builtins_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void __ashlti3(legacy_ptr<__int128> ret, uint64_t low, uint64_t high, uint32_t shift) {
      if (shift >= 128) {
         *ret = 0;
      } else {
         unsigned __int128 i = high;
         i <<= 64;
         i |= low;
         i <<= shift;
         *ret = (__int128)i;
      }
   }

   void __ashrti3(legacy_ptr<__int128> ret, uint64_t low, uint64_t high, uint32_t shift) {
      // retain the signedness
      *ret = high;
      *ret <<= 64;
      *ret |= low;
      *ret >>= shift;
   }

   void __lshlti3(legacy_ptr<__int128> ret, uint64_t low, uint64_t high, uint32_t shift) {
      if (shift >= 128) {
         *ret = 0;
      } else {
         unsigned __int128 i = high;
         i <<= 64;
         i |= low;
         i <<= shift;
         *ret = (__int128)i;
      }
   }

   void __lshrti3(legacy_ptr<__int128> ret, uint64_t low, uint64_t high, uint32_t shift) {
      unsigned __int128 i = high;
      i <<= 64;
      i |= low;
      i >>= shift;
      *ret = (unsigned __int128)i;
   }

   void __divti3(legacy_ptr<__int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      __int128 lhs = ha;
      __int128 rhs = hb;

      lhs <<= 64;
      lhs |= la;

      rhs <<= 64;
      rhs |= lb;

      if (rhs == 0)
         throw std::runtime_error("divide by zero");

      lhs /= rhs;

      *ret = lhs;
   }

   void __udivti3(legacy_ptr<unsigned __int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      unsigned __int128 lhs = ha;
      unsigned __int128 rhs = hb;

      lhs <<= 64;
      lhs |= la;

      rhs <<= 64;
      rhs |= lb;

      if (rhs == 0)
         throw std::runtime_error("divide by zero");

      lhs /= rhs;
      *ret = lhs;
   }

   void __modti3(legacy_ptr<__int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      __int128 lhs = ha;
      __int128 rhs = hb;

      lhs <<= 64;
      lhs |= la;

      rhs <<= 64;
      rhs |= lb;

      if (rhs == 0)
         throw std::runtime_error("divide by zero");

      lhs %= rhs;
      *ret = lhs;
   }

   void __umodti3(legacy_ptr<unsigned __int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      unsigned __int128 lhs = ha;
      unsigned __int128 rhs = hb;

      lhs <<= 64;
      lhs |= la;

      rhs <<= 64;
      rhs |= lb;

      if (rhs == 0)
         throw std::runtime_error("divide by zero");

      lhs %= rhs;
      *ret = lhs;
   }

   void __multi3(legacy_ptr<__int128> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      __int128 lhs = ha;
      __int128 rhs = hb;

      lhs <<= 64;
      lhs |= la;

      rhs <<= 64;
      rhs |= lb;

      lhs *= rhs;
      *ret = lhs;
   }

   void __addtf3(legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      float128_t a = { { la, ha } };
      float128_t b = { { lb, hb } };
      *ret         = f128_add(a, b);
   }

   void __subtf3(legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      float128_t a = { { la, ha } };
      float128_t b = { { lb, hb } };
      *ret         = f128_sub(a, b);
   }

   void __multf3(legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      float128_t a = { { la, ha } };
      float128_t b = { { lb, hb } };
      *ret         = f128_mul(a, b);
   }

   void __divtf3(legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      float128_t a = { { la, ha } };
      float128_t b = { { lb, hb } };
      *ret         = f128_div(a, b);
   }

   int __unordtf2(uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
      float128_t a = { { la, ha } };
      float128_t b = { { lb, hb } };
      if (f128_is_nan(a) || f128_is_nan(b))
         return 1;
      return 0;
   }

   int ___cmptf2(uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb, int return_value_if_nan) {
      float128_t a = { { la, ha } };
      float128_t b = { { lb, hb } };
      if (__unordtf2(la, ha, lb, hb))
         return return_value_if_nan;
      if (f128_lt(a, b))
         return -1;
      if (f128_eq(a, b))
         return 0;
      return 1;
   }

   int __eqtf2(uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) { return ___cmptf2(la, ha, lb, hb, 1); }

   int __netf2(uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) { return ___cmptf2(la, ha, lb, hb, 1); }

   int __getf2(uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) { return ___cmptf2(la, ha, lb, hb, -1); }

   int __gttf2(uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) { return ___cmptf2(la, ha, lb, hb, 0); }

   int __letf2(uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) { return ___cmptf2(la, ha, lb, hb, 1); }

   int __lttf2(uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) { return ___cmptf2(la, ha, lb, hb, 0); }

   int __cmptf2(uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) { return ___cmptf2(la, ha, lb, hb, 1); }

   void __negtf2(legacy_ptr<float128_t> ret, uint64_t la, uint64_t ha) { *ret = { { la, (ha ^ (uint64_t)1 << 63) } }; }

   void __floatsitf(legacy_ptr<float128_t> ret, int32_t i) { *ret = i32_to_f128(i); }

   void __floatditf(legacy_ptr<float128_t> ret, uint64_t a) { *ret = i64_to_f128(a); }

   void __floatunsitf(legacy_ptr<float128_t> ret, uint32_t i) { *ret = ui32_to_f128(i); }

   void __floatunditf(legacy_ptr<float128_t> ret, uint64_t a) { *ret = ui64_to_f128(a); }

   double __floattidf(uint64_t l, uint64_t h) {
      unsigned __int128 val = h;
      val <<= 64;
      val |= l;
      return ___floattidf(*(__int128*)&val);
   }

   double __floatuntidf(uint64_t l, uint64_t h) {
      unsigned __int128 val = h;
      val <<= 64;
      val |= l;
      return ___floatuntidf((unsigned __int128)val);
   }

   double __floatsidf(int32_t i) { return from_softfloat64(i32_to_f64(i)); }

   void __extendsftf2(legacy_ptr<float128_t> ret, float f) { *ret = f32_to_f128(to_softfloat32(f)); }

   void __extenddftf2(legacy_ptr<float128_t> ret, double d) { *ret = f64_to_f128(to_softfloat64(d)); }

   void __fixtfti(legacy_ptr<__int128> ret, uint64_t l, uint64_t h) {
      float128_t f = { { l, h } };
      *ret         = ___fixtfti(f);
   }

   int32_t __fixtfsi(uint64_t l, uint64_t h) {
      float128_t f = { { l, h } };
      return f128_to_i32(f, 0, false);
   }

   int64_t __fixtfdi(uint64_t l, uint64_t h) {
      float128_t f = { { l, h } };
      return f128_to_i64(f, 0, false);
   }

   void __fixunstfti(legacy_ptr<unsigned __int128> ret, uint64_t l, uint64_t h) {
      float128_t f = { { l, h } };
      *ret         = ___fixunstfti(f);
   }

   uint32_t __fixunstfsi(uint64_t l, uint64_t h) {
      float128_t f = { { l, h } };
      return f128_to_ui32(f, 0, false);
   }

   uint64_t __fixunstfdi(uint64_t l, uint64_t h) {
      float128_t f = { { l, h } };
      return f128_to_ui64(f, 0, false);
   }

   void __fixsfti(legacy_ptr<__int128> ret, float a) { *ret = ___fixsfti(to_softfloat32(a).v); }

   void __fixdfti(legacy_ptr<__int128> ret, double a) { *ret = ___fixdfti(to_softfloat64(a).v); }

   void __fixunssfti(legacy_ptr<unsigned __int128> ret, float a) { *ret = ___fixunssfti(to_softfloat32(a).v); }

   void __fixunsdfti(legacy_ptr<unsigned __int128> ret, double a) { *ret = ___fixunsdfti(to_softfloat64(a).v); }

   double __trunctfdf2(uint64_t l, uint64_t h) {
      float128_t f = { { l, h } };
      return from_softfloat64(f128_to_f64(f));
   }

   float __trunctfsf2(uint64_t l, uint64_t h) {
      float128_t f = { { l, h } };
      return from_softfloat32(f128_to_f32(f));
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      Rft::template add<&Derived::__ashlti3>("env", "__ashlti3");
      Rft::template add<&Derived::__ashrti3>("env", "__ashrti3");
      Rft::template add<&Derived::__lshlti3>("env", "__lshlti3");
      Rft::template add<&Derived::__lshrti3>("env", "__lshrti3");
      Rft::template add<&Derived::__divti3>("env", "__divti3");
      Rft::template add<&Derived::__udivti3>("env", "__udivti3");
      Rft::template add<&Derived::__modti3>("env", "__modti3");
      Rft::template add<&Derived::__umodti3>("env", "__umodti3");
      Rft::template add<&Derived::__multi3>("env", "__multi3");
      Rft::template add<&Derived::__addtf3>("env", "__addtf3");
      Rft::template add<&Derived::__subtf3>("env", "__subtf3");
      Rft::template add<&Derived::__multf3>("env", "__multf3");
      Rft::template add<&Derived::__divtf3>("env", "__divtf3");
      Rft::template add<&Derived::__eqtf2>("env", "__eqtf2");
      Rft::template add<&Derived::__netf2>("env", "__netf2");
      Rft::template add<&Derived::__getf2>("env", "__getf2");
      Rft::template add<&Derived::__gttf2>("env", "__gttf2");
      Rft::template add<&Derived::__lttf2>("env", "__lttf2");
      Rft::template add<&Derived::__letf2>("env", "__letf2");
      Rft::template add<&Derived::__cmptf2>("env", "__cmptf2");
      Rft::template add<&Derived::__unordtf2>("env", "__unordtf2");
      Rft::template add<&Derived::__negtf2>("env", "__negtf2");
      Rft::template add<&Derived::__floatsitf>("env", "__floatsitf");
      Rft::template add<&Derived::__floatunsitf>("env", "__floatunsitf");
      Rft::template add<&Derived::__floatditf>("env", "__floatditf");
      Rft::template add<&Derived::__floatunditf>("env", "__floatunditf");
      Rft::template add<&Derived::__floattidf>("env", "__floattidf");
      Rft::template add<&Derived::__floatuntidf>("env", "__floatuntidf");
      Rft::template add<&Derived::__floatsidf>("env", "__floatsidf");
      Rft::template add<&Derived::__extendsftf2>("env", "__extendsftf2");
      Rft::template add<&Derived::__extenddftf2>("env", "__extenddftf2");
      Rft::template add<&Derived::__fixtfti>("env", "__fixtfti");
      Rft::template add<&Derived::__fixtfdi>("env", "__fixtfdi");
      Rft::template add<&Derived::__fixtfsi>("env", "__fixtfsi");
      Rft::template add<&Derived::__fixunstfti>("env", "__fixunstfti");
      Rft::template add<&Derived::__fixunstfdi>("env", "__fixunstfdi");
      Rft::template add<&Derived::__fixunstfsi>("env", "__fixunstfsi");
      Rft::template add<&Derived::__fixsfti>("env", "__fixsfti");
      Rft::template add<&Derived::__fixdfti>("env", "__fixdfti");
      Rft::template add<&Derived::__fixunssfti>("env", "__fixunssfti");
      Rft::template add<&Derived::__fixunsdfti>("env", "__fixunsdfti");
      Rft::template add<&Derived::__trunctfdf2>("env", "__trunctfdf2");
      Rft::template add<&Derived::__trunctfsf2>("env", "__trunctfsf2");
   }
};

} // namespace b1::rodeos
