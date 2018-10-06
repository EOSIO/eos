/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */
#include <arisenlib/eosio.hpp>

#include "test_api.hpp"

void test_types::types_size() {

   eosio_assert( sizeof(int64_t) == 8, "int64_t size != 8");
   eosio_assert( sizeof(uint64_t) ==  8, "uint64_t size != 8");
   eosio_assert( sizeof(uint32_t) ==  4, "uint32_t size != 4");
   eosio_assert( sizeof(int32_t) ==  4, "int32_t size != 4");
   eosio_assert( sizeof(uint128_t) == 16, "uint128_t size != 16");
   eosio_assert( sizeof(int128_t) == 16, "int128_t size != 16");
   eosio_assert( sizeof(uint8_t) ==  1, "uint8_t size != 1");

   eosio_assert( sizeof(account_name) ==  8, "account_name size !=  8");
   eosio_assert( sizeof(table_name) ==  8, "table_name size !=  8");
   eosio_assert( sizeof(time) ==  4, "time size !=  4");
   eosio_assert( sizeof(arisen::key256) == 32, "key256 size != 32" );
}

void test_types::char_to_symbol() {

   eosio_assert( arisen::char_to_symbol('1') ==  1, "arisen::char_to_symbol('1') !=  1");
   eosio_assert( arisen::char_to_symbol('2') ==  2, "arisen::char_to_symbol('2') !=  2");
   eosio_assert( arisen::char_to_symbol('3') ==  3, "arisen::char_to_symbol('3') !=  3");
   eosio_assert( arisen::char_to_symbol('4') ==  4, "arisen::char_to_symbol('4') !=  4");
   eosio_assert( arisen::char_to_symbol('5') ==  5, "arisen::char_to_symbol('5') !=  5");
   eosio_assert( arisen::char_to_symbol('a') ==  6, "arisen::char_to_symbol('a') !=  6");
   eosio_assert( arisen::char_to_symbol('b') ==  7, "arisen::char_to_symbol('b') !=  7");
   eosio_assert( arisen::char_to_symbol('c') ==  8, "arisen::char_to_symbol('c') !=  8");
   eosio_assert( arisen::char_to_symbol('d') ==  9, "arisen::char_to_symbol('d') !=  9");
   eosio_assert( arisen::char_to_symbol('e') == 10, "arisen::char_to_symbol('e') != 10");
   eosio_assert( arisen::char_to_symbol('f') == 11, "arisen::char_to_symbol('f') != 11");
   eosio_assert( arisen::char_to_symbol('g') == 12, "arisen::char_to_symbol('g') != 12");
   eosio_assert( arisen::char_to_symbol('h') == 13, "arisen::char_to_symbol('h') != 13");
   eosio_assert( arisen::char_to_symbol('i') == 14, "arisen::char_to_symbol('i') != 14");
   eosio_assert( arisen::char_to_symbol('j') == 15, "arisen::char_to_symbol('j') != 15");
   eosio_assert( arisen::char_to_symbol('k') == 16, "arisen::char_to_symbol('k') != 16");
   eosio_assert( arisen::char_to_symbol('l') == 17, "arisen::char_to_symbol('l') != 17");
   eosio_assert( arisen::char_to_symbol('m') == 18, "arisen::char_to_symbol('m') != 18");
   eosio_assert( arisen::char_to_symbol('n') == 19, "arisen::char_to_symbol('n') != 19");
   eosio_assert( arisen::char_to_symbol('o') == 20, "arisen::char_to_symbol('o') != 20");
   eosio_assert( arisen::char_to_symbol('p') == 21, "arisen::char_to_symbol('p') != 21");
   eosio_assert( arisen::char_to_symbol('q') == 22, "arisen::char_to_symbol('q') != 22");
   eosio_assert( arisen::char_to_symbol('r') == 23, "arisen::char_to_symbol('r') != 23");
   eosio_assert( arisen::char_to_symbol('s') == 24, "arisen::char_to_symbol('s') != 24");
   eosio_assert( arisen::char_to_symbol('t') == 25, "arisen::char_to_symbol('t') != 25");
   eosio_assert( arisen::char_to_symbol('u') == 26, "arisen::char_to_symbol('u') != 26");
   eosio_assert( arisen::char_to_symbol('v') == 27, "arisen::char_to_symbol('v') != 27");
   eosio_assert( arisen::char_to_symbol('w') == 28, "arisen::char_to_symbol('w') != 28");
   eosio_assert( arisen::char_to_symbol('x') == 29, "arisen::char_to_symbol('x') != 29");
   eosio_assert( arisen::char_to_symbol('y') == 30, "arisen::char_to_symbol('y') != 30");
   eosio_assert( arisen::char_to_symbol('z') == 31, "arisen::char_to_symbol('z') != 31");

   for(unsigned char i = 0; i<255; i++) {
      if((i >= 'a' && i <= 'z') || (i >= '1' || i <= '5')) continue;
      eosio_assert( arisen::char_to_symbol((char)i) == 0, "arisen::char_to_symbol() != 0");
   }
}

