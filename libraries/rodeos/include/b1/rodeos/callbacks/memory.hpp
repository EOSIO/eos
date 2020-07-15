#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>

namespace b1::rodeos {

template <typename Derived>
struct memory_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void* memcpy_impl(memcpy_params params) const {
      auto [dest, src, length] = params;
      if ((size_t)(std::abs((ptrdiff_t)(char*)dest - (ptrdiff_t)(const char*)src)) < length)
         throw std::runtime_error("memcpy can only accept non-aliasing pointers");
      return (char*)std::memcpy((char*)dest, (const char*)src, length);
   }

   void* memmove_impl(memcpy_params params) const {
      auto [dest, src, length] = params;
      return (char*)std::memmove((char*)dest, (const char*)src, length);
   }

   int32_t memcmp_impl(memcmp_params params) const {
      auto [dest, src, length] = params;
      int32_t ret              = std::memcmp((const char*)dest, (const char*)src, length);
      return ret < 0 ? -1 : ret > 0 ? 1 : 0;
   }

   void* memset_impl(memset_params params) const {
      auto [dest, value, length] = params;
      return (char*)std::memset((char*)dest, value, length);
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      b1::rodeos::register_host_function<Rft, &Derived::memcpy_impl>(BOOST_HANA_STRING("memcpy"));
      b1::rodeos::register_host_function<Rft, &Derived::memmove_impl>(BOOST_HANA_STRING("memmove"));
      b1::rodeos::register_host_function<Rft, &Derived::memcmp_impl>(BOOST_HANA_STRING("memcmp"));
      b1::rodeos::register_host_function<Rft, &Derived::memset_impl>(BOOST_HANA_STRING("memset"));
   }
}; // memory_callbacks

} // namespace b1::rodeos
