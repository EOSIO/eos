/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include "get_table_test.hpp"

void get_table_test::addnumobj(uint64_t input) {
   numobjs numobjs_table( _self, _self.value );
      numobjs_table.emplace(_self, [&]( auto& obj ) {
         obj.key = numobjs_table.available_primary_key();
         obj.sec64 = input;
         obj.sec128 = input;
         obj.secdouble = input;
         obj.secldouble = input;
      });
}

void get_table_test::modifynumobj(uint64_t id) {
   numobjs numobjs_table( _self, _self.value );
   numobjs_table.modify(numobjs_table.get(id), _self, [&](auto& obj) {
      obj.sec64++;
   });
}

void get_table_test::erasenumobj(uint64_t id) {
   numobjs numobjs_table( _self, _self.value );
   auto iterator = numobjs_table.find(id);
   check(iterator != numobjs_table.end(), "Record does not exist.");
   numobjs_table.erase(iterator);
}

void get_table_test::addhashobj(std::string hashinput) {
   hashobjs hashobjs_table( _self, _self.value );
      hashobjs_table.emplace(_self, [&]( auto& obj ) {
         obj.key = hashobjs_table.available_primary_key();
         obj.hash_input = hashinput;
         obj.sec256 = sha256(hashinput.data(), hashinput.size());
         obj.sec160 = ripemd160(hashinput.data(), hashinput.size());
      });
}
