#pragma once

#include <eosio/chain/name.hpp>
#include <unordered_map>
#include <string>
#include <fstream>

// rodeos lib is not the ideal location for this as it is also used by eosio-tester but rodeos lib keeps it out of nodeos
namespace eosio {
namespace coverage {

class coverage_maps {
public:
   static coverage_maps& instance() {
      static coverage_maps instance;
      return instance;
   }
   coverage_maps(const coverage_maps&) = delete;
   void operator=(const coverage_maps&) = delete;

   std::unordered_map<uint64_t, std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t> > > funcnt_map;
   std::unordered_map<uint64_t, std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t> > > linecnt_map;
private:
   coverage_maps() = default;
};

inline void coverage_inc_fun_cnt( uint64_t code, uint32_t file_num, uint32_t func_num ) {
   auto& code_map = coverage_maps::instance().funcnt_map[code];
   auto& funcnt = code_map[file_num];
   funcnt[func_num]++;
}

inline void coverage_inc_line_cnt( uint64_t code, uint32_t file_num, uint32_t line_num ) {
   auto& code_map = coverage_maps::instance().linecnt_map[code];
   auto& linecnt = code_map[file_num];
   linecnt[line_num]++;
}

inline uint32_t coverage_get_fun_cnt( uint64_t code, uint32_t file_num, uint32_t func_num ) {
   auto& code_map = coverage_maps::instance().funcnt_map[code];
   auto& funcnt = code_map[file_num];
   return funcnt[func_num];
}

inline uint32_t coverage_get_line_cnt( uint64_t code, uint32_t file_num, uint32_t line_num ) {
   auto& code_map = coverage_maps::instance().linecnt_map[code];
   auto& linecnt = code_map[file_num];
   return linecnt[line_num];
}

// dump coverage output to file
// if code == 0, dump all coverage for all codes, constrained by max argument
//    max is only checked for every code, so it is possible to exceed the number
// if code > 0, then only dump coverage for specific code and specific file_num
//    in this mode, max is effectively ignored
// returns the next code, or 0 if at end
inline uint64_t coverage_dump_funcnt(uint64_t code, uint32_t file_num, const char* file_name, uint32_t file_name_size, uint32_t max, bool append ) {
   std::ofstream out_bin_file;
   auto flags = std::ofstream::out | std::ofstream::binary;
   if (append)
      flags = flags | std::ofstream::app;
   else
      flags = flags | std::ofstream::trunc;

   ilog("coverage_dump_funcnt: file_name= ${f} max= ${max} ${app}", ("f", file_name)("nax", max)("app", append));
   out_bin_file.open(file_name, flags );
   uint32_t i = 0;
   auto funcnt_map = coverage_maps::instance().funcnt_map;
   auto code_itr = funcnt_map.begin();
   if (code > 0) {
      code_itr = funcnt_map.find(code);
   }
   while (code_itr != funcnt_map.end() && i < max) {
      auto codenum = code_itr->first;
      auto filenum_map = code_itr->second;
      auto filenum_itr = filenum_map.begin();
      if (code > 0) {
         filenum_itr = filenum_map.find(file_num);
      }
      while (filenum_itr != filenum_map.end()) {
         auto filenum = filenum_itr->first;
         auto funcnum_map = filenum_itr->second;
         for (const auto& funcnum_itr : funcnum_map) {
            auto funcnum = funcnum_itr.first;
            auto calls = funcnum_itr.second;
            out_bin_file.write(reinterpret_cast<const char*>(&codenum), sizeof(code));
            out_bin_file.write(reinterpret_cast<const char*>(&filenum), sizeof(filenum));
            out_bin_file.write(reinterpret_cast<const char*>(&funcnum), sizeof(funcnum));
            out_bin_file.write(reinterpret_cast<const char*>(&calls), sizeof(calls));
            ++i;
         }
         ++filenum_itr;
         if (code > 0)
            break;
      }
      ++code_itr;
      if (code > 0);
         break;
   }

   out_bin_file.flush();
   out_bin_file.close();

   uint64_t r = 0;
   if(code_itr != funcnt_map.end())
      r = code_itr->first;
   return r;
}

inline void coverage_reset() {
   coverage_maps::instance().funcnt_map.clear();
   coverage_maps::instance().linecnt_map.clear();
}

} // namespace coverage
} // namespace eosio
