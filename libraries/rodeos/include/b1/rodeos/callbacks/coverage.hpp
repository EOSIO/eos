#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>
#include <eosio/coverage.hpp>

namespace b1::rodeos {

class coverage_maps {
   public:
      static coverage_maps& instance() {
         static coverage_maps instance;
         return instance;
      }
      coverage_maps(const coverage_maps&) = delete;
      void operator=(const coverage_maps&) = delete;

      cov_map_t funcnt_map;
      cov_map_t linecnt_map;
   private:
      coverage_maps() = default;
};

struct coverage_state {
};

template <typename Derived>
struct coverage_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   void coverage_inc_fun_cnt(uint64_t code, uint32_t file_num, uint32_t func_num) {
      eosio::coverage::coverage_inc_fun_cnt(code, file_num, func_num, coverage_maps::instance().funcnt_map);
   }

   void coverage_inc_line_cnt(uint64_t code, uint32_t file_num, uint32_t line_num) {
      eosio::coverage::coverage_inc_line_cnt(code, file_num, line_num, coverage_maps::instance().linecnt_map);
   }

   uint32_t coverage_get_fun_cnt(uint64_t code, uint32_t file_num, uint32_t func_num) {
      return eosio::coverage::coverage_get_fun_cnt(code, file_num, func_num, coverage_maps::instance().funcnt_map);
   }

   uint32_t coverage_get_line_cnt(uint64_t code, uint32_t file_num, uint32_t line_num) {
      return eosio::coverage::coverage_get_line_cnt(code, file_num, line_num, coverage_maps::instance().linecnt_map);
   }

   uint64_t coverage_dump_funcnt(uint64_t code, uint32_t file_num, eosio::vm::span<const char> file_name, uint32_t max, bool append) {
      return eosio::coverage::coverage_dump_funcnt(code, file_num, file_name.data(), file_name.size(), max, append, coverage_maps::instance().funcnt_map);
   }

   void coverage_reset() {
      coverage_maps::instance().funcnt_map.clear();
      coverage_maps::instance().linecnt_map.clear();
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_inc_fun_cnt);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_inc_line_cnt);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_get_fun_cnt);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_get_line_cnt);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_dump_funcnt);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_reset);
   }
}; // coverage_callbacks

} // namespace b1::rodeos