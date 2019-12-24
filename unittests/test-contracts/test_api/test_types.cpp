#include <apifinylib/apifiny.hpp>

#include "test_api.hpp"

void test_types::types_size() {

   apifiny_assert( sizeof(int64_t)   ==  8, "int64_t size != 8"   );
   apifiny_assert( sizeof(uint64_t)  ==  8, "uint64_t size != 8"  );
   apifiny_assert( sizeof(uint32_t)  ==  4, "uint32_t size != 4"  );
   apifiny_assert( sizeof(int32_t)   ==  4, "int32_t size != 4"   );
   apifiny_assert( sizeof(uint128_t) == 16, "uint128_t size != 16");
   apifiny_assert( sizeof(int128_t)  == 16, "int128_t size != 16" );
   apifiny_assert( sizeof(uint8_t)   ==  1, "uint8_t size != 1"   );

   apifiny_assert( sizeof(apifiny::name) ==  8, "name size !=  8");
}

void test_types::char_to_symbol() {

   apifiny_assert( apifiny::name::char_to_value('1') ==  1, "apifiny::char_to_symbol('1') !=  1" );
   apifiny_assert( apifiny::name::char_to_value('2') ==  2, "apifiny::char_to_symbol('2') !=  2" );
   apifiny_assert( apifiny::name::char_to_value('3') ==  3, "apifiny::char_to_symbol('3') !=  3" );
   apifiny_assert( apifiny::name::char_to_value('4') ==  4, "apifiny::char_to_symbol('4') !=  4" );
   apifiny_assert( apifiny::name::char_to_value('5') ==  5, "apifiny::char_to_symbol('5') !=  5" );
   apifiny_assert( apifiny::name::char_to_value('a') ==  6, "apifiny::char_to_symbol('a') !=  6" );
   apifiny_assert( apifiny::name::char_to_value('b') ==  7, "apifiny::char_to_symbol('b') !=  7" );
   apifiny_assert( apifiny::name::char_to_value('c') ==  8, "apifiny::char_to_symbol('c') !=  8" );
   apifiny_assert( apifiny::name::char_to_value('d') ==  9, "apifiny::char_to_symbol('d') !=  9" );
   apifiny_assert( apifiny::name::char_to_value('e') == 10, "apifiny::char_to_symbol('e') != 10" );
   apifiny_assert( apifiny::name::char_to_value('f') == 11, "apifiny::char_to_symbol('f') != 11" );
   apifiny_assert( apifiny::name::char_to_value('g') == 12, "apifiny::char_to_symbol('g') != 12" );
   apifiny_assert( apifiny::name::char_to_value('h') == 13, "apifiny::char_to_symbol('h') != 13" );
   apifiny_assert( apifiny::name::char_to_value('i') == 14, "apifiny::char_to_symbol('i') != 14" );
   apifiny_assert( apifiny::name::char_to_value('j') == 15, "apifiny::char_to_symbol('j') != 15" );
   apifiny_assert( apifiny::name::char_to_value('k') == 16, "apifiny::char_to_symbol('k') != 16" );
   apifiny_assert( apifiny::name::char_to_value('l') == 17, "apifiny::char_to_symbol('l') != 17" );
   apifiny_assert( apifiny::name::char_to_value('m') == 18, "apifiny::char_to_symbol('m') != 18" );
   apifiny_assert( apifiny::name::char_to_value('n') == 19, "apifiny::char_to_symbol('n') != 19" );
   apifiny_assert( apifiny::name::char_to_value('o') == 20, "apifiny::char_to_symbol('o') != 20" );
   apifiny_assert( apifiny::name::char_to_value('p') == 21, "apifiny::char_to_symbol('p') != 21" );
   apifiny_assert( apifiny::name::char_to_value('q') == 22, "apifiny::char_to_symbol('q') != 22" );
   apifiny_assert( apifiny::name::char_to_value('r') == 23, "apifiny::char_to_symbol('r') != 23" );
   apifiny_assert( apifiny::name::char_to_value('s') == 24, "apifiny::char_to_symbol('s') != 24" );
   apifiny_assert( apifiny::name::char_to_value('t') == 25, "apifiny::char_to_symbol('t') != 25" );
   apifiny_assert( apifiny::name::char_to_value('u') == 26, "apifiny::char_to_symbol('u') != 26" );
   apifiny_assert( apifiny::name::char_to_value('v') == 27, "apifiny::char_to_symbol('v') != 27" );
   apifiny_assert( apifiny::name::char_to_value('w') == 28, "apifiny::char_to_symbol('w') != 28" );
   apifiny_assert( apifiny::name::char_to_value('x') == 29, "apifiny::char_to_symbol('x') != 29" );
   apifiny_assert( apifiny::name::char_to_value('y') == 30, "apifiny::char_to_symbol('y') != 30" );
   apifiny_assert( apifiny::name::char_to_value('z') == 31, "apifiny::char_to_symbol('z') != 31" );

   for(unsigned char i = 0; i<255; i++) {
      if( (i >= 'a' && i <= 'z') || (i >= '1' || i <= '5') ) continue;
      apifiny_assert( apifiny::name::char_to_value((char)i) == 0, "apifiny::char_to_symbol() != 0" );
   }
}

void test_types::string_to_name() {
   return;
   apifiny_assert( apifiny::name("a") == "a"_n, "apifiny::string_to_name(a)" );
   apifiny_assert( apifiny::name("ba") == "ba"_n, "apifiny::string_to_name(ba)" );
   apifiny_assert( apifiny::name("cba") == "cba"_n, "apifiny::string_to_name(cba)" );
   apifiny_assert( apifiny::name("dcba") == "dcba"_n, "apifiny::string_to_name(dcba)" );
   apifiny_assert( apifiny::name("edcba") == "edcba"_n, "apifiny::string_to_name(edcba)" );
   apifiny_assert( apifiny::name("fedcba") == "fedcba"_n, "apifiny::string_to_name(fedcba)" );
   apifiny_assert( apifiny::name("gfedcba") == "gfedcba"_n, "apifiny::string_to_name(gfedcba)" );
   apifiny_assert( apifiny::name("hgfedcba") == "hgfedcba"_n, "apifiny::string_to_name(hgfedcba)" );
   apifiny_assert( apifiny::name("ihgfedcba") == "ihgfedcba"_n, "apifiny::string_to_name(ihgfedcba)" );
   apifiny_assert( apifiny::name("jihgfedcba") == "jihgfedcba"_n, "apifiny::string_to_name(jihgfedcba)" );
   apifiny_assert( apifiny::name("kjihgfedcba") == "kjihgfedcba"_n, "apifiny::string_to_name(kjihgfedcba)" );
   apifiny_assert( apifiny::name("lkjihgfedcba") == "lkjihgfedcba"_n, "apifiny::string_to_name(lkjihgfedcba)" );
   apifiny_assert( apifiny::name("mlkjihgfedcba") == "mlkjihgfedcba"_n, "apifiny::string_to_name(mlkjihgfedcba)" );
}
