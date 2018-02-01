/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eos.hpp>

#include "test_api.hpp"

unsigned int test_types::types_size() {
   
   WASM_ASSERT( sizeof(int64_t) == 8, "int64_t size != 8");
   WASM_ASSERT( sizeof(uint64_t) ==  8, "uint64_t size != 8");
   WASM_ASSERT( sizeof(uint32_t) ==  4, "uint32_t size != 4");
   WASM_ASSERT( sizeof(int32_t) ==  4, "int32_t size != 4");
   WASM_ASSERT( sizeof(uint128_t) == 16, "uint128_t size != 16");
   WASM_ASSERT( sizeof(int128_t) == 16, "int128_t size != 16");
   WASM_ASSERT( sizeof(uint8_t) ==  1, "uint8_t size != 1");
   
   WASM_ASSERT( sizeof(account_name) ==  8, "account_name size !=  8");
   WASM_ASSERT( sizeof(token_name) ==  8, "token_name size !=  8");
   WASM_ASSERT( sizeof(table_name) ==  8, "table_name size !=  8");
   WASM_ASSERT( sizeof(time) ==  4, "time size !=  4");
   WASM_ASSERT( sizeof(uint256) == 32, "uint256 != 32" );

   return WASM_TEST_PASS;
}

unsigned int test_types::char_to_symbol() {
   
   WASM_ASSERT( eosio::char_to_symbol('1') ==  1, "eosio::char_to_symbol('1') !=  1");
   WASM_ASSERT( eosio::char_to_symbol('2') ==  2, "eosio::char_to_symbol('2') !=  2");
   WASM_ASSERT( eosio::char_to_symbol('3') ==  3, "eosio::char_to_symbol('3') !=  3");
   WASM_ASSERT( eosio::char_to_symbol('4') ==  4, "eosio::char_to_symbol('4') !=  4");
   WASM_ASSERT( eosio::char_to_symbol('5') ==  5, "eosio::char_to_symbol('5') !=  5");
   WASM_ASSERT( eosio::char_to_symbol('a') ==  6, "eosio::char_to_symbol('a') !=  6");
   WASM_ASSERT( eosio::char_to_symbol('b') ==  7, "eosio::char_to_symbol('b') !=  7");
   WASM_ASSERT( eosio::char_to_symbol('c') ==  8, "eosio::char_to_symbol('c') !=  8");
   WASM_ASSERT( eosio::char_to_symbol('d') ==  9, "eosio::char_to_symbol('d') !=  9");
   WASM_ASSERT( eosio::char_to_symbol('e') == 10, "eosio::char_to_symbol('e') != 10");
   WASM_ASSERT( eosio::char_to_symbol('f') == 11, "eosio::char_to_symbol('f') != 11");
   WASM_ASSERT( eosio::char_to_symbol('g') == 12, "eosio::char_to_symbol('g') != 12");
   WASM_ASSERT( eosio::char_to_symbol('h') == 13, "eosio::char_to_symbol('h') != 13");
   WASM_ASSERT( eosio::char_to_symbol('i') == 14, "eosio::char_to_symbol('i') != 14");
   WASM_ASSERT( eosio::char_to_symbol('j') == 15, "eosio::char_to_symbol('j') != 15");
   WASM_ASSERT( eosio::char_to_symbol('k') == 16, "eosio::char_to_symbol('k') != 16");
   WASM_ASSERT( eosio::char_to_symbol('l') == 17, "eosio::char_to_symbol('l') != 17");
   WASM_ASSERT( eosio::char_to_symbol('m') == 18, "eosio::char_to_symbol('m') != 18");
   WASM_ASSERT( eosio::char_to_symbol('n') == 19, "eosio::char_to_symbol('n') != 19");
   WASM_ASSERT( eosio::char_to_symbol('o') == 20, "eosio::char_to_symbol('o') != 20");
   WASM_ASSERT( eosio::char_to_symbol('p') == 21, "eosio::char_to_symbol('p') != 21");
   WASM_ASSERT( eosio::char_to_symbol('q') == 22, "eosio::char_to_symbol('q') != 22");
   WASM_ASSERT( eosio::char_to_symbol('r') == 23, "eosio::char_to_symbol('r') != 23");
   WASM_ASSERT( eosio::char_to_symbol('s') == 24, "eosio::char_to_symbol('s') != 24");
   WASM_ASSERT( eosio::char_to_symbol('t') == 25, "eosio::char_to_symbol('t') != 25");
   WASM_ASSERT( eosio::char_to_symbol('u') == 26, "eosio::char_to_symbol('u') != 26");
   WASM_ASSERT( eosio::char_to_symbol('v') == 27, "eosio::char_to_symbol('v') != 27");
   WASM_ASSERT( eosio::char_to_symbol('w') == 28, "eosio::char_to_symbol('w') != 28");
   WASM_ASSERT( eosio::char_to_symbol('x') == 29, "eosio::char_to_symbol('x') != 29");
   WASM_ASSERT( eosio::char_to_symbol('y') == 30, "eosio::char_to_symbol('y') != 30");
   WASM_ASSERT( eosio::char_to_symbol('z') == 31, "eosio::char_to_symbol('z') != 31");
 
   for(unsigned char i = 0; i<255; i++) {
      if((i >= 'a' && i <= 'z') || (i >= '1' || i <= '5')) continue;
      WASM_ASSERT( eosio::char_to_symbol(i) == 0, "eosio::char_to_symbol() != 0");
   }

   return WASM_TEST_PASS;  
}

