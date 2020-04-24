#pragma once

#include <b1/rodeos/callbacks/basic.hpp>

namespace b1::rodeos {

struct action_state {
   eosio::name         receiver{};
   eosio::input_stream action_data{};
   std::vector<char>   action_return_value{};
};

template <typename Derived>
struct action_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   int read_action_data(char* memory, uint32_t buffer_size) {
      derived().check_bounds(memory, buffer_size);
      auto& state = derived().get_state();
      auto  s     = state.action_data.end - state.action_data.pos;
      if (buffer_size == 0)
         return s;
      auto copy_size = std::min(static_cast<size_t>(buffer_size), static_cast<size_t>(s));
      memcpy(memory, state.action_data.pos, copy_size);
      return copy_size;
   }

   int action_data_size() {
      auto& state = derived().get_state();
      return state.action_data.end - state.action_data.pos;
   }

   uint64_t current_receiver() { return derived().get_state().receiver.value; }

   void set_action_return_value(char* packed_blob, uint32_t datalen) {
      // todo: limit size
      derived().check_bounds(packed_blob, datalen);
      derived().get_state().action_return_value.assign(packed_blob, packed_blob + datalen);
   }

   template <typename Rft, typename Allocator>
   static void register_callbacks() {
      Rft::template add<Derived, &Derived::read_action_data, Allocator>("env", "read_action_data");
      Rft::template add<Derived, &Derived::action_data_size, Allocator>("env", "action_data_size");
      Rft::template add<Derived, &Derived::current_receiver, Allocator>("env", "current_receiver");
      Rft::template add<Derived, &Derived::set_action_return_value, Allocator>("env", "set_action_return_value");
   }
}; // action_callbacks

} // namespace b1::rodeos
