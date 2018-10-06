/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */
#include <arisenlib/arisen.hpp>

#include "test_api.hpp"

void test_types::types_size() {

   arisen_assert( sizeof(int64_t) == 8, "int64_t size != 8");
   arisen_assert( sizeof(uint64_t) ==  8, "uint64_t size != 8");
   arisen_assert( sizeof(uint32_t) ==  4, "uint32_t size != 4");
   arisen_assert( sizeof(int32_t) ==  4, "int32_t size != 4");
   arisen_assert( sizeof(uint128_t) == 16, "uint128_t size != 16");
   arisen_assert( sizeof(int128_t) == 16, "int128_t size != 16");
   arisen_assert( sizeof(uint8_t) ==  1, "uint8_t size != 1");

   arisen_assert( sizeof(account_name) ==  8, "account_name size !=  8");
   arisen_assert( sizeof(table_name) ==  8, "table_name size !=  8");
   arisen_assert( sizeof(time) ==  4, "time size !=  4");
   arisen_assert( sizeof(arisen::key256) == 32, "key256 size != 32" );
}

void test_types::char_to_symbol() {

   arisen_assert( arisen::char_to_symbol('1') ==  1, "arisen::char_to_symbol('1') !=  1");
   arisen_assert( arisen::char_to_symbol('2') ==  2, "arisen::char_to_symbol('2') !=  2");
   arisen_assert( arisen::char_to_symbol('3') ==  3, "arisen::char_to_symbol('3') !=  3");
   arisen_assert( arisen::char_to_symbol('4') ==  4, "arisen::char_to_symbol('4') !=  4");
   arisen_assert( arisen::char_to_symbol('5') ==  5, "arisen::char_to_symbol('5') !=  5");
   arisen_assert( arisen::char_to_symbol('a') ==  6, "arisen::char_to_symbol('a') !=  6");
   arisen_assert( arisen::char_to_symbol('b') ==  7, "arisen::char_to_symbol('b') !=  7");
   arisen_assert( arisen::char_to_symbol('c') ==  8, "arisen::char_to_symbol('c') !=  8");
   arisen_assert( arisen::char_to_symbol('d') ==  9, "arisen::char_to_symbol('d') !=  9");
   arisen_assert( arisen::char_to_symbol('e') == 10, "arisen::char_to_symbol('e') != 10");
   arisen_assert( arisen::char_to_symbol('f') == 11, "arisen::char_to_symbol('f') != 11");
   arisen_assert( arisen::char_to_symbol('g') == 12, "arisen::char_to_symbol('g') != 12");
   arisen_assert( arisen::char_to_symbol('h') == 13, "arisen::char_to_symbol('h') != 13");
   arisen_assert( arisen::char_to_symbol('i') == 14, "arisen::char_to_symbol('i') != 14");
   arisen_assert( arisen::char_to_symbol('j') == 15, "arisen::char_to_symbol('j') != 15");
   arisen_assert( arisen::char_to_symbol('k') == 16, "arisen::char_to_symbol('k') != 16");
   arisen_assert( arisen::char_to_symbol('l') == 17, "arisen::char_to_symbol('l') != 17");
   arisen_assert( arisen::char_to_symbol('m') == 18, "arisen::char_to_symbol('m') != 18");
   arisen_assert( arisen::char_to_symbol('n') == 19, "arisen::char_to_symbol('n') != 19");
   arisen_assert( arisen::char_to_symbol('o') == 20, "arisen::char_to_symbol('o') != 20");
   arisen_assert( arisen::char_to_symbol('p') == 21, "arisen::char_to_symbol('p') != 21");
   arisen_assert( arisen::char_to_symbol('q') == 22, "arisen::char_to_symbol('q') != 22");
   arisen_assert( arisen::char_to_symbol('r') == 23, "arisen::char_to_symbol('r') != 23");
   arisen_assert( arisen::char_to_symbol('s') == 24, "arisen::char_to_symbol('s') != 24");
   arisen_assert( arisen::char_to_symbol('t') == 25, "arisen::char_to_symbol('t') != 25");
   arisen_assert( arisen::char_to_symbol('u') == 26, "arisen::char_to_symbol('u') != 26");
   arisen_assert( arisen::char_to_symbol('v') == 27, "arisen::char_to_symbol('v') != 27");
   arisen_assert( arisen::char_to_symbol('w') == 28, "arisen::char_to_symbol('w') != 28");
   arisen_assert( arisen::char_to_symbol('x') == 29, "arisen::char_to_symbol('x') != 29");
   arisen_assert( arisen::char_to_symbol('y') == 30, "arisen::char_to_symbol('y') != 30");
   arisen_assert( arisen::char_to_symbol('z') == 31, "arisen::char_to_symbol('z') != 31");

   for(unsigned char i = 0; i<255; i++) {
      if((i >= 'a' && i <= 'z') || (i >= '1' || i <= '5')) continue;
      arisen_assert( arisen::char_to_symbol((char)i) == 0, "arisen::char_to_symbol() != 0");
   }
}

