#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>

namespace b1::rodeos {

struct filter_callback_state {
   std::function<void(const char* data, uint64_t size)> push_data;
};

template <typename Derived>
struct filter_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void push_data(eosio::vm::span<const char> data) {
      derived().get_filter_callback_state().push_data(data.data(), data.size());
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      RODEOS_REGISTER_CALLBACK(Rft, Derived, push_data)
   }
}; // query_callbacks

} // namespace b1::rodeos
