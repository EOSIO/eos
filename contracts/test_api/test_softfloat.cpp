/**
 * @file
 * @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>
#include <iostream>
#include "test_api.hpp"

void test_softfloat::test_f32() {
#if 1
   float values[3];
   read_action((char*)values, sizeof(values));
   /*
   float c = values[0] + values[1];
   prints("HELLO : \n");
   printi(*(uint32_t*)&c);
   printd(c);
   prints("\n");
   */
   //return values[0];
   eosio_assert(values[0] == 0.3f, "hello");
   //std::cout << "f1 " << float(a) << "f2 " << float(b) << " " << float(c) << "\n";
#endif
}
