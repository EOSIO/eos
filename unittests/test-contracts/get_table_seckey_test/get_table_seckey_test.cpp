/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include "get_table_seckey_test.hpp"


void get_table_seckey_test::addnumobj(uint64_t input, std::string nm) {
   numobjs numobjs_table( _self, _self.value );
   numobjs_table.emplace(_self, [&](auto &obj) {
      obj.key = numobjs_table.available_primary_key();
      obj.sec64 = input;
      obj.sec128 = input;
      obj.secdouble = input;
      obj.secldouble = input;
      obj.nm = name(nm);
   });
}
