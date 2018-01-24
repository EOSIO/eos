/**
 *    @file test_compiler_builtins.cpp
 *    @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include <eoslib/print.hpp>
#include <eoslib/compiler_builtins.h>

#include "test_api.hpp"

using namespace eosio;

void test_compiler_builtins::test_multi3() {
   /*
    * tests for negative values
    */
   __int128 res   = 0;
   __int128 lhs_a = -30;
   __int128 rhs_a = 100;
   __int128 lhs_b = 100;
   __int128 rhs_b = -30;

   __multi3(res, lhs_a, (lhs_a >> 64), rhs_a, (rhs_a >> 64));
   assert(res == -3000, "__multi3 result should be -3000"); 

   __multi3(res, lhs_b, (lhs_b >> 64), rhs_b, (rhs_b >> 64));
   assert(res == -3000, "__multi3 result should be -3000"); 

   __multi3(res, lhs_a, (lhs_a >> 64), rhs_b, (rhs_b >> 64));
   assert(res == 900, "__multi3 result should be 900"); 

   /*
    * test for positive values
    */
   __multi3(res, lhs_b, (lhs_b >> 64), rhs_a, (rhs_a >> 64));
   assert(res == 10000, "__multi3 result should be 10000"); 

   /*
    * test identity
    */
   __multi3(res, 1, 0, rhs_a, rhs_a >> 64);
   assert(res == 100, "__multi3 result should be 100");

   __multi3(res, 1, 0, rhs_b, rhs_b >> 64);
   assert(res == -30, "__multi3 result should be -30");
 

   /*
    * test for large values
    */
   uint128_t large_lhs;
}

void test_compiler_builtins::test_divti3() {
   /*
    * test for negative values
    */
   __int128 res   = 0;
   __int128 lhs_a = -30;
   __int128 rhs_a = 100;
   __int128 lhs_b = 100;
   __int128 rhs_b = -30;

   __divti3(res, lhs_a, (lhs_a >> 64), rhs_a, (rhs_a >> 64));
   assert(res == 0, "__divti3 result should be 0"); 

   __divti3(res, lhs_b, (lhs_b >> 64), rhs_b, (rhs_b >> 64));
   assert(res == -3, "__divti3 result should be -3"); 

   __divti3(res, lhs_a, (lhs_a >> 64), rhs_b, (rhs_b >> 64));
   assert(res == 1, "__divti3 result should be 1"); 

   /*
    * test for positive values
    */
   __int128 lhs_c = 3333;
   __int128 rhs_c = 3333;
   __divti3(res, lhs_b, (lhs_b >> 64), rhs_a, (rhs_a >> 64));
   assert(res == 1, "__divti3 result should be 1"); 

   __divti3(res, lhs_b, (lhs_b >> 64), rhs_c, (rhs_c >> 64));
   assert(res == 0, "__divti3 result should be 0"); 

   __divti3(res, lhs_c, (lhs_c >> 64), rhs_a, (rhs_a >> 64));
   assert(res == 33, "__divti3 result should be 33"); 

   /*
    * test identity
    */
   __divti3(res, lhs_b, (lhs_b >> 64), 1, 0);
   assert(res == 100, "__divti3 result should be 100"); 

   __divti3(res, lhs_a, (lhs_a >> 64), 1, 0);
   assert(res == -30, "__divti3 result should be -30"); 
}

void test_compiler_builtins::test_divti3_by_0() {
   __int128 res = 0;

   __divti3(res, 100, 0, 0, 0);
   assert(false, "Should have asserted");
}

void test_compiler_builtins::test_lshlti3() {
   __int128 res = 0;
   __int128 val = 1;

   __lshlti3(res, val, val >> 64, 0);
   assert(res == 1, "__lshlti3 result should be 1");


   __lshlti3(res, val, val >> 64, 1);
   assert(res == (1 << 1), "__lshlti3 result should be 2");

   __lshlti3(res, val, (val >> 64), 31);
   assert(res == 0x80000000, "__lshlti3 result should be 2^31");
   
   __lshlti3(res, val, (val >> 64), 63);
   assert(res == 0x8000000000000000, "__lshlti3 result should be 2^63");

   __lshlti3(res, val, (val >> 64), 64);
   __int128 test_res = 0;
   test_res = 0x8000000000000000;
   test_res <<= 1;
   assert(res == test_res, "__lshlti3 result should be 2^64");

   __lshlti3(res, val, (val >> 64), 127);
   test_res <<= 63;
   assert(res == test_res, "__lshlti3 result should be 2^127");

   __lshlti3(res, val, (val >> 64), 128);
   test_res <<= 1;
   //should rollover
   assert(res == test_res, "__lshlti3 result should be 2^128");
}

