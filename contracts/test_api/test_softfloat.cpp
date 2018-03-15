/**
 * @file
 * @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>
#include <iostream>
#include "test_api.hpp"

void test_softfloat::test_f32() {
   float a = 0.3f;
   float b = -0.0f;
   float c = a + b;
   std::cout << "f1 " << a << "f2 " << b << " " << c << "\n";
}
