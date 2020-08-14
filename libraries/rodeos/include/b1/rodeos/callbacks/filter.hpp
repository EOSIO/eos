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

   void print_time_us() {
      auto result =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
      time_t  sec  = result.count() / 1'000'000;
      int32_t usec = result.count() % 1'000'000;
      tm      t;
      gmtime_r(&sec, &t);
      char s[sizeof("2011-10-08T07:07:09.000000")];
      snprintf(s, sizeof(s), "%04d-%02d-%02dT%02d:%02d:%02d.%06d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
               t.tm_min, t.tm_sec, usec);
      derived().append_console(s, strlen(s));
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      Rft::template add<&Derived::push_data>("env", "push_data");
      Rft::template add<&Derived::print_time_us>("env", "print_time_us");
   }
}; // query_callbacks

} // namespace b1::rodeos
