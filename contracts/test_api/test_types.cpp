/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>

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
   eosio_assert( sizeof(eosio::key256) == 32, "key256 size != 32" );
}

void test_types::char_to_symbol() {

   eosio_assert( eosio::char_to_symbol('1') ==  1, "eosio::char_to_symbol('1') !=  1");
   eosio_assert( eosio::char_to_symbol('2') ==  2, "eosio::char_to_symbol('2') !=  2");
   eosio_assert( eosio::char_to_symbol('3') ==  3, "eosio::char_to_symbol('3') !=  3");
   eosio_assert( eosio::char_to_symbol('4') ==  4, "eosio::char_to_symbol('4') !=  4");
   eosio_assert( eosio::char_to_symbol('5') ==  5, "eosio::char_to_symbol('5') !=  5");
   eosio_assert( eosio::char_to_symbol('a') ==  6, "eosio::char_to_symbol('a') !=  6");
   eosio_assert( eosio::char_to_symbol('b') ==  7, "eosio::char_to_symbol('b') !=  7");
   eosio_assert( eosio::char_to_symbol('c') ==  8, "eosio::char_to_symbol('c') !=  8");
   eosio_assert( eosio::char_to_symbol('d') ==  9, "eosio::char_to_symbol('d') !=  9");
   eosio_assert( eosio::char_to_symbol('e') == 10, "eosio::char_to_symbol('e') != 10");
   eosio_assert( eosio::char_to_symbol('f') == 11, "eosio::char_to_symbol('f') != 11");
   eosio_assert( eosio::char_to_symbol('g') == 12, "eosio::char_to_symbol('g') != 12");
   eosio_assert( eosio::char_to_symbol('h') == 13, "eosio::char_to_symbol('h') != 13");
   eosio_assert( eosio::char_to_symbol('i') == 14, "eosio::char_to_symbol('i') != 14");
   eosio_assert( eosio::char_to_symbol('j') == 15, "eosio::char_to_symbol('j') != 15");
   eosio_assert( eosio::char_to_symbol('k') == 16, "eosio::char_to_symbol('k') != 16");
   eosio_assert( eosio::char_to_symbol('l') == 17, "eosio::char_to_symbol('l') != 17");
   eosio_assert( eosio::char_to_symbol('m') == 18, "eosio::char_to_symbol('m') != 18");
   eosio_assert( eosio::char_to_symbol('n') == 19, "eosio::char_to_symbol('n') != 19");
   eosio_assert( eosio::char_to_symbol('o') == 20, "eosio::char_to_symbol('o') != 20");
   eosio_assert( eosio::char_to_symbol('p') == 21, "eosio::char_to_symbol('p') != 21");
   eosio_assert( eosio::char_to_symbol('q') == 22, "eosio::char_to_symbol('q') != 22");
   eosio_assert( eosio::char_to_symbol('r') == 23, "eosio::char_to_symbol('r') != 23");
   eosio_assert( eosio::char_to_symbol('s') == 24, "eosio::char_to_symbol('s') != 24");
   eosio_assert( eosio::char_to_symbol('t') == 25, "eosio::char_to_symbol('t') != 25");
   eosio_assert( eosio::char_to_symbol('u') == 26, "eosio::char_to_symbol('u') != 26");
   eosio_assert( eosio::char_to_symbol('v') == 27, "eosio::char_to_symbol('v') != 27");
   eosio_assert( eosio::char_to_symbol('w') == 28, "eosio::char_to_symbol('w') != 28");
   eosio_assert( eosio::char_to_symbol('x') == 29, "eosio::char_to_symbol('x') != 29");
   eosio_assert( eosio::char_to_symbol('y') == 30, "eosio::char_to_symbol('y') != 30");
   eosio_assert( eosio::char_to_symbol('z') == 31, "eosio::char_to_symbol('z') != 31");

   for(unsigned char i = 0; i<255; i++) {
      if((i >= 'a' && i <= 'z') || (i >= '1' || i <= '5')) continue;
      eosio_assert( eosio::char_to_symbol((char)i) == 0, "eosio::char_to_symbol() != 0");
   }
}

