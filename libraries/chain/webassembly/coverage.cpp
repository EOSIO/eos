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

   uint32_t interface::coverage_get_fun_cnt(account_name code, uint32_t file_num, uint32_t func_num) {
       uint32_t ret_val = 0;
       if (funcnt_map.count(code) > 0) {
            auto &code_map = funcnt_map[code];
            if (code_map.count(file_num) > 0) {
                auto &funcnt = code_map[file_num];
                if (funcnt.count(func_num) > 0) {
                    ret_val = funcnt[func_num];
                }
            }
       }
       return ret_val;
   }

   uint32_t interface::coverage_get_line_cnt(account_name code, uint32_t file_num, uint32_t line_num) {
       uint32_t ret_val = 0;
       if (linecnt_map.count(code) > 0) {
            auto &code_map = linecnt_map[code];
            if (code_map.count(file_num) > 0) {
                auto &linecnt = code_map[file_num];
                if (linecnt.count(line_num) > 0) {
                    ret_val = linecnt[line_num];
                }
            }
       }
       return ret_val;
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

   void interface::coverage_reset() {
       funcnt_map.clear();
       linecnt_map.clear();
   }
}}}
