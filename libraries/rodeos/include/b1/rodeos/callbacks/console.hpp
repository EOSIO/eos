#pragma once

#include <b1/rodeos/callbacks/basic.hpp>
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

   void prints(const char* str) {
      auto len = derived().check_bounds_get_len(str);
      append_console(str, len);
   }

   void prints_l(const char* str, uint32_t len) {
      derived().check_bounds(str, len);
      append_console(str, len);
   }

   void printi(int64_t value) {
      auto& state = derived().get_state();
      if (!state.max_console_size)
         return;
      auto s = std::to_string(value);
      append_console(s.c_str(), s.size());
   }

   void printui(uint64_t value) {
      auto& state = derived().get_state();
      if (!state.max_console_size)
         return;
      auto s = std::to_string(value);
      append_console(s.c_str(), s.size());
   }

   void printi128(void* value) {
      // todo
      append_console("{unimplemented}", 15);
   }

   void printui128(void* value) {
      // todo
      append_console("{unimplemented}", 15);
   }

   void printsf(float value) { printdf(value); }

   void printdf(double value) {
      auto& state = derived().get_state();
      if (!state.max_console_size)
         return;
      auto s = std::to_string(value);
      append_console(s.c_str(), s.size());
   }

   void printqf(void*) {
      // don't bother
      append_console("{unimplemented}", 15);
   }

   void printn(uint64_t value) {
      auto& state = derived().get_state();
      if (!state.max_console_size)
         return;
      auto s = std::to_string(value);
      append_console(s.c_str(), s.size());
   }

   void printhex(const char* data, uint32_t len) {
      derived().check_bounds(data, len);
      auto& state = derived().get_state();
      for (uint32_t i = 0; i < len && state.console.size() + 2 < state.max_console_size; ++i) {
         static const char hex_digits[] = "0123456789ABCDEF";
         uint8_t           byte         = data[i];
         state.console.push_back(hex_digits[byte >> 4]);
         state.console.push_back(hex_digits[byte & 15]);
      }
   }

   template <typename Rft, typename Allocator>
   static void register_callbacks() {
      Rft::template add<Derived, &Derived::prints, Allocator>("env", "prints");
      Rft::template add<Derived, &Derived::prints_l, Allocator>("env", "prints_l");
      Rft::template add<Derived, &Derived::printi, Allocator>("env", "printi");
      Rft::template add<Derived, &Derived::printui, Allocator>("env", "printui");
      Rft::template add<Derived, &Derived::printi128, Allocator>("env", "printi128");
      Rft::template add<Derived, &Derived::printui128, Allocator>("env", "printui128");
      Rft::template add<Derived, &Derived::printsf, Allocator>("env", "printsf");
      Rft::template add<Derived, &Derived::printdf, Allocator>("env", "printdf");
      Rft::template add<Derived, &Derived::printqf, Allocator>("env", "printqf");
      Rft::template add<Derived, &Derived::printn, Allocator>("env", "printn");
      Rft::template add<Derived, &Derived::printhex, Allocator>("env", "printhex");
   }
}; // console_callbacks

} // namespace b1::rodeos
