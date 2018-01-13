/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/action.h>
#include <eoslib/chain.h>

#include "test_api.hpp"

#pragma pack(push, 1)
struct producers {
   char len;
   account_name producers[21];
};
#pragma pack(pop)

unsigned int test_chain::test_activeprods() {
  producers msg_prods;
  read_action(&msg_prods, sizeof(producers));

  WASM_ASSERT(msg_prods.len == 21, "producers.len != 21");

  producers api_prods;
  get_active_producers(api_prods.producers, sizeof(account_name)*21);

  for( int i = 0; i < 21 ; ++i ) {
    WASM_ASSERT(api_prods.producers[i] == msg_prods.producers[i], "Active producer");
  }
  
  return WASM_TEST_PASS;
}