void test_types::string_to_name() {

   eosio_assert( eosio::string_to_name("a") == N(a) , "eosio::string_to_name(a)" );
   eosio_assert( eosio::string_to_name("ba") == N(ba) , "eosio::string_to_name(ba)" );
   eosio_assert( eosio::string_to_name("cba") == N(cba) , "eosio::string_to_name(cba)" );
   eosio_assert( eosio::string_to_name("dcba") == N(dcba) , "eosio::string_to_name(dcba)" );
   eosio_assert( eosio::string_to_name("edcba") == N(edcba) , "eosio::string_to_name(edcba)" );
   eosio_assert( eosio::string_to_name("fedcba") == N(fedcba) , "eosio::string_to_name(fedcba)" );
   eosio_assert( eosio::string_to_name("gfedcba") == N(gfedcba) , "eosio::string_to_name(gfedcba)" );
   eosio_assert( eosio::string_to_name("hgfedcba") == N(hgfedcba) , "eosio::string_to_name(hgfedcba)" );
   eosio_assert( eosio::string_to_name("ihgfedcba") == N(ihgfedcba) , "eosio::string_to_name(ihgfedcba)" );
   eosio_assert( eosio::string_to_name("jihgfedcba") == N(jihgfedcba) , "eosio::string_to_name(jihgfedcba)" );
   eosio_assert( eosio::string_to_name("kjihgfedcba") == N(kjihgfedcba) , "eosio::string_to_name(kjihgfedcba)" );
   eosio_assert( eosio::string_to_name("lkjihgfedcba") == N(lkjihgfedcba) , "eosio::string_to_name(lkjihgfedcba)" );
   eosio_assert( eosio::string_to_name("mlkjihgfedcba") == N(mlkjihgfedcba) , "eosio::string_to_name(mlkjihgfedcba)" );
   eosio_assert( eosio::string_to_name("mlkjihgfedcba1") == N(mlkjihgfedcba2) , "eosio::string_to_name(mlkjihgfedcba2)" );
   eosio_assert( eosio::string_to_name("mlkjihgfedcba55") == N(mlkjihgfedcba14) , "eosio::string_to_name(mlkjihgfedcba14)" );

   eosio_assert( eosio::string_to_name("azAA34") == N(azBB34) , "eosio::string_to_name N(azBB34)" );
   eosio_assert( eosio::string_to_name("AZaz12Bc34") == N(AZaz12Bc34) , "eosio::string_to_name AZaz12Bc34" );
   eosio_assert( eosio::string_to_name("AAAAAAAAAAAAAAA") == eosio::string_to_name("BBBBBBBBBBBBBDDDDDFFFGG") , "eosio::string_to_name BBBBBBBBBBBBBDDDDDFFFGG" );
}

void test_types::name_mask() {

   eosio_assert( eosio::name_mask(0) == 0b0000000000000000000000000000000000000000000000000000000000000000, "eosio::name_mask(0)" );
   eosio_assert( eosio::name_mask(1) == 0b1111100000000000000000000000000000000000000000000000000000000000, "eosio::name_mask(1)" );
   eosio_assert( eosio::name_mask(2) == 0b1111111111000000000000000000000000000000000000000000000000000000, "eosio::name_mask(2)" );
   eosio_assert( eosio::name_mask(3) == 0b1111111111111110000000000000000000000000000000000000000000000000, "eosio::name_mask(3)" );
   eosio_assert( eosio::name_mask(4) == 0b1111111111111111111100000000000000000000000000000000000000000000, "eosio::name_mask(4)" );
   eosio_assert( eosio::name_mask(5) == 0b1111111111111111111111111000000000000000000000000000000000000000, "eosio::name_mask(5)" );
   eosio_assert( eosio::name_mask(6) == 0b1111111111111111111111111111110000000000000000000000000000000000, "eosio::name_mask(6)" );
   eosio_assert( eosio::name_mask(7) == 0b1111111111111111111111111111111111100000000000000000000000000000, "eosio::name_mask(7)" );
   eosio_assert( eosio::name_mask(8) == 0b1111111111111111111111111111111111111111000000000000000000000000, "eosio::name_mask(8)" );
   eosio_assert( eosio::name_mask(9) == 0b1111111111111111111111111111111111111111111110000000000000000000, "eosio::name_mask(9)" );
   eosio_assert( eosio::name_mask(10) == 0b1111111111111111111111111111111111111111111111111100000000000000, "eosio::name_mask(10)" );
   eosio_assert( eosio::name_mask(11) == 0b1111111111111111111111111111111111111111111111111111111000000000, "eosio::name_mask(11)" );
   eosio_assert( eosio::name_mask(12) == 0b1111111111111111111111111111111111111111111111111111111111110000, "eosio::name_mask(12)" );
   eosio_assert( eosio::name_mask(13) == 0b1111111111111111111111111111111111111111111111111111111111111111, "eosio::name_mask(13)" );

}

