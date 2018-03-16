/**
 * @file
 * @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>
#include <iostream>
#include "test_api.hpp"

#define FORCE_EVALF(x) do { \
   volatile float __x;      \
   __x = (x);               \
} while(0);                 \

float test_softfloat::test_f32() {
   float values[3];
   read_action((char*)values, sizeof(values));
   float c = values[0] + values[1];
   return c;
   //eosio_assert(true, "hello");
   //std::cout << "f1 " << float(a) << "f2 " << float(b) << " " << float(c) << "\n";
}
