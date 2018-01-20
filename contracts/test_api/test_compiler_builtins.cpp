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

   uint128_t r2(res);
   prints("res2 ");
   printi128(&r2);
   prints("\n");
}

void test_compiler_builtins::test_divti3_by_0() {
   __int128 res = 0;

   __divti3(res, 100, 0, 0, 0);
   assert(false, "Should have thrown divide by zero");
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
   //assert(res == (1 << 31), "__lshlti3 result should be 2^31");
   
   __lshlti3(res, val, (val >> 64), 63);
   assert(res == 0x8000000000000000, "__lshlti3 result should be 2^63");
   //assert(res == (1 << 31), "__lshlti3 result should be 2^31");
   //assert(false, "Should have thrown divide by zero");
   //__lshlti3(res, val, (val >> 64), 65);
   //assert(res == 0x20000000000000000L, "__lshlti3 result should be 2^63");
   //assert(res == , "__lshlti3 result should be 2^63");
}
