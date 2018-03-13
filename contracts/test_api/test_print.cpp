/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>

#include "test_api.hpp"

//using namespace eosio;

void test_print::test_prints_l() {
  char ab[] = { 'a', 'b' };
  const char test[] = "test";
  prints_l(ab, 2);
  prints_l(ab, 1);
  prints_l(ab, 0);
  prints_l(test, sizeof(test)-1);
}

void test_print::test_prints() {
   prints("ab");
   prints(nullptr);
   prints("c\0test_prints");
   prints(0);
   prints("efg");
   prints(0);
}

void test_print::test_printi() {
   printi(0);
   printi(556644);
   printi((uint64_t)-1);
}

void test_print::test_printi128() {
  uint128_t a((uint128_t)-1);
  uint128_t b(0);
  uint128_t c(87654323456);
  printi128(&a);
  printi128(&b);
  printi128(&c);
}

void test_print::test_printn() {
   printn(N(abcde));
   printn(N(abBde));
   printn(N(1q1q1qAA));
   printn(N());
   printn(N(AAAAAA));
   printn(N(abcdefghijk));
   printn(N(abcdefghijkl));
   printn(N(abcdefghijkl1));
   printn(N(abcdefghijkl12));
   printn(N(abcdefghijkl123));
}
