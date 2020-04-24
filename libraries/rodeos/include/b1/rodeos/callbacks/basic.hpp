#pragma once

#include <fc/exception/exception.hpp>

#include <eosio/stream.hpp>
#include <eosio/vm/backend.hpp>

namespace b1::rodeos {

struct assert_exception : std::exception {
   std::string msg;

   assert_exception(std::string&& msg) : msg(std::move(msg)) {}

   const char* what() const noexcept override { return msg.c_str(); }
};

template <typename Backend>
struct wasm_state {
   eosio::vm::wasm_allocator& wa;
   Backend&                   backend;
};

inline size_t copy_to_wasm(char* dest, size_t dest_size, const char* src, size_t src_size) {
   if (dest_size == 0)
      return src_size;
   auto copy_size = std::min(dest_size, src_size);
   memcpy(dest, src, copy_size);
   return copy_size;
}

template <typename Derived>
struct basic_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void check_bounds(const char* begin, const char* end) {
      if (begin > end)
         throw std::runtime_error("bad memory");
      // todo: check bounds
   }

   void check_bounds(const char* begin, uint32_t size) {
      // todo: check bounds
   }

   uint32_t check_bounds_get_len(const char* str) {
      // todo: check bounds
      return strlen(str);
   }

   void abort() { throw std::runtime_error("called abort"); }

   void eosio_assert_message(bool test, const char* msg, uint32_t msg_len) {
      check_bounds(msg, msg + msg_len);
      if (!test)
         throw assert_exception(std::string(msg, msg_len));
   }

   // todo: replace with prints_l
   void print_range(const char* begin, const char* end) {
      check_bounds(begin, end);
      std::cerr.write(begin, end - begin);
   }

   template <typename Rft, typename Allocator>
   static void register_callbacks() {
      Rft::template add<Derived, &Derived::abort, Allocator>("env", "abort");
      Rft::template add<Derived, &Derived::eosio_assert_message, Allocator>("env", "eosio_assert_message");
      Rft::template add<Derived, &Derived::print_range, Allocator>("env", "print_range");
   }
};

template <typename Backend>
struct data_state {
   eosio::input_stream input_data;
   std::vector<char>   output_data;
};

template <typename Derived>
struct data_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   uint32_t get_input_data(char* dest, uint32_t size) {
      derived().check_bounds(dest, size);
      auto& input_data = derived().get_state().input_data;
      return copy_to_wasm(dest, size, input_data.pos, size_t(input_data.end - input_data.pos));
   }

   void set_output_data(const char* data, uint32_t size) {
      derived().check_bounds(data, size);
      auto& output_data = derived().get_state().output_data;
      output_data.clear();
      output_data.insert(output_data.end(), data, data + size);
   }

   template <typename Rft, typename Allocator>
   static void register_callbacks() {
      Rft::template add<Derived, &Derived::get_input_data, Allocator>("env", "get_input_data");
      Rft::template add<Derived, &Derived::set_output_data, Allocator>("env", "set_output_data");
   }
};

} // namespace b1::rodeos