void test_types::string_to_name() {

   eosio_assert( arisen::string_to_name("a") == N(a) , "arisen::string_to_name(a)" );
   eosio_assert( arisen::string_to_name("ba") == N(ba) , "arisen::string_to_name(ba)" );
   eosio_assert( arisen::string_to_name("cba") == N(cba) , "arisen::string_to_name(cba)" );
   eosio_assert( arisen::string_to_name("dcba") == N(dcba) , "arisen::string_to_name(dcba)" );
   eosio_assert( arisen::string_to_name("edcba") == N(edcba) , "arisen::string_to_name(edcba)" );
   eosio_assert( arisen::string_to_name("fedcba") == N(fedcba) , "arisen::string_to_name(fedcba)" );
   eosio_assert( arisen::string_to_name("gfedcba") == N(gfedcba) , "arisen::string_to_name(gfedcba)" );
   eosio_assert( arisen::string_to_name("hgfedcba") == N(hgfedcba) , "arisen::string_to_name(hgfedcba)" );
   eosio_assert( arisen::string_to_name("ihgfedcba") == N(ihgfedcba) , "arisen::string_to_name(ihgfedcba)" );
   eosio_assert( arisen::string_to_name("jihgfedcba") == N(jihgfedcba) , "arisen::string_to_name(jihgfedcba)" );
   eosio_assert( arisen::string_to_name("kjihgfedcba") == N(kjihgfedcba) , "arisen::string_to_name(kjihgfedcba)" );
   eosio_assert( arisen::string_to_name("lkjihgfedcba") == N(lkjihgfedcba) , "arisen::string_to_name(lkjihgfedcba)" );
   eosio_assert( arisen::string_to_name("mlkjihgfedcba") == N(mlkjihgfedcba) , "arisen::string_to_name(mlkjihgfedcba)" );
   eosio_assert( arisen::string_to_name("mlkjihgfedcba1") == N(mlkjihgfedcba2) , "arisen::string_to_name(mlkjihgfedcba2)" );
   eosio_assert( arisen::string_to_name("mlkjihgfedcba55") == N(mlkjihgfedcba14) , "arisen::string_to_name(mlkjihgfedcba14)" );

   eosio_assert( arisen::string_to_name("azAA34") == N(azBB34) , "arisen::string_to_name N(azBB34)" );
   eosio_assert( arisen::string_to_name("AZaz12Bc34") == N(AZaz12Bc34) , "arisen::string_to_name AZaz12Bc34" );
   eosio_assert( arisen::string_to_name("AAAAAAAAAAAAAAA") == arisen::string_to_name("BBBBBBBBBBBBBDDDDDFFFGG") , "arisen::string_to_name BBBBBBBBBBBBBDDDDDFFFGG" );
}

void test_types::name_class() {

   eosio_assert( arisen::name{arisen::string_to_name("azAA34")}.value == N(azAA34), "arisen::name != N(azAA34)" );
   eosio_assert( arisen::name{arisen::string_to_name("AABBCC")}.value == 0, "arisen::name != N(0)" );
   eosio_assert( arisen::name{arisen::string_to_name("AA11")}.value == N(AA11), "arisen::name != N(AA11)" );
   eosio_assert( arisen::name{arisen::string_to_name("11AA")}.value == N(11), "arisen::name != N(11)" );
   eosio_assert( arisen::name{arisen::string_to_name("22BBCCXXAA")}.value == N(22), "arisen::name != N(22)" );
   eosio_assert( arisen::name{arisen::string_to_name("AAAbbcccdd")} == arisen::name{arisen::string_to_name("AAAbbcccdd")}, "arisen::name == arisen::name" );

   uint64_t tmp = arisen::name{arisen::string_to_name("11bbcccdd")};
   eosio_assert(N(11bbcccdd) == tmp, "N(11bbcccdd) == tmp");
}
