#pragma once

#include <b1/rodeos/callbacks/basic.hpp>
#include <b1/rodeos/get_state_row.hpp>

namespace b1::rodeos {

// for filter callback. query callback for current_time is in query.hpp

template <typename Derived>
struct system_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   auto load_block_info() {
      auto& state = derived().get_state();

      block_info_kv table{ { derived().get_db_view_state() } };
      if (table.primary_index.begin() != table.primary_index.end()) {
         return (--table.primary_index.end()).value();
      }
      throw std::runtime_error("system callback database is missing block_info_v0");
   }

   int64_t current_time() {
      auto block_info = load_block_info();
      return std::visit(
            [](auto& b) { //
               return b.timestamp.to_time_point().time_since_epoch().count();
            },
            block_info);
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      RODEOS_REGISTER_CALLBACK(Rft, Derived, current_time);
   }
}; // system_callbacks

} // namespace b1::rodeos
