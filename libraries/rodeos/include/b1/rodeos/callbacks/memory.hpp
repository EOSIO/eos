#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>

namespace b1::rodeos {

template <typename Derived>
struct memory_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void* memcpy_impl(unvalidated_ptr<char> dest, unvalidated_ptr<const char> src, wasm_size_t length) const {
      volatile auto check_src    = *((const char*)src + length - 1);
      volatile auto check_dest   = *((const char*)dest + length - 1);
      volatile auto check_result = *(const char*)dest;
      eosio::vm::ignore_unused_variable_warning(check_src, check_dest, check_result);
      if ((size_t)(std::abs((ptrdiff_t)(char*)dest - (ptrdiff_t)(const char*)src)) < length)
         throw std::runtime_error("memcpy can only accept non-aliasing pointers");
      return (char*)std::memcpy((char*)dest, (const char*)src, length);
   }

   void* memmove_impl(unvalidated_ptr<char> dest, unvalidated_ptr<const char> src, wasm_size_t length) const {
      volatile auto check_src    = *((const char*)src + length - 1);
      volatile auto check_dest   = *((const char*)dest + length - 1);
      volatile auto check_result = *(const char*)dest;
      eosio::vm::ignore_unused_variable_warning(check_src, check_dest, check_result);
      return (char*)std::memmove((char*)dest, (const char*)src, length);
   }

   int32_t memcmp_impl(unvalidated_ptr<const char> dest, unvalidated_ptr<const char> src, wasm_size_t length) const {
      volatile auto check_src  = *((const char*)src + length - 1);
      volatile auto check_dest = *((const char*)dest + length - 1);
      eosio::vm::ignore_unused_variable_warning(check_src, check_dest);
      int32_t ret = std::memcmp((const char*)dest, (const char*)src, length);
      return ret < 0 ? -1 : ret > 0 ? 1 : 0;
   }

   void* memset_impl(unvalidated_ptr<char> dest, int32_t value, wasm_size_t length) const {
      volatile auto check_dest   = *((const char*)dest + length - 1);
      volatile auto check_result = *(const char*)dest;
      eosio::vm::ignore_unused_variable_warning(check_dest, check_result);
      return (char*)std::memset((char*)dest, value, length);
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      Rft::template add<&Derived::memcpy_impl>("env", "memcpy");
      Rft::template add<&Derived::memmove_impl>("env", "memmove");
      Rft::template add<&Derived::memcmp_impl>("env", "memcmp");
      Rft::template add<&Derived::memset_impl>("env", "memset");
   }
}; // memory_callbacks

} // namespace b1::rodeos
