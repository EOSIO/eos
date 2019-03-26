/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include "deferred_test.hpp"
#include <eosio/transaction.hpp>

using namespace eosio;

void deferred_test::defercall( name payer, uint64_t sender_id, name contract, uint64_t payload ) {
   print( "defercall called on ", get_self(), "\n" );
   require_auth( payer );

   print( "deferred send of deferfunc action to ", contract, " by ", payer, " with sender id ", sender_id );
   transaction trx;
   deferfunc_action a( contract, {get_self(), "active"_n} );
   trx.actions.emplace_back( a.to_action( payload ) );
   trx.send( (static_cast<uint128_t>(payer.value) << 64) | sender_id, payer );
}

void deferred_test::deferfunc( uint64_t payload ) {
   print( "deferfunc called on ", get_self(), " with payload = ", payload, "\n" );
   check( payload != 13, "value 13 not allowed in payload" );
}

void deferred_test::inlinecall( name contract, name authorizer, uint64_t payload ) {
   deferfunc_action a( contract, {authorizer, "active"_n} );
   a.send( payload );
}

void deferred_test::on_error( uint128_t sender_id, ignore<std::vector<char>> sent_trx ) {
   print( "onerror called on ", get_self(), "\n" );
}
