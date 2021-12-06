#include <eosio/chain/webassembly/interface.hpp>

#include <unordered_map>
#include <fstream>

namespace eosio { namespace chain { namespace webassembly {
   static std::unordered_map<name, std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t> > > funcnt_map;
   static std::unordered_map<name, std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t> > > linecnt_map;

   void interface::coverage_inc_fun_cnt(account_name code, uint32_t file_num, uint32_t func_num) {
       auto &code_map = funcnt_map[code];
       auto &funcnt = code_map[file_num];
       funcnt[func_num]++;
   }

   void interface::coverage_inc_line_cnt(account_name code, uint32_t file_num, uint32_t line_num) {
       auto &code_map = linecnt_map[code];
       auto &linecnt = code_map[file_num];
       linecnt[line_num]++;
   }

   void interface::coverage_dump() {
       auto fName = std::string("call-counts.json");
       std::ofstream out_json_file;
       out_json_file.open(fName);
       
       // dont use auto JSON serialization because 
       // we want to convert maps to arrays
       out_json_file << "{\n";
       out_json_file << "\t\"functions\": {";
       for (auto code_map : funcnt_map) {
            auto code_str = code_map.first.to_string();
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
            auto code_str = code_map.first.to_string();
            out_json_file << "{ \"" << code_str <<"\"";

            for (auto linecnt : code_map.second) {
                for (auto line : linecnt.second) {
                    
                }
            }
            out_json_file << "]\n";
       }
       out_json_file << "\t}\n";
       out_json_file.close();
   }
}}}
