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

inline void coverage_dump( uint32_t n ) {
   auto fName = std::string( "call-counts" ) + std::to_string( n ) + std::string( ".json" );
   std::ofstream out_json_file;
   out_json_file.open( fName );

   // dont use auto JSON serialization because
   // we want to convert maps to arrays
   out_json_file << "{\n";
   out_json_file << "\t\"functions\": {";
   for( const auto& code_map: coverage_maps::instance().funcnt_map ) {
      auto code_str = eosio::chain::name( code_map.first ).to_string();
      out_json_file << "\t\t\"" << code_str << "\": [\n";
      for( const auto& funcnt: code_map.second ) {
         auto file_num = funcnt.first;
         for( const auto& func: funcnt.second ) {
            auto func_num = func.first;
            auto calls = func.second;
            out_json_file << "\t\t\t[" << file_num << ", " << func_num << ", " << calls << "]\n";
         }
      }
      out_json_file << "\t\t]\n";
   }
   out_json_file << "\t}\n";

   out_json_file << "\t\"lines\": {";
   for( const auto& code_map: coverage_maps::instance().linecnt_map ) {
      auto code_str = eosio::chain::name( code_map.first ).to_string();
      out_json_file << "\t\t\"" << code_str << "\": [\n";

      for( const auto& linecnt: code_map.second ) {
         auto file_num = linecnt.first;
         for( const auto& line: linecnt.second ) {
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

inline void coverage_reset() {
   coverage_maps::instance().funcnt_map.clear();
   coverage_maps::instance().linecnt_map.clear();
}

} // namespace coverage
} // namespace eosio
