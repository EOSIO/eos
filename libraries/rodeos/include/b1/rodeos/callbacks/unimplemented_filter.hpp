#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>

namespace b1::rodeos {

template <typename Derived>
struct unimplemented_filter_callbacks {
   template <typename T>
   T unimplemented(const char* name) {
      throw std::runtime_error("wasm called " + std::string(name) + ", which is unimplemented");
   }

   // system_api
   int64_t current_time() { return unimplemented<int64_t>("current_time"); }

   template <typename Rft>
   static void register_callbacks() {
      Rft::template add<&Derived::current_time>("env", "current_time");
   }
}; // unimplemented_filter_callbacks

} // namespace b1::rodeos
