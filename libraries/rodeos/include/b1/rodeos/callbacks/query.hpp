#pragma once

#include <b1/rodeos/callbacks/basic.hpp>
#include <b1/rodeos/get_state_row.hpp>

namespace b1::rodeos {

struct query_state {
   uint32_t                          block_num;
   std::optional<rodeos::block_info> block_info;
};

template <typename Derived>
struct query_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void load_block_info() {
      auto& state = derived().get_state();
      if (state.block_info)
         return;
      auto info = get_state_row<rodeos::block_info>(
            derived().get_db_view_state().kv_state.view,
            std::make_tuple(eosio::name{ "block.info" }, eosio::name{ "primary" }, state.block_num));
      if (!info)
         throw std::runtime_error("database is missing block.info for block " + std::to_string(state.block_num));
      state.block_info = info->second;
   }

   int64_t current_time() {
      load_block_info();
      return std::visit(
            [](auto& b) { //
               return b.timestamp.to_time_point().time_since_epoch().count();
            },
            *derived().get_state().block_info);
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      Rft::template add<&Derived::current_time>("env", "current_time");
   }
}; // query_callbacks

} // namespace b1::rodeos
