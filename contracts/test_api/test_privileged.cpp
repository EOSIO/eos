/**
 * @file
 * @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/privileged.h>
#include "test_api.hpp"

/*
void test_privileged::set_privileged() {
   prints("hello\n");
}
*/

void test_privileged::test_is_privileged() {
   prints("HELLO\n");
   eosio_assert( is_privileged( N(eosio) ), "eosio should be privileged" );
   eosio_assert( is_privileged( N(testapi) ), "testapi should not be privileged" );
   eosio_assert( is_privileged( N(acc2) ), "testapi should not be privileged" );
   prints("HELLO\n");
}