void test_compiler_builtins::test_ashlti3() {
   __int128 res = 0;
   __int128 val = 1;

   __ashlti3(res, val, val >> 64, 0);
   assert(res == 1, "__ashlti3 result should be 1");


   __ashlti3(res, val, val >> 64, 1);
   assert(res == (1 << 1), "__ashlti3 result should be 2");

   __ashlti3(res, val, (val >> 64), 31);
   assert(res == 0x80000000, "__ashlti3 result should be 2^31");
   
   __ashlti3(res, val, (val >> 64), 63);
   assert(res == 0x8000000000000000, "__ashlti3 result should be 2^63");

   __ashlti3(res, val, (val >> 64), 64);
   __int128 test_res = 0;
   test_res = 0x8000000000000000;
   test_res <<= 1;
   assert(res == test_res, "__ashlti3 result should be 2^64");

   __ashlti3(res, val, (val >> 64), 127);
   test_res <<= 63;
   assert(res == test_res, "__ashlti3 result should be 2^127");

   __ashlti3(res, val, (val >> 64), 128);
   test_res <<= 1;
   //should rollover
   assert(res == test_res, "__ashlti3 result should be 2^128");
}


void test_compiler_builtins::test_lshrti3() {
   auto atoi128 = [](const char* in) {
      __int128 ret = 0;
      size_t   i = 0;
      bool     sign = false;

      if (in[i] == '-') {
         ++i;
         sign = true;
      }

      if (in[i] == '+')
         ++i;

      for (; in[i] != '\0' ; ++i) {
         const char c = in[i];
         ret *= 10;
         ret += c - '0';
      }

      if (sign)
         ret *= -1;

      return ret;
   };

   __int128 res      = 0;
   __int128 val      = 0x8000000000000000;
   __int128 test_res = 0x8000000000000000;

   val      <<= 64;
   test_res <<= 64;
   
   __lshrti3(res, val, (val >> 64), 0);
   assert(res == test_res, "__lshrti3 result should be 2^127");

   __lshrti3(res, val, (val >> 64), 1);
   test_res >>= 1;
   assert(res == atoi128("85070591730234615865843651857942052864"), "__lshrti3 result should be 2^126");

   __lshrti3(res, val, (val >> 64), 63);
   assert(res == atoi128("18446744073709551616"), "__lshrti3 result should be 2^64");

   __lshrti3(res, val, (val >> 64), 64);
   assert(res == 0x8000000000000000, "__lshrti3 result should be 2^63");

   __lshrti3(res, val, (val >> 64), 96);
   assert(res == 0x80000000, "__lshrti3 result should be 2^31");

   __lshrti3(res, val, (val >> 64), 127);
   assert(res == 0x1, "__lshrti3 result should be 2^0");
}

void test_compiler_builtins::test_ashrti3() {
   auto atoi128 = [](const char* in) {
      __int128 ret = 0;
      size_t   i = 0;
      bool     sign = false;

      if (in[i] == '-') {
         ++i;
         sign = true;
      }

      if (in[i] == '+')
         ++i;

      for (; in[i] != '\0' ; ++i) {
         const char c = in[i];
         ret *= 10;
         ret += c - '0';
      }

      if (sign)
         ret *= -1;

      return ret;
   };
   
   auto bit_pattern = [](__int128 in, char* buff) {
      uint64_t high = in >> 64;
      uint64_t low = in;
      for (int i=0; i < 64; i++) {
         buff[63-i] = (low & (1<<i)) ? '1' : '0';
         buff[(127-i)] = (high & (1<<i)) ? '1' : '0';
      }
      buff[128] = '\0';
   };

   __int128 res      = 0;
   __int128 val      = atoi128("-170141183460469231731687303715884105728");

   __ashrti3(res, val, (val >> 64), 0);
   assert(res == atoi128("-170141183460469231731687303715884105728"), "__ashrti3 result should be 2^127");
   char buff[129] = {0};
   bit_pattern(-2, buff);
   prints("BITS A ");
   prints(buff);
   prints("\n");
   return;
   assert((res >> 64) >> 63, "should be one");

   __ashrti3(res, val, (val >> 64), 1);
   uint128_t a(res);
   uint128_t v(val);
   prints("RES ");
   printi128(&a);
   prints("\n");
   prints("VAL ");
   printi128(&v);
   prints("\n");
   assert(res == atoi128("85070591730234615865843651857942052864"), "__ashrti3 result should be 2^126");

   __ashrti3(res, val, (val >> 64), 63);
   assert(res == atoi128("18446744073709551616"), "__ashrti3 result should be 2^64");

   __ashrti3(res, val, (val >> 64), 64);
   assert(res == 0x8000000000000000, "__ashrti3 result should be 2^63");

   __ashrti3(res, val, (val >> 64), 95);
   assert(res == 0x80000000, "__ashrti3 result should be 2^31");

   __ashrti3(res, val, (val >> 64), 127);
   assert(res == 0x1, "__ashrti3 result should be 2^0");
}