void test_types::string_to_name() {

   arisen_assert( arisen::string_to_name("a") == N(a) , "arisen::string_to_name(a)" );
   arisen_assert( arisen::string_to_name("ba") == N(ba) , "arisen::string_to_name(ba)" );
   arisen_assert( arisen::string_to_name("cba") == N(cba) , "arisen::string_to_name(cba)" );
   arisen_assert( arisen::string_to_name("dcba") == N(dcba) , "arisen::string_to_name(dcba)" );
   arisen_assert( arisen::string_to_name("edcba") == N(edcba) , "arisen::string_to_name(edcba)" );
   arisen_assert( arisen::string_to_name("fedcba") == N(fedcba) , "arisen::string_to_name(fedcba)" );
   arisen_assert( arisen::string_to_name("gfedcba") == N(gfedcba) , "arisen::string_to_name(gfedcba)" );
   arisen_assert( arisen::string_to_name("hgfedcba") == N(hgfedcba) , "arisen::string_to_name(hgfedcba)" );
   arisen_assert( arisen::string_to_name("ihgfedcba") == N(ihgfedcba) , "arisen::string_to_name(ihgfedcba)" );
   arisen_assert( arisen::string_to_name("jihgfedcba") == N(jihgfedcba) , "arisen::string_to_name(jihgfedcba)" );
   arisen_assert( arisen::string_to_name("kjihgfedcba") == N(kjihgfedcba) , "arisen::string_to_name(kjihgfedcba)" );
   arisen_assert( arisen::string_to_name("lkjihgfedcba") == N(lkjihgfedcba) , "arisen::string_to_name(lkjihgfedcba)" );
   arisen_assert( arisen::string_to_name("mlkjihgfedcba") == N(mlkjihgfedcba) , "arisen::string_to_name(mlkjihgfedcba)" );
   arisen_assert( arisen::string_to_name("mlkjihgfedcba1") == N(mlkjihgfedcba2) , "arisen::string_to_name(mlkjihgfedcba2)" );
   arisen_assert( arisen::string_to_name("mlkjihgfedcba55") == N(mlkjihgfedcba14) , "arisen::string_to_name(mlkjihgfedcba14)" );

   arisen_assert( arisen::string_to_name("azAA34") == N(azBB34) , "arisen::string_to_name N(azBB34)" );
   arisen_assert( arisen::string_to_name("AZaz12Bc34") == N(AZaz12Bc34) , "arisen::string_to_name AZaz12Bc34" );
   arisen_assert( arisen::string_to_name("AAAAAAAAAAAAAAA") == arisen::string_to_name("BBBBBBBBBBBBBDDDDDFFFGG") , "arisen::string_to_name BBBBBBBBBBBBBDDDDDFFFGG" );
}

void test_types::name_class() {

   arisen_assert( arisen::name{arisen::string_to_name("azAA34")}.value == N(azAA34), "arisen::name != N(azAA34)" );
   arisen_assert( arisen::name{arisen::string_to_name("AABBCC")}.value == 0, "arisen::name != N(0)" );
   arisen_assert( arisen::name{arisen::string_to_name("AA11")}.value == N(AA11), "arisen::name != N(AA11)" );
   arisen_assert( arisen::name{arisen::string_to_name("11AA")}.value == N(11), "arisen::name != N(11)" );
   arisen_assert( arisen::name{arisen::string_to_name("22BBCCXXAA")}.value == N(22), "arisen::name != N(22)" );
   arisen_assert( arisen::name{arisen::string_to_name("AAAbbcccdd")} == arisen::name{arisen::string_to_name("AAAbbcccdd")}, "arisen::name == arisen::name" );

   uint64_t tmp = arisen::name{arisen::string_to_name("11bbcccdd")};
   arisen_assert(N(11bbcccdd) == tmp, "N(11bbcccdd) == tmp");
}
