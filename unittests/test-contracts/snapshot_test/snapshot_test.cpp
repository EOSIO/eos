/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include "snapshot_test.hpp"

using namespace eosio;

void snapshot_test::increment( uint32_t value ) {
   require_auth( get_self() );

   data_table data( get_self(), get_self().value );

   auto current = data.begin();
   if( current == data.end() ) {
      data.emplace( get_self(), [&]( auto& r ) {
         r.id         = value;
         r.index_f64  = value;
         r.index_f128 = value;
         r.index_i64  = value;
         r.index_i128 = value;
         r.index_i256.data()[0] = value;
      } );
   } else {
      data.modify( current, same_payer, [&]( auto& r ) {
         r.index_f64  += value;
         r.index_f128 += value;
         r.index_i64  += value;
         r.index_i128 += value;
         r.index_i256.data()[0] += value;
      } );
   }
}
