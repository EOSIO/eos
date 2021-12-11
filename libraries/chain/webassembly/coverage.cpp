#include <eosio/chain/webassembly/interface.hpp>

#include <unordered_map>
#include <fstream>

namespace eosio { namespace chain { namespace webassembly {
   std::unordered_map<uint64_t, std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t> > > funcnt_map;
   std::unordered_map<uint64_t, std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t> > > linecnt_map;

   void interface::coverage_inc_fun_cnt(uint64_t code, uint32_t file_num, uint32_t func_num) {
       auto &code_map = funcnt_map[code];
       auto &funcnt = code_map[file_num];
       funcnt[func_num]++;
   }

   void interface::coverage_inc_line_cnt(uint64_t code, uint32_t file_num, uint32_t line_num) {
       auto &code_map = linecnt_map[code];
       auto &linecnt = code_map[file_num];
       linecnt[line_num]++;
   }

   uint32_t interface::coverage_get_fun_cnt(uint64_t code, uint32_t file_num, uint32_t func_num) {
      auto &code_map = funcnt_map[code];
      auto &funcnt = code_map[file_num];
      return funcnt[func_num];
   }

   uint32_t interface::coverage_get_line_cnt(uint64_t code, uint32_t file_num, uint32_t line_num) {
      auto &code_map = linecnt_map[code];
      auto &linecnt = code_map[file_num];
      return linecnt[line_num];
   }

   void interface::coverage_dump(uint32_t n) {
      auto fName = std::string("call-counts") + std::to_string(n) + std::string(".json");
      std::ofstream out_json_file;
      out_json_file.open(fName);

      // dont use auto JSON serialization because 
      // we want to convert maps to arrays
      out_json_file << "{\n";
      out_json_file << "\t\"functions\": {";
      for (auto code_map : funcnt_map) {
         auto code_str = eosio::chain::name(code_map.first).to_string();
         out_json_file << "\t\t\"" << code_str <<"\": [\n";
         for (auto funcnt : code_map.second) {
            auto file_num = funcnt.first;
            for (auto func : funcnt.second) {
               auto func_num = func.first;
               auto calls = func.second;
               out_json_file << "\t\t\t[" << file_num << ", " << func_num << ", " << calls << "]\n";
            }
         }
         out_json_file << "\t\t]\n";
      }
      out_json_file << "\t}\n";

      out_json_file << "\t\"lines\": {";
      for (auto code_map : linecnt_map) {
         auto code_str = eosio::chain::name(code_map.first).to_string();
         out_json_file << "\t\t\"" << code_str <<"\": [\n";

         for (auto linecnt : code_map.second) {
            auto file_num = linecnt.first;
            for (auto line : linecnt.second) {
               auto line_num = line.first;
               auto cnt = line.second;
               out_json_file << "\t\t\t[" << file_num << ", " << line_num << ", " << cnt << "]\n";
            }
         }
         out_json_file << "]\n";
      }
      out_json_file << "\t}\n";
      out_json_file << "}\n";
      out_json_file.close();
   }

   void interface::coverage_reset() {
       funcnt_map.clear();
       linecnt_map.clear();
   }
}}}
