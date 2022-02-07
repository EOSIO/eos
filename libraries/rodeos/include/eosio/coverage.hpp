#pragma once

#include <eosio/chain/name.hpp>
#include <unordered_map>
#include <string>
#include <fstream>

using cov_map_t = std::unordered_map<uint64_t, std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t> > >;

// rodeos lib is not the ideal location for this as it is also used by eosio-tester but rodeos lib keeps it out of nodeos
namespace eosio {
namespace coverage {

template<uint64_t Unique>
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

enum coverage_mode { func, line };

inline void coverage_inc_cnt( uint64_t code, uint32_t file_num, uint32_t func_or_line_num, cov_map_t& cov_map) {
   auto& code_map = cov_map[code];
   auto& cnt_map = code_map[file_num];
   cnt_map[func_or_line_num]++;
}

inline uint32_t coverage_get_cnt( uint64_t code, uint32_t file_num, uint32_t func_or_line_num, cov_map_t& cov_map) {
   auto& code_map = cov_map[code];
   auto& cnt_map = code_map[file_num];
   return cnt_map[func_or_line_num];
}

// dump coverage output (function or line) to binary file
// if code == 0, begin at first code in the map
//    max is only checked for every code, so it is possible to exceed the number
// if max == 0, then only dump coverage for specific code and specific file_num
//     in theis case code must be > 0
// returns the next code, or 0 if at end
inline uint64_t coverage_dump(uint64_t code, uint32_t file_num, const char* file_name, uint32_t file_name_size, uint32_t max, bool append, cov_map_t& cov_map) {
   std::ofstream out_bin_file;
   auto flags = std::ofstream::out | std::ofstream::binary;
   if (append)
      flags = flags | std::ofstream::app;
   else
      flags = flags | std::ofstream::trunc;

   ilog("coverage_dump_funcnt: file_name= ${f} max= ${max} ${app}", ("f", file_name)("nax", max)("app", append));
   out_bin_file.open(file_name, flags );
   uint32_t i = 0;
   auto code_itr = cov_map.begin();
   if (max == 0 && code == 0) {
      elog("coverage_dump_funcnt: when max == 0, code must be > 0");
      return 0;
   }
   if (code > 0) {
      code_itr = cov_map.find(code);
   }
   while (code_itr != cov_map.end() && (max == 0 || i < max)) {
      auto codenum = code_itr->first;
      auto& filenum_map = code_itr->second;
      auto filenum_itr = filenum_map.begin();
      if (max == 0) {
         filenum_itr = filenum_map.find(file_num);
      }
      while (filenum_itr != filenum_map.end()) {
         auto filenum = filenum_itr->first;
         auto& funcnum_map = filenum_itr->second;
         for (const auto& funcnum_itr : funcnum_map) {
            auto func_or_line_num = funcnum_itr.first;
            auto calls = funcnum_itr.second;
            out_bin_file.write(reinterpret_cast<const char*>(&codenum), sizeof(code));
            out_bin_file.write(reinterpret_cast<const char*>(&filenum), sizeof(filenum));
            out_bin_file.write(reinterpret_cast<const char*>(&func_or_line_num), sizeof(func_or_line_num));
            out_bin_file.write(reinterpret_cast<const char*>(&calls), sizeof(calls));
            ++i;
         }
         ++filenum_itr;
         if (max == 0)
            break;
      }
      ++code_itr;
      if (max == 0)
         break;
   }

   out_bin_file.flush();
   out_bin_file.close();

   uint64_t r = 0;
   if(code_itr != cov_map.end())
      r = code_itr->first;
   return r;
}

} // namespace coverage
} // namespace eosio
