#pragma once

#include <b1/rodeos/callbacks/basic.hpp>

namespace b1::rodeos {

struct filter_callback_state {
   std::function<void(const char* data, uint64_t size)> push_data;
};

template <typename Derived>
struct filter_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void push_data(const char* data, uint32_t size) {
      derived().check_bounds(data, size);
      derived().get_filter_callback_state().push_data(data, size);
   }

   template <typename Rft, typename Allocator>
   static void register_callbacks() {
      Rft::template add<Derived, &Derived::push_data, Allocator>("env", "push_data");
   }
}; // query_callbacks

} // namespace b1::rodeos