unsigned int test_types::string_to_name() {

   WASM_ASSERT( eosio::string_to_name("a") == N(a) , "eosio::string_to_name(a)" );
   WASM_ASSERT( eosio::string_to_name("ba") == N(ba) , "eosio::string_to_name(ba)" );
   WASM_ASSERT( eosio::string_to_name("cba") == N(cba) , "eosio::string_to_name(cba)" );
   WASM_ASSERT( eosio::string_to_name("dcba") == N(dcba) , "eosio::string_to_name(dcba)" );
   WASM_ASSERT( eosio::string_to_name("edcba") == N(edcba) , "eosio::string_to_name(edcba)" );
   WASM_ASSERT( eosio::string_to_name("fedcba") == N(fedcba) , "eosio::string_to_name(fedcba)" );
   WASM_ASSERT( eosio::string_to_name("gfedcba") == N(gfedcba) , "eosio::string_to_name(gfedcba)" );
   WASM_ASSERT( eosio::string_to_name("hgfedcba") == N(hgfedcba) , "eosio::string_to_name(hgfedcba)" );
   WASM_ASSERT( eosio::string_to_name("ihgfedcba") == N(ihgfedcba) , "eosio::string_to_name(ihgfedcba)" );
   WASM_ASSERT( eosio::string_to_name("jihgfedcba") == N(jihgfedcba) , "eosio::string_to_name(jihgfedcba)" );
   WASM_ASSERT( eosio::string_to_name("kjihgfedcba") == N(kjihgfedcba) , "eosio::string_to_name(kjihgfedcba)" );
   WASM_ASSERT( eosio::string_to_name("lkjihgfedcba") == N(lkjihgfedcba) , "eosio::string_to_name(lkjihgfedcba)" );
   WASM_ASSERT( eosio::string_to_name("mlkjihgfedcba") == N(mlkjihgfedcba) , "eosio::string_to_name(mlkjihgfedcba)" );
   WASM_ASSERT( eosio::string_to_name("mlkjihgfedcba1") == N(mlkjihgfedcba2) , "eosio::string_to_name(mlkjihgfedcba2)" );
   WASM_ASSERT( eosio::string_to_name("mlkjihgfedcba55") == N(mlkjihgfedcba14) , "eosio::string_to_name(mlkjihgfedcba14)" );
   
   WASM_ASSERT( eosio::string_to_name("azAA34") == N(azBB34) , "eosio::string_to_name N(azBB34)" );
   WASM_ASSERT( eosio::string_to_name("AZaz12Bc34") == N(AZaz12Bc34) , "eosio::string_to_name AZaz12Bc34" );
   WASM_ASSERT( eosio::string_to_name("AAAAAAAAAAAAAAA") == eosio::string_to_name("BBBBBBBBBBBBBDDDDDFFFGG") , "eosio::string_to_name BBBBBBBBBBBBBDDDDDFFFGG" );
   
   return WASM_TEST_PASS;
}

unsigned int test_types::name_class() {

   WASM_ASSERT ( eosio::name(eosio::string_to_name("azAA34")).value == N(azAA34), "eosio::name != N(azAA34)" );
   WASM_ASSERT ( eosio::name(eosio::string_to_name("AABBCC")).value == 0, "eosio::name != N(0)" );
   WASM_ASSERT ( eosio::name(eosio::string_to_name("AA11")).value == N(AA11), "eosio::name != N(AA11)" );
   WASM_ASSERT ( eosio::name(eosio::string_to_name("11AA")).value == N(11), "eosio::name != N(11)" );
   WASM_ASSERT ( eosio::name(eosio::string_to_name("22BBCCXXAA")).value == N(22), "eosio::name != N(22)" );
   WASM_ASSERT ( eosio::name(eosio::string_to_name("AAAbbcccdd")) == eosio::name(eosio::string_to_name("AAAbbcccdd")), "eosio::name == eosio::name" );

   uint64_t tmp = eosio::name(eosio::string_to_name("11bbcccdd"));
   WASM_ASSERT(N(11bbcccdd) == tmp, "N(11bbcccdd) == tmp");

   return WASM_TEST_PASS;  
}
