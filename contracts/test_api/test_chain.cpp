#include <eoslib/message.h>
#include <eoslib/chain.h>

#include "test_api.hpp"

#pragma pack(push, 1)
struct Producers {
   char len;
   AccountName producers[21];
};
#pragma pack(pop)

unsigned int test_chain::test_activeprods() {
  Producers msg_prods;
  readMessage(&msg_prods, sizeof(Producers));

  WASM_ASSERT(msg_prods.len == 21, "Producers.len != 21");

  Producers api_prods;
  getActiveProducers(api_prods.producers, sizeof(AccountName)*21);

  for( int i = 0; i < 21 ; ++i ) {
    WASM_ASSERT(api_prods.producers[i] == msg_prods.producers[i], "Active producer");
  }
  
  return WASM_TEST_PASS;
}