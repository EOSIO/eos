#pragma once

#include <b1/rodeos/callbacks/basic.hpp>

namespace b1::rodeos {

template <typename Derived>
struct memory_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   char* memcpy(char* dest, const char* src, uint32_t length) {
      derived().check_bounds(dest, length);
      derived().check_bounds(src, length);
      if ((size_t)(std::abs((ptrdiff_t)dest - (ptrdiff_t)src)) < length)
         throw std::runtime_error("memcpy can only accept non-aliasing pointers");
      return (char*)::memcpy(dest, src, length);
   }

   char* memmove(char* dest, const char* src, uint32_t length) {
      derived().check_bounds(dest, length);
      derived().check_bounds(src, length);
      return (char*)::memmove(dest, src, length);
   }

   int memcmp(const char* dest, const char* src, uint32_t length) {
      derived().check_bounds(dest, length);
      derived().check_bounds(src, length);
      int ret = ::memcmp(dest, src, length);
      if (ret < 0)
         return -1;
      if (ret > 0)
         return 1;
      return 0;
   }

   char* memset(char* dest, int value, uint32_t length) {
      derived().check_bounds(dest, length);
      return (char*)::memset(dest, value, length);
   }

   template <typename Rft, typename Allocator>
   static void register_callbacks() {
      Rft::template add<Derived, &Derived::memcpy, Allocator>("env", "memcpy");
      Rft::template add<Derived, &Derived::memmove, Allocator>("env", "memmove");
      Rft::template add<Derived, &Derived::memcmp, Allocator>("env", "memcmp");
      Rft::template add<Derived, &Derived::memset, Allocator>("env", "memset");
   }
}; // memory_callbacks

} // namespace b1::rodeos