void test_types::name_length() {

   eosio_assert( eosio::name_length(N(.)) == 0, "eosio::name_length(N(.))" );
   eosio_assert( eosio::name_length(N(a)) == 1, "eosio::name_length(N(a))" );
   eosio_assert( eosio::name_length(N(ab)) == 2, "eosio::name_length(N(ab))" );
   eosio_assert( eosio::name_length(N(abc)) == 3, "eosio::name_length(N(abc))" );
   eosio_assert( eosio::name_length(N(abcd)) == 4, "eosio::name_length(N(abcd))" );
   eosio_assert( eosio::name_length(N(abcde)) == 5, "eosio::name_length(N(abcde))" );
   eosio_assert( eosio::name_length(N(abcdef)) == 6, "eosio::name_length(N(abcdef))" );
   eosio_assert( eosio::name_length(N(abcdefg)) == 7, "eosio::name_length(N(abcdefg))" );
   eosio_assert( eosio::name_length(N(abcdefgh)) == 8, "eosio::name_length(N(abcdefgh))" );
   eosio_assert( eosio::name_length(N(abcdefghi)) == 9, "eosio::name_length(N(abcdefghi))" );
   eosio_assert( eosio::name_length(N(abcdefghij)) == 10, "eosio::name_length(N(abcdefghij))" );
   eosio_assert( eosio::name_length(N(abcdefghijk)) == 11, "eosio::name_length(N(abcdefghijk))" );
   eosio_assert( eosio::name_length(N(abcdefghijkl)) == 12, "eosio::name_length(N(abcdefghijkl))" );

   eosio_assert( eosio::name_length(N(a..d.esfas)) == 10, "eosio::name_length(N(a..d.esfas))" );
   eosio_assert( eosio::name_length(N(abcd.esfas)) == 10, "eosio::name_length(N(abcd.esfas))" );
   eosio_assert( eosio::name_length(N(abcd.asdfes)) == 11, "eosio::name_length(N(abcd.asdfes))" );
   eosio_assert( eosio::name_length(N(abcdefgh.jkl)) == 12, "eosio::name_length(N(abcdefgh.jkl))" );
   eosio_assert( eosio::name_length(N(ab.def.hijkl)) == 12, "eosio::name_length(N(ab.def.hijkl))" );

}

void test_types::name_sub() {

   eosio_assert( eosio::name_sub(N(abcdef), 0, 1) == N(a), "eosio::name_sub(N(abcdef), 0, 1)" );
   eosio_assert( eosio::name_sub(N(abcdef), 1, 1) == N(b), "eosio::name_sub(N(abcdef), 1, 1)" );
   eosio_assert( eosio::name_sub(N(abcdef), 2, 1) == N(c), "eosio::name_sub(N(abcdef), 2, 1)" );

   eosio_assert( eosio::name_sub(N(abcdef), 0, 3) == N(abc), "eosio::name_sub(N(abcdef), 0, 3)" );
   eosio_assert( eosio::name_sub(N(abcdef), 1, 3) == N(bcd), "eosio::name_sub(N(abcdef), 1, 3)" );
   eosio_assert( eosio::name_sub(N(abcdef), 2, 3) == N(cde), "eosio::name_sub(N(abcdef), 2, 3)" );
   eosio_assert( eosio::name_sub(N(abcdef), 3, 3) == N(def), "eosio::name_sub(N(abcdef), 3, 3)" );
   eosio_assert( eosio::name_sub(N(abcdef), 4, 3) == N(ef), "eosio::name_sub(N(abcdef), 4, 3)" );

}

void test_types::name_class() {

   eosio_assert( eosio::name{eosio::string_to_name("azAA34")}.value == N(azAA34), "eosio::name != N(azAA34)" );
   eosio_assert( eosio::name{eosio::string_to_name("AABBCC")}.value == 0, "eosio::name != N(0)" );
   eosio_assert( eosio::name{eosio::string_to_name("AA11")}.value == N(AA11), "eosio::name != N(AA11)" );
   eosio_assert( eosio::name{eosio::string_to_name("11AA")}.value == N(11), "eosio::name != N(11)" );
   eosio_assert( eosio::name{eosio::string_to_name("22BBCCXXAA")}.value == N(22), "eosio::name != N(22)" );
   eosio_assert( eosio::name{eosio::string_to_name("AAAbbcccdd")} == eosio::name{eosio::string_to_name("AAAbbcccdd")}, "eosio::name == eosio::name" );

   uint64_t tmp = eosio::name{eosio::string_to_name("11bbcccdd")};
   eosio_assert(N(11bbcccdd) == tmp, "N(11bbcccdd) == tmp");
}
