#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>
#include <eosio/name.hpp>
#include <eosio/stream.hpp>

namespace b1::rodeos {

struct action_state {
   eosio::name         receiver{};
   eosio::input_stream action_data{};
   std::vector<char>   action_return_value{};
};

template <typename Derived>
struct action_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   int read_action_data(eosio::vm::span<char> data) {
      auto&  state = derived().get_state();
      size_t s     = state.action_data.end - state.action_data.pos;
      memcpy(data.data(), state.action_data.pos, std::min(data.size(), s));
      return s;
   }

   int action_data_size() {
      auto& state = derived().get_state();
      return state.action_data.end - state.action_data.pos;
   }

   uint64_t current_receiver() { return derived().get_state().receiver.value; }

   void set_action_return_value(eosio::vm::span<const char> packed_blob) {
      // todo: limit size
      derived().get_state().action_return_value.assign(packed_blob.begin(), packed_blob.end());
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      RODEOS_REGISTER_CALLBACK(Rft, Derived, read_action_data);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, action_data_size);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, current_receiver);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, set_action_return_value);
   }
}; // action_callbacks

} // namespace b1::rodeos
