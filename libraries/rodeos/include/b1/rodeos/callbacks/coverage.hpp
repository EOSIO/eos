#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>
#include <eosio/coverage.hpp>

namespace b1::rodeos {

struct coverage_state {
};

template <typename Derived>
struct coverage_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void coverage_inc_fun_cnt(uint64_t code, uint32_t file_num, uint32_t func_num) {
      eosio::coverage::coverage_inc_fun_cnt(code, file_num, func_num);
   }

   void coverage_inc_line_cnt(uint64_t code, uint32_t file_num, uint32_t line_num) {
      eosio::coverage::coverage_inc_line_cnt(code, file_num, line_num);
   }

   uint32_t coverage_get_fun_cnt(uint64_t code, uint32_t file_num, uint32_t func_num) {
      return eosio::coverage::coverage_get_fun_cnt(code, file_num, func_num);
   }

   uint32_t coverage_get_line_cnt(uint64_t code, uint32_t file_num, uint32_t line_num) {
      return eosio::coverage::coverage_get_line_cnt(code, file_num, line_num);
   }

   void coverage_dump(uint32_t n) {
      eosio::coverage::coverage_dump(n);
   }

   void coverage_reset() {
      eosio::coverage::coverage_reset();
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_inc_fun_cnt);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_inc_line_cnt);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_get_fun_cnt);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_get_line_cnt);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_dump);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_reset);
   }
}; // coverage_callbacks

} // namespace b1::rodeos