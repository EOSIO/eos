/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosiolib/action.h>
#include <eosiolib/chain.h>
#include <eosiolib/eosio.hpp>

#include "test_api.hpp"

#pragma pack(push, 1)
struct producers {
   char len;
   capi_name producers[21];
};
#pragma pack(pop)

void test_chain::test_activeprods() {
  producers act_prods;
  read_action_data( &act_prods, sizeof(producers) );
   
  eosio_assert( act_prods.len == 21, "producers.len != 21" );

  producers api_prods;
  get_active_producers( api_prods.producers, sizeof(eosio::name)*21 );

  for( int i = 0; i < 21 ; ++i )
      eosio_assert( api_prods.producers[i] == act_prods.producers[i], "Active producer" );
}
