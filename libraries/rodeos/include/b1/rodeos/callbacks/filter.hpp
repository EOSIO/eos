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

   int32_t clock_gettime(int32_t id, uint64_t* data) {
      std::chrono::nanoseconds result;
      if (id == 0) { // CLOCK_REALTIME
         result = std::chrono::system_clock::now().time_since_epoch();
      } else if (id == 1) { // CLOCK_MONOTONIC
         result = std::chrono::steady_clock::now().time_since_epoch();
      } else {
         return -1;
      }
      int32_t               sec  = result.count() / 1000000000;
      int32_t               nsec = result.count() % 1000000000;
      fc::datastream<char*> ds((char*)data, 8);
      fc::raw::pack(ds, sec);
      fc::raw::pack(ds, nsec);
      return 0;
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      Rft::template add<&Derived::push_data>("env", "push_data");
      Rft::template add<&Derived::clock_gettime>("env", "clock_gettime");
   }
}; // query_callbacks

} // namespace b1::rodeos
