/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>

#include "test_api.hpp"

unsigned int test_print::test_prints() {
  prints("ab");
  prints(nullptr);
  prints("c\0test_prints");
  prints(0);
  prints("efg");
  prints(0);
  return WASM_TEST_PASS;
}

unsigned int test_print::test_printi() {
  printi(0);
  printi(556644);
  printi(-1);
  return WASM_TEST_PASS;
}

unsigned int test_print::test_printi128() {
  uint128_t a(-1);
  uint128_t b(0);
  uint128_t c(87654323456);
  printi128(&a);
  printi128(&b);
  printi128(&c);
  return WASM_TEST_PASS;
}

unsigned int test_print::test_printn() {
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
  return WASM_TEST_PASS;
}
