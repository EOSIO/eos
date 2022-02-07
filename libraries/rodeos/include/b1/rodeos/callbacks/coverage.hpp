#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>
#include <eosio/coverage.hpp>

namespace b1::rodeos {

template<eosio::name::raw Unique>
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
      eosio::name T;
   private:
      coverage_maps() = default;
};

enum class coverage_mode = { func, line };

struct coverage_state {
};

template <typename Derived>
struct coverage_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   uint32_t coverage_getinc(uint64_t code, uint32_t file_num, uint32_t func_or_line_num, uint32_t mode, bool inc) {
      if(inc) {
         if (mode == coverage_mode::func) {
            eosio::coverage::coverage_inc_cnt(code, file_num, func_or_line_num, coverage_maps::instance().funcnt_map);
         } else if (mode == coverage_mode::line) {
            eosio::coverage::coverage_inc_cnt(code, file_num, func_or_line_num, coverage_maps::instance().linecnt_map);
         }
      }
      else {
         if (mode == 0) {
            return eosio::coverage::coverage_get_cnt(code, file_num, func_or_line_num, coverage_maps::instance().funcnt_map);
         }
         else if (mode == 1) {
            return eosio::coverage::coverage_get_cnt(code, file_num, func_or_line_num, coverage_maps::instance().linecnt_map);
         }
      }
      return 0;
   }

   uint64_t coverage_dump(uint64_t code, uint32_t file_num, eosio::vm::span<const char> file_name, uint32_t max, bool append, uint32_t mode, bool reset) {
      if (reset) {
         coverage_maps<"rodeos"_n>::instance().funcnt_map.clear();
         coverage_maps<"rodeos"_n>::instance().linecnt_map.clear();
      }
      else if (mode == 0) {
         return eosio::coverage::coverage_dump(code, file_num, file_name.data(), file_name.size(), max, append, coverage_maps<"rodeos"_n>::instance().funcnt_map);
      }
      else if (mode == 1) {
         return eosio::coverage::coverage_dump(code, file_num, file_name.data(), file_name.size(), max, append, coverage_maps<"rodeos"_n>::instance().linecnt_map);
      }
      return 0;
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_getinc);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, coverage_dump);
   }
}; // coverage_callbacks

} // namespace b1::rodeos