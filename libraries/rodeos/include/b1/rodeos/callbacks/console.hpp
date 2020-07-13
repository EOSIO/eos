#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>
#include <eosio/to_json.hpp>

namespace b1::rodeos {

struct console_state {
   uint32_t    max_console_size = 0;
   std::string console          = {};
};

template <typename Derived>
struct console_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void append_console(const char* str, uint32_t len) {
      auto& state = derived().get_state();
      state.console.append(str, std::min(size_t(len), state.max_console_size - state.console.size()));
   }

   template <int size>
   void append_console(const char (&str)[size]) {
      return append_console(str, size - 1); // omit 0-termination
   }

   void prints(null_terminated_ptr str) { append_console(str.data(), str.size()); }
   void prints_l(legacy_span<const char> str) { append_console(str.data(), str.size()); }

   template <typename T>
   void print_numeric(T value) {
      auto& state = derived().get_state();
      if (!state.max_console_size)
         return;
      auto s = std::to_string(value);
      append_console(s.c_str(), s.size());
   }

   void printi(int64_t value) { print_numeric(value); }
   void printui(uint64_t value) { print_numeric(value); }
   void printi128(legacy_ptr<const __int128> p) { append_console("{printi128 unimplemented}"); }
   void printui128(legacy_ptr<const unsigned __int128> p) { append_console("{printui128 unimplemented}"); }
   void printsf(float value) { print_numeric(value); }
   void printdf(double value) { print_numeric(value); }
   void printqf(legacy_ptr<const char>) { append_console("{printqf unimplemented}"); }

   void printn(uint64_t value) {
      auto& state = derived().get_state();
      if (!state.max_console_size)
         return;
      auto s = eosio::name{ value }.to_string();
      append_console(s.c_str(), s.size());
   }

   void printhex(legacy_span<const char> data) {
      auto& state = derived().get_state();
      for (uint32_t i = 0; i < data.size() && state.console.size() + 2 < state.max_console_size; ++i) {
         static const char hex_digits[] = "0123456789ABCDEF";
         uint8_t           byte         = data[i];
         state.console.push_back(hex_digits[byte >> 4]);
         state.console.push_back(hex_digits[byte & 15]);
      }
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      RODEOS_REGISTER_CALLBACK(Rft, Derived, prints);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, prints_l);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, printi);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, printui);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, printi128);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, printui128);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, printsf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, printdf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, printqf);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, printn);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, printhex);
   }
}; // console_callbacks

} // namespace b1::rodeos
