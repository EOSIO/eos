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
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __ashlti3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __ashrti3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __lshlti3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __lshrti3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __divti3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __udivti3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __modti3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __umodti3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __multi3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __addtf3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __subtf3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __multf3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __divtf3);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __eqtf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __netf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __getf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __gttf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __lttf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __letf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __cmptf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __unordtf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __negtf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __floatsitf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __floatunsitf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __floatditf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __floatunditf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __floattidf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __floatuntidf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __floatsidf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __extendsftf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __extenddftf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixtfti);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixtfdi);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixtfsi);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixunstfti);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixunstfdi);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixunstfsi);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixsfti);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixdfti);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixunssfti);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __fixunsdfti);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __trunctfdf2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, __trunctfsf2);
   }
};

} // namespace b1::rodeos
