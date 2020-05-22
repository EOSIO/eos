#include <eosio/eosio.hpp>

#include "test_api.hpp"

void test_types::types_size() {

   eosio_assert( sizeof(int64_t)   ==  8, "int64_t size != 8"   );
   eosio_assert( sizeof(uint64_t)  ==  8, "uint64_t size != 8"  );
   eosio_assert( sizeof(uint32_t)  ==  4, "uint32_t size != 4"  );
   eosio_assert( sizeof(int32_t)   ==  4, "int32_t size != 4"   );
   eosio_assert( sizeof(uint128_t) == 16, "uint128_t size != 16");
   eosio_assert( sizeof(int128_t)  == 16, "int128_t size != 16" );
   eosio_assert( sizeof(uint8_t)   ==  1, "uint8_t size != 1"   );

   eosio_assert( sizeof(eosio::name) ==  8, "name size !=  8");
}

void test_types::char_to_symbol() {

   eosio_assert( eosio::name::char_to_value('1') ==  1, "eosio::char_to_symbol('1') !=  1" );
   eosio_assert( eosio::name::char_to_value('2') ==  2, "eosio::char_to_symbol('2') !=  2" );
   eosio_assert( eosio::name::char_to_value('3') ==  3, "eosio::char_to_symbol('3') !=  3" );
   eosio_assert( eosio::name::char_to_value('4') ==  4, "eosio::char_to_symbol('4') !=  4" );
   eosio_assert( eosio::name::char_to_value('5') ==  5, "eosio::char_to_symbol('5') !=  5" );
   eosio_assert( eosio::name::char_to_value('a') ==  6, "eosio::char_to_symbol('a') !=  6" );
   eosio_assert( eosio::name::char_to_value('b') ==  7, "eosio::char_to_symbol('b') !=  7" );
   eosio_assert( eosio::name::char_to_value('c') ==  8, "eosio::char_to_symbol('c') !=  8" );
   eosio_assert( eosio::name::char_to_value('d') ==  9, "eosio::char_to_symbol('d') !=  9" );
   eosio_assert( eosio::name::char_to_value('e') == 10, "eosio::char_to_symbol('e') != 10" );
   eosio_assert( eosio::name::char_to_value('f') == 11, "eosio::char_to_symbol('f') != 11" );
   eosio_assert( eosio::name::char_to_value('g') == 12, "eosio::char_to_symbol('g') != 12" );
   eosio_assert( eosio::name::char_to_value('h') == 13, "eosio::char_to_symbol('h') != 13" );
   eosio_assert( eosio::name::char_to_value('i') == 14, "eosio::char_to_symbol('i') != 14" );
   eosio_assert( eosio::name::char_to_value('j') == 15, "eosio::char_to_symbol('j') != 15" );
   eosio_assert( eosio::name::char_to_value('k') == 16, "eosio::char_to_symbol('k') != 16" );
   eosio_assert( eosio::name::char_to_value('l') == 17, "eosio::char_to_symbol('l') != 17" );
   eosio_assert( eosio::name::char_to_value('m') == 18, "eosio::char_to_symbol('m') != 18" );
   eosio_assert( eosio::name::char_to_value('n') == 19, "eosio::char_to_symbol('n') != 19" );
   eosio_assert( eosio::name::char_to_value('o') == 20, "eosio::char_to_symbol('o') != 20" );
   eosio_assert( eosio::name::char_to_value('p') == 21, "eosio::char_to_symbol('p') != 21" );
   eosio_assert( eosio::name::char_to_value('q') == 22, "eosio::char_to_symbol('q') != 22" );
   eosio_assert( eosio::name::char_to_value('r') == 23, "eosio::char_to_symbol('r') != 23" );
   eosio_assert( eosio::name::char_to_value('s') == 24, "eosio::char_to_symbol('s') != 24" );
   eosio_assert( eosio::name::char_to_value('t') == 25, "eosio::char_to_symbol('t') != 25" );
   eosio_assert( eosio::name::char_to_value('u') == 26, "eosio::char_to_symbol('u') != 26" );
   eosio_assert( eosio::name::char_to_value('v') == 27, "eosio::char_to_symbol('v') != 27" );
   eosio_assert( eosio::name::char_to_value('w') == 28, "eosio::char_to_symbol('w') != 28" );
   eosio_assert( eosio::name::char_to_value('x') == 29, "eosio::char_to_symbol('x') != 29" );
   eosio_assert( eosio::name::char_to_value('y') == 30, "eosio::char_to_symbol('y') != 30" );
   eosio_assert( eosio::name::char_to_value('z') == 31, "eosio::char_to_symbol('z') != 31" );

   for(unsigned char i = 0; i<255; i++) {
      if( (i >= 'a' && i <= 'z') || (i >= '1' || i <= '5') ) continue;
      eosio_assert( eosio::name::char_to_value((char)i) == 0, "eosio::char_to_symbol() != 0" );
   }
}

void test_types::string_to_name() {
   return;
   eosio_assert( eosio::name("a") == "a"_n, "eosio::string_to_name(a)" );
   eosio_assert( eosio::name("ba") == "ba"_n, "eosio::string_to_name(ba)" );
   eosio_assert( eosio::name("cba") == "cba"_n, "eosio::string_to_name(cba)" );
   eosio_assert( eosio::name("dcba") == "dcba"_n, "eosio::string_to_name(dcba)" );
   eosio_assert( eosio::name("edcba") == "edcba"_n, "eosio::string_to_name(edcba)" );
   eosio_assert( eosio::name("fedcba") == "fedcba"_n, "eosio::string_to_name(fedcba)" );
   eosio_assert( eosio::name("gfedcba") == "gfedcba"_n, "eosio::string_to_name(gfedcba)" );
   eosio_assert( eosio::name("hgfedcba") == "hgfedcba"_n, "eosio::string_to_name(hgfedcba)" );
   eosio_assert( eosio::name("ihgfedcba") == "ihgfedcba"_n, "eosio::string_to_name(ihgfedcba)" );
   eosio_assert( eosio::name("jihgfedcba") == "jihgfedcba"_n, "eosio::string_to_name(jihgfedcba)" );
   eosio_assert( eosio::name("kjihgfedcba") == "kjihgfedcba"_n, "eosio::string_to_name(kjihgfedcba)" );
   eosio_assert( eosio::name("lkjihgfedcba") == "lkjihgfedcba"_n, "eosio::string_to_name(lkjihgfedcba)" );
   eosio_assert( eosio::name("mlkjihgfedcba") == "mlkjihgfedcba"_n, "eosio::string_to_name(mlkjihgfedcba)" );
}
