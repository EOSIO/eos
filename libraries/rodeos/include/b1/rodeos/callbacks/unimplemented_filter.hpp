#pragma once

#include <b1/rodeos/callbacks/basic.hpp>

namespace b1::rodeos {

template <typename Derived>
struct unimplemented_filter_callbacks {
   template <typename T>
   T unimplemented(const char* name) {
      throw std::runtime_error("wasm called " + std::string(name) + ", which is unimplemented");
   }

   // system_api
   int64_t current_time() { return unimplemented<int64_t>("current_time"); }

   template <typename Rft, typename Allocator>
   static void register_callbacks() {
      // system_api
      Rft::template add<Derived, &Derived::current_time, Allocator>("env", "current_time");
   }
}; // unimplemented_filter_callbacks

} // namespace b1::rodeos
