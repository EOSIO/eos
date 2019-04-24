/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include "deferred_test.hpp"
#include <eosio/transaction.hpp>
#include <eosio/datastream.hpp>
#include <eosio/crypto.hpp>

using namespace eosio;

void deferred_test::defercall( name payer, uint64_t sender_id, name contract, uint64_t payload ) {
   print( "defercall called on ", get_self(), "\n" );
   require_auth( payer );

   print( "deferred send of deferfunc action to ", contract, " by ", payer, " with sender id ", sender_id );
   transaction trx;
   deferfunc_action a( contract, {get_self(), "active"_n} );
   trx.actions.emplace_back( a.to_action( payload ) );
   bool replace_existing = (payload >= 100);

   if( (50 <= payload && payload < 100) || payload >= 150 ) {
      size_t tx_size = transaction_size();
      char* buffer = (char*)malloc( tx_size );
      read_transaction( buffer, tx_size );
      auto tx_id = sha256( buffer, tx_size );
      char context_buffer[56];
      trx.transaction_extensions.emplace_back( (uint16_t)0, std::vector<char>() );
      auto& context_vector = std::get<1>( trx.transaction_extensions.back() );
      context_vector.resize(56);
      datastream ds( context_vector.data(), 56 );
      ds << tx_id.extract_as_byte_array();
      ds << ((static_cast<uint128_t>(payer.value) << 64) | sender_id);
      if( payload != 77 )
         ds << get_self();
      else
         ds << payer;
   }

   trx.send( (static_cast<uint128_t>(payer.value) << 64) | sender_id, payer, replace_existing );
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
